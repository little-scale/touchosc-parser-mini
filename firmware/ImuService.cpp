// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include "ImuService.h"

#include <Wire.h>

namespace {
constexpr uint8_t kAddress = 0x6b;
constexpr uint8_t kWhoAmI = 0x00;
constexpr uint8_t kExpectedWhoAmI = 0x05;
constexpr uint8_t kControl1 = 0x02;
constexpr uint8_t kControl2 = 0x03;
constexpr uint8_t kControl3 = 0x04;
constexpr uint8_t kControl5 = 0x06;
constexpr uint8_t kControl7 = 0x08;
constexpr uint8_t kControl8 = 0x09;
constexpr uint8_t kStatus0 = 0x2e;
constexpr uint8_t kAccelXLow = 0x35;
constexpr uint8_t kResetResult = 0x4d;
constexpr uint8_t kReset = 0x60;
constexpr float kAccelScale = 4.0f / 32768.0f;
constexpr float kGyroScale = 512.0f / 32768.0f;
constexpr float kRadiansToDegrees = 57.2957795131f;
constexpr float kComplementaryGyroWeight = 0.97f;

int16_t readI16(const uint8_t *data) {
  return static_cast<int16_t>(static_cast<uint16_t>(data[0]) |
                              (static_cast<uint16_t>(data[1]) << 8));
}

float pitchFromAccel(const float accel[3]) {
  return atan2f(-accel[0], sqrtf(accel[1] * accel[1] + accel[2] * accel[2])) *
         kRadiansToDegrees;
}

float rollFromAccel(const float accel[3]) {
  return atan2f(accel[1], accel[2]) * kRadiansToDegrees;
}

float wrap180(float value) {
  while (value > 180.0f) value -= 360.0f;
  while (value < -180.0f) value += 360.0f;
  return value;
}
}  // namespace

bool ImuService::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(kAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool ImuService::readRegister(uint8_t reg, uint8_t &value) {
  return readRegisters(reg, &value, 1);
}

bool ImuService::readRegisters(uint8_t reg, uint8_t *data, size_t length) {
  Wire.beginTransmission(kAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(kAddress, length) != length) return false;
  for (size_t index = 0; index < length; ++index) {
    if (!Wire.available()) return false;
    data[index] = Wire.read();
  }
  return true;
}

bool ImuService::begin(const DeviceSettings &settings) {
  rateHz_ = settings.imuRateHz == 50 ? 50 : 25;
  memcpy(gyroBias_, settings.gyroBias, sizeof(gyroBias_));
  pitchOffset_ = settings.pitchOffset;
  rollOffset_ = settings.rollOffset;

  if (!writeRegister(kReset, 0xb0)) return false;
  const uint32_t resetDeadline = millis() + 500;
  uint8_t resetResult = 0;
  while (static_cast<int32_t>(resetDeadline - millis()) > 0) {
    if (readRegister(kResetResult, resetResult) && resetResult == 0x80) break;
    delay(10);
  }
  if (resetResult != 0x80) return false;

  // Little-endian samples with automatic register-address increment.
  if (!writeRegister(kControl1, 0x40)) return false;
  uint8_t identity = 0;
  if (!readRegister(kWhoAmI, identity) || identity != kExpectedWhoAmI) return false;

  // ±4 g at 250 Hz, ±512 degrees/second at 224.2 Hz, matching Micro Apps.
  if (!writeRegister(kControl7, 0x00) ||
      !writeRegister(kControl8, 0x80) ||
      !writeRegister(kControl2, 0x15) ||
      !writeRegister(kControl3, 0x55) ||
      !writeRegister(kControl5, 0x71) ||
      !writeRegister(kControl7, 0x03)) {
    return false;
  }

  lastSampleUs_ = micros();
  available_ = true;
  return true;
}

void ImuService::updateOrientation(ImuFrame &frame, float dt) {
  const float accelPitch = pitchFromAccel(frame.accel);
  const float accelRoll = rollFromAccel(frame.accel);
  if (!orientationInitialized_) {
    pitch_ = accelPitch;
    roll_ = accelRoll;
    orientationInitialized_ = true;
  } else {
    pitch_ = kComplementaryGyroWeight * (pitch_ + frame.gyro[1] * dt) +
             (1.0f - kComplementaryGyroWeight) * accelPitch;
    roll_ = kComplementaryGyroWeight * (roll_ + frame.gyro[0] * dt) +
            (1.0f - kComplementaryGyroWeight) * accelRoll;
  }
  yaw_ = wrap180(yaw_ + frame.gyro[2] * dt);
  frame.orientation[0] = wrap180(pitch_ - pitchOffset_);
  frame.orientation[1] = wrap180(roll_ - rollOffset_);
  frame.orientation[2] = yaw_;
}

bool ImuService::update(ImuFrame &frame) {
  if (!available_) return false;
  const uint32_t now = micros();
  const uint32_t period = 1000000UL / rateHz_;
  if (static_cast<uint32_t>(now - lastSampleUs_) < period) return false;

  uint8_t status = 0;
  if (!readRegister(kStatus0, status) || (status & 0x03) == 0) return false;
  uint8_t raw[12];
  if (!readRegisters(kAccelXLow, raw, sizeof(raw))) return false;

  float sensorAccel[3];
  float sensorGyro[3];
  for (uint8_t axis = 0; axis < 3; ++axis) {
    sensorAccel[axis] = readI16(raw + axis * 2) * kAccelScale;
    sensorGyro[axis] = readI16(raw + 6 + axis * 2) * kGyroScale;
  }

  // The sensor is mounted 90 degrees clockwise relative to the portrait screen.
  frame.accel[0] = sensorAccel[1];
  frame.accel[1] = sensorAccel[0];
  frame.accel[2] = sensorAccel[2];
  frame.gyro[0] = sensorGyro[1] - gyroBias_[0];
  frame.gyro[1] = sensorGyro[0] - gyroBias_[1];
  frame.gyro[2] = sensorGyro[2] - gyroBias_[2];

  float dt = static_cast<float>(now - lastSampleUs_) / 1000000.0f;
  lastSampleUs_ = now;
  dt = constrain(dt, 0.001f, 0.1f);
  updateOrientation(frame, dt);
  return true;
}
