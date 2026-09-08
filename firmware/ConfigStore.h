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
  uint32_t provisioningRevision();
  bool saveProvisioningRevision(uint32_t revision);
  uint32_t defaultLayoutRevision();
  bool saveDefaultLayoutRevision(uint32_t revision);
  void clear();

 private:
  Preferences preferences_;
};
