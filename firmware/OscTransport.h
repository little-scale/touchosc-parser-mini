// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>

#include "AppTypes.h"

class OscTransport {
 public:
  void begin(const DeviceSettings &settings, RemoteMessageHandler handler);
  void reconfigure(const DeviceSettings &settings);
  void setLayoutMode(bool active) { layoutMode_ = active; }
  void loop();

  void sendXy(float x, float y);
  void sendFader(uint8_t index, float value);
  void sendToggle(uint8_t index, bool value);
  void sendButton(uint8_t index, bool value);
  void sendNote(uint8_t midiNote, bool value, uint8_t velocity);
  void sendSpectrum(const float values[32]);
  void sendWallCollision(uint8_t ball, uint8_t wall, float impact);
  void sendBallCollision(uint8_t firstBall, uint8_t secondBall, float impact);
  void sendParticleWall(uint8_t wall, float normalizedSize);
  void sendMovementTrigger();
  void sendMicEnergy(float energy);
  void sendPendulum(uint8_t index, const float values[6]);
  void sendPendulumPing(uint8_t index, uint8_t ping);
  void sendPendulumActive(uint8_t index, bool active);
  void sendImu(const ImuFrame &frame);
  void sendLayout(const char *address, const float *values, uint8_t valueCount);

 private:
  bool sendPacket(const String &address, const char *types, const float *floats,
                  size_t floatCount, const int32_t *integers, size_t integerCount);
  bool decodePacket(const uint8_t *data, size_t length, RemoteMessage &message) const;
  String addressFor(const char *control, uint8_t index) const;
  String layoutAddressFor(const char *address) const;

  WiFiUDP udp_;
  DeviceSettings settings_;
  RemoteMessageHandler handler_ = nullptr;
  bool listening_ = false;
  bool layoutMode_ = false;
};
