#pragma once

#include <Arduino.h>

enum class InputSource : uint8_t {
  LocalTouch,
  Osc,
  Ble,
};

struct ControlState {
  float xyX = 0.5f;
  float xyY = 0.5f;
  float faders[4] = {0.5f, 0.5f, 0.5f, 0.5f};
  bool buttons[4] = {false, false, false, false};
  bool keyboardNotes[128] = {};
  float spectrum[32] = {};
  uint8_t background[3] = {0, 0, 0};
};

struct ImuFrame {
  float accel[3] = {0.0f, 0.0f, 0.0f};
  float gyro[3] = {0.0f, 0.0f, 0.0f};
  float orientation[3] = {0.0f, 0.0f, 0.0f};  // pitch, roll, relative yaw
};

enum class MessageType : uint8_t {
  Invalid = 0x00,
  Xy = 0x01,
  Fader = 0x02,
  Toggle = 0x03,
  Button = 0x04,
  Note = 0x05,
  Spectrum = 0x06,
  Imu = 0x10,
  ImuAccel = 0x11,
  ImuGyro = 0x12,
  ImuOrientation = 0x13,
  Background = 0x20,
  MovementTrigger = 0x21,
  MicEnergy = 0x22,
  Layout = 0x30,
  LayoutText = 0x31,
};

struct RemoteMessage {
  MessageType type = MessageType::Invalid;
  uint8_t index = 0;
  float values[32] = {};
  int32_t intValue = 0;
  uint8_t rgb[3] = {};
  String address;
  String textValue;
  uint8_t valueCount = 0;
};

using RemoteMessageHandler = void (*)(const RemoteMessage &, InputSource);

struct DeviceSettings {
  String deviceName = "device-0000";
  String wifiSsid;
  String wifiPassword;
  String oscTarget = "192.168.1.2";
  uint16_t oscSendPort = 9000;
  uint16_t oscReceivePort = 9001;
  uint8_t imuRateHz = 25;
  bool imuOutputEnabled = false;
  bool bleEnabled = true;
  uint32_t dimAfterMs = 60000;
  float ballGravity = 2.2f;
  float ballBounciness = 0.82f;
  float gyroBias[3] = {};
  float pitchOffset = 0.0f;
  float rollOffset = 0.0f;
};
