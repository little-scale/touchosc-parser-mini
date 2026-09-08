// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

#include "AppTypes.h"

class ImuService {
 public:
  bool begin(const DeviceSettings &settings);
  bool update(ImuFrame &frame);
  bool available() const { return available_; }

 private:
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegister(uint8_t reg, uint8_t &value);
  bool readRegisters(uint8_t reg, uint8_t *data, size_t length);
  void updateOrientation(ImuFrame &frame, float dt);

  bool available_ = false;
  bool orientationInitialized_ = false;
  uint8_t rateHz_ = 25;
  uint32_t lastSampleUs_ = 0;
  float gyroBias_[3] = {};
  float pitchOffset_ = 0.0f;
  float rollOffset_ = 0.0f;
  float pitch_ = 0.0f;
  float roll_ = 0.0f;
  float yaw_ = 0.0f;
};
