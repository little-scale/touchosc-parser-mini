// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include <Preferences.h>

#include "AppTypes.h"

class ConfigStore {
 public:
  bool begin();
  void load(DeviceSettings &settings);
  bool save(const DeviceSettings &settings);
  void clear();

 private:
  Preferences preferences_;
};
