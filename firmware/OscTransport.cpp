// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include "OscTransport.h"

#include <WiFi.h>

namespace {
constexpr size_t kMaxOscPacket = 256;
constexpr char kSpectrumOscTypes[] = ",ffffffffffffffffffffffffffffffff";

size_t paddedStringLength(size_t stringLength) {
  return (stringLength + 1 + 3) & ~static_cast<size_t>(3);
}

bool writePaddedString(uint8_t *buffer, size_t capacity, size_t &offset, const char *text) {
  const size_t textLength = strlen(text);
  const size_t required = paddedStringLength(textLength);
  if (offset + required > capacity) return false;
  memcpy(buffer + offset, text, textLength);
  memset(buffer + offset + textLength, 0, required - textLength);
  offset += required;
  return true;
}

void writeU32Be(uint8_t *target, uint32_t value) {
  target[0] = static_cast<uint8_t>(value >> 24);
  target[1] = static_cast<uint8_t>(value >> 16);
  target[2] = static_cast<uint8_t>(value >> 8);
  target[3] = static_cast<uint8_t>(value);
}

uint32_t readU32Be(const uint8_t *source) {
  return (static_cast<uint32_t>(source[0]) << 24) |
         (static_cast<uint32_t>(source[1]) << 16) |
         (static_cast<uint32_t>(source[2]) << 8) |
         static_cast<uint32_t>(source[3]);
}

bool readPaddedString(const uint8_t *data, size_t length, size_t &offset, String &result) {
  if (offset >= length) return false;
  size_t end = offset;
  while (end < length && data[end] != 0) ++end;
  if (end == length) return false;
  result = String(reinterpret_cast<const char *>(data + offset), end - offset);
  offset += paddedStringLength(end - offset);
  return offset <= length;
}

float readFloatBe(const uint8_t *source) {
  const uint32_t bits = readU32Be(source);
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

void writeFloatBe(uint8_t *target, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  writeU32Be(target, bits);
}

bool finiteUnit(float value) {
  return isfinite(value) && value >= 0.0f && value <= 1.0f;
}
}  // namespace

void OscTransport::begin(const DeviceSettings &settings, RemoteMessageHandler handler) {
  handler_ = handler;
  reconfigure(settings);
}

void OscTransport::reconfigure(const DeviceSettings &settings) {
  settings_ = settings;
  if (listening_) udp_.stop();
  listening_ = false;
}

String OscTransport::addressFor(const char *control, uint8_t index) const {
  String address = "/";
  address += settings_.deviceName;
  address += "/";
  address += control;
  address += String(index);
  return address;
}

String OscTransport::layoutAddressFor(const char *address) const {
  if (!settings_.oscIncludeDeviceName) return address ? String(address) : String();
  String namespaced = "/";
  namespaced += settings_.deviceName;
  if (!address || address[0] != '/') namespaced += "/";
  if (address) namespaced += address;
  return namespaced;
}

bool OscTransport::sendPacket(const String &address, const char *types, const float *floats,
                              size_t floatCount, const int32_t *integers, size_t integerCount) {
  if (WiFi.status() != WL_CONNECTED || settings_.oscTarget.isEmpty()) return false;

  uint8_t packet[kMaxOscPacket] = {};
  size_t offset = 0;
  if (!writePaddedString(packet, sizeof(packet), offset, address.c_str()) ||
      !writePaddedString(packet, sizeof(packet), offset, types)) {
    return false;
  }

  size_t floatIndex = 0;
  size_t integerIndex = 0;
  for (size_t i = 1; types[i] != '\0'; ++i) {
    if (offset + 4 > sizeof(packet)) return false;
    if (types[i] == 'f' && floatIndex < floatCount) {
      writeFloatBe(packet + offset, floats[floatIndex++]);
    } else if (types[i] == 'i' && integerIndex < integerCount) {
      writeU32Be(packet + offset, static_cast<uint32_t>(integers[integerIndex++]));
    } else {
      return false;
    }
    offset += 4;
  }

  if (!udp_.beginPacket(settings_.oscTarget.c_str(), settings_.oscSendPort)) return false;
  udp_.write(packet, offset);
  return udp_.endPacket() == 1;
}

void OscTransport::sendXy(float x, float y) {
  const float values[] = {x, y};
  sendPacket(addressFor("xy", 0), ",ff", values, 2, nullptr, 0);
}

void OscTransport::sendFader(uint8_t index, float value) {
  sendPacket(addressFor("fader", index), ",f", &value, 1, nullptr, 0);
}

void OscTransport::sendToggle(uint8_t index, bool value) {
  const int32_t integer = value ? 1 : 0;
  sendPacket(addressFor("toggle", index), ",i", nullptr, 0, &integer, 1);
}

void OscTransport::sendButton(uint8_t index, bool value) {
  const int32_t integer = value ? 1 : 0;
  sendPacket(addressFor("button", index), ",i", nullptr, 0, &integer, 1);
}

void OscTransport::sendNote(uint8_t midiNote, bool value, uint8_t velocity) {
  const String address = "/" + settings_.deviceName + "/note";
  const int32_t integers[] = {midiNote, value ? 1 : 0, value ? velocity : 0};
  sendPacket(address, ",iii", nullptr, 0, integers, 3);
}

void OscTransport::sendSpectrum(const float values[32]) {
  sendPacket(addressFor("spectrum", 0), kSpectrumOscTypes, values, 32, nullptr, 0);
}

void OscTransport::sendWallCollision(uint8_t ball, uint8_t wall, float impact) {
  const String address = "/" + settings_.deviceName + "/collision/wall";
  const int32_t indices[] = {ball, wall};
  sendPacket(address, ",iif", &impact, 1, indices, 2);
}

void OscTransport::sendBallCollision(uint8_t firstBall, uint8_t secondBall,
                                     float impact) {
  const String address = "/" + settings_.deviceName + "/collision/ball";
  const int32_t indices[] = {firstBall, secondBall};
  sendPacket(address, ",iif", &impact, 1, indices, 2);
}

void OscTransport::sendParticleWall(uint8_t wall, float normalizedSize) {
  const String address = "/" + settings_.deviceName + "/particle/wall";
  const int32_t wallNumber = wall;
  sendPacket(address, ",if", &normalizedSize, 1, &wallNumber, 1);
}

void OscTransport::sendMovementTrigger() {
  const String address = "/" + settings_.deviceName + "/movement";
  const int32_t triggered = 1;
  sendPacket(address, ",i", nullptr, 0, &triggered, 1);
}

void OscTransport::sendMicEnergy(float energy) {
  sendPacket(addressFor("mic", 0), ",f", &energy, 1, nullptr, 0);
}

void OscTransport::sendPendulum(uint8_t index, const float values[6]) {
  sendPacket(addressFor("pendulum", index), ",ffffff", values, 6, nullptr, 0);
}

void OscTransport::sendPendulumPing(uint8_t index, uint8_t ping) {
  static const char *names[] = {"centre", "left", "right"};
  if (ping >= 3) return;
  const String address = "/" + settings_.deviceName + "/pendulum" + String(index) +
                         "/" + names[ping];
  const int32_t triggered = 1;
  sendPacket(address, ",i", nullptr, 0, &triggered, 1);
}

void OscTransport::sendPendulumActive(uint8_t index, bool active) {
  const String address = "/" + settings_.deviceName + "/pendulum" + String(index) +
                         "/active";
  const int32_t value = active ? 1 : 0;
  sendPacket(address, ",i", nullptr, 0, &value, 1);
}

void OscTransport::sendImu(const ImuFrame &frame) {
  float values[9];
  memcpy(values, frame.accel, sizeof(frame.accel));
  memcpy(values + 3, frame.gyro, sizeof(frame.gyro));
  memcpy(values + 6, frame.orientation, sizeof(frame.orientation));
  sendPacket(addressFor("imu", 0), ",fffffffff", values, 9, nullptr, 0);
}

void OscTransport::sendLayout(const char *address, const float *values,
                              uint8_t valueCount) {
  if (!address || address[0] != '/' || valueCount == 0 || valueCount > 2) return;
  sendPacket(layoutAddressFor(address), valueCount == 1 ? ",f" : ",ff",
             values, valueCount, nullptr, 0);
}

bool OscTransport::decodePacket(const uint8_t *data, size_t length, RemoteMessage &message) const {
  size_t offset = 0;
  String address;
  String types;
  if (!readPaddedString(data, length, offset, address) ||
      !readPaddedString(data, length, offset, types) || !types.startsWith(",")) {
    return false;
  }

  const String root = "/" + settings_.deviceName + "/";
  const bool deviceAddress = address.startsWith(root);
  const String endpoint = deviceAddress ? address.substring(root.length()) : String();
  const String layoutAddress = deviceAddress ? String("/") + endpoint : address;

  auto readFloat = [&](size_t position, float &value) {
    if (position + 4 > length) return false;
    value = readFloatBe(data + position);
    return isfinite(value);
  };
  auto readInt = [&](size_t position, int32_t &value) {
    if (position + 4 > length) return false;
    value = static_cast<int32_t>(readU32Be(data + position));
    return true;
  };

  // Uploaded TouchOSC documents own their addresses. Resolve their receive
  // mappings before the legacy built-in endpoints so even an intentional
  // address collision updates the visible layout.
  if (layoutMode_ && (types == ",f" || types == ",ff") && address.startsWith("/")) {
    message.type = MessageType::Layout;
    message.address = layoutAddress;
    message.valueCount = types == ",ff" ? 2 : 1;
    for (uint8_t index = 0; index < message.valueCount; ++index) {
      if (!readFloat(offset + index * 4, message.values[index]) ||
          !finiteUnit(message.values[index])) return false;
    }
    return true;
  }
  if (layoutMode_ && types == ",s" && address.startsWith("/")) {
    message.type = MessageType::LayoutText;
    message.address = layoutAddress;
    return readPaddedString(data, length, offset, message.textValue);
  }

  if (deviceAddress && endpoint == "xy0" && types == ",ff") {
    message.type = MessageType::Xy;
    if (!readFloat(offset, message.values[0]) || !readFloat(offset + 4, message.values[1])) return false;
    return finiteUnit(message.values[0]) && finiteUnit(message.values[1]);
  }
  for (uint8_t index = 0; index < 4; ++index) {
    if (deviceAddress && endpoint == String("fader") + String(index) && types == ",f") {
      message.type = MessageType::Fader;
      message.index = index;
      if (!readFloat(offset, message.values[0])) return false;
      return finiteUnit(message.values[0]);
    }
    if (deviceAddress && endpoint == String("button") + String(index) && types == ",i") {
      message.type = MessageType::Button;
      message.index = index;
      return readInt(offset, message.intValue) &&
             (message.intValue == 0 || message.intValue == 1);
    }
  }
  if (deviceAddress && endpoint == "note" && (types == ",ii" || types == ",iii")) {
    int32_t midiNote;
    message.type = MessageType::Note;
    if (!readInt(offset, midiNote) || !readInt(offset + 4, message.intValue) ||
        midiNote < 0 || midiNote > 127 ||
        (message.intValue != 0 && message.intValue != 1)) {
      return false;
    }
    if (types == ",iii") {
      int32_t velocity;
      if (!readInt(offset + 8, velocity) || velocity < 0 || velocity > 127) return false;
    }
    message.index = static_cast<uint8_t>(midiNote);
    return true;
  }
  if (deviceAddress && endpoint == "spectrum0" && types == kSpectrumOscTypes) {
    message.type = MessageType::Spectrum;
    message.index = 0;
    for (uint8_t index = 0; index < 32; ++index) {
      if (!readFloat(offset + index * 4, message.values[index]) ||
          !finiteUnit(message.values[index])) return false;
    }
    return true;
  }
  if (deviceAddress && endpoint == "background" && types == ",iii") {
    message.type = MessageType::Background;
    int32_t rgb[3];
    for (uint8_t i = 0; i < 3; ++i) {
      if (!readInt(offset + i * 4, rgb[i]) || rgb[i] < 0 || rgb[i] > 255) return false;
      message.rgb[i] = static_cast<uint8_t>(rgb[i]);
    }
    return true;
  }
  if ((types == ",f" || types == ",ff") && address.startsWith("/")) {
    message.type = MessageType::Layout;
    message.address = address;
    message.valueCount = types == ",ff" ? 2 : 1;
    for (uint8_t index = 0; index < message.valueCount; ++index) {
      if (!readFloat(offset + index * 4, message.values[index]) ||
          !finiteUnit(message.values[index])) return false;
    }
    return true;
  }
  if (types == ",s" && address.startsWith("/")) {
    message.type = MessageType::LayoutText;
    message.address = address;
    return readPaddedString(data, length, offset, message.textValue);
  }
  return false;
}

void OscTransport::loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (listening_) {
      udp_.stop();
      listening_ = false;
    }
    return;
  }
  if (!listening_) listening_ = udp_.begin(settings_.oscReceivePort) == 1;
  if (!listening_) return;

  int packetSize;
  while ((packetSize = udp_.parsePacket()) > 0) {
    uint8_t packet[kMaxOscPacket];
    const size_t received = udp_.read(packet, min(packetSize, static_cast<int>(sizeof(packet))));
    RemoteMessage message;
    if (decodePacket(packet, received, message) && handler_) handler_(message, InputSource::Osc);
  }
}
