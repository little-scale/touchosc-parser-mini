// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include "ConfigStore.h"

namespace {
constexpr char kNamespace[] = "classroom";
}

bool ConfigStore::begin() {
  return preferences_.begin(kNamespace, false);
}

void ConfigStore::load(DeviceSettings &settings) {
  settings.deviceName = preferences_.getString("name", settings.deviceName);
  settings.wifiSsid = preferences_.getString("ssid", settings.wifiSsid);
  settings.wifiPassword = preferences_.getString("pass", settings.wifiPassword);
  settings.oscTarget = preferences_.getString("osc_host", settings.oscTarget);
  settings.oscSendPort = preferences_.getUShort("osc_tx", settings.oscSendPort);
  settings.oscReceivePort = preferences_.getUShort("osc_rx", settings.oscReceivePort);
  settings.oscIncludeDeviceName = preferences_.getBool(
      "layout_prefix", settings.oscIncludeDeviceName);
  settings.imuRateHz = preferences_.getUChar("imu_hz", settings.imuRateHz);
  settings.imuOutputEnabled = preferences_.getBool("imu_out", settings.imuOutputEnabled);
  settings.bleEnabled = preferences_.getBool("ble", settings.bleEnabled);
  settings.dimAfterMs = preferences_.getULong("dim_ms", settings.dimAfterMs);
  settings.ballGravity = preferences_.getFloat("ball_grav", settings.ballGravity);
  settings.ballBounciness = preferences_.getFloat("ball_bounce", settings.ballBounciness);
  settings.gyroBias[0] = preferences_.getFloat("gb_x", 0.0f);
  settings.gyroBias[1] = preferences_.getFloat("gb_y", 0.0f);
  settings.gyroBias[2] = preferences_.getFloat("gb_z", 0.0f);
  settings.pitchOffset = preferences_.getFloat("pitch_0", 0.0f);
  settings.rollOffset = preferences_.getFloat("roll_0", 0.0f);

  if (settings.imuRateHz != 50) settings.imuRateHz = 25;
  settings.oscSendPort = constrain(settings.oscSendPort, 1, 65535);
  settings.oscReceivePort = constrain(settings.oscReceivePort, 1, 65535);
  settings.ballGravity = constrain(settings.ballGravity, 0.5f, 4.0f);
  settings.ballBounciness = constrain(settings.ballBounciness, 0.0f, 1.0f);
  settings.deviceName.trim();
  settings.oscTarget.trim();
  if (settings.deviceName.isEmpty()) settings.deviceName = "device-0000";
}

bool ConfigStore::save(const DeviceSettings &settings) {
  bool ok = true;
  ok &= preferences_.putString("name", settings.deviceName) > 0;
  preferences_.putString("ssid", settings.wifiSsid);
  preferences_.putString("pass", settings.wifiPassword);
  ok &= preferences_.putString("osc_host", settings.oscTarget) > 0;
  ok &= preferences_.putUShort("osc_tx", settings.oscSendPort) == sizeof(uint16_t);
  ok &= preferences_.putUShort("osc_rx", settings.oscReceivePort) == sizeof(uint16_t);
  ok &= preferences_.putBool("layout_prefix", settings.oscIncludeDeviceName) == sizeof(bool);
  ok &= preferences_.putUChar("imu_hz", settings.imuRateHz) == sizeof(uint8_t);
  ok &= preferences_.putBool("imu_out", settings.imuOutputEnabled) == sizeof(bool);
  ok &= preferences_.putBool("ble", settings.bleEnabled) == sizeof(bool);
  ok &= preferences_.putULong("dim_ms", settings.dimAfterMs) == sizeof(uint32_t);
  ok &= preferences_.putFloat("ball_grav", settings.ballGravity) == sizeof(float);
  ok &= preferences_.putFloat("ball_bounce", settings.ballBounciness) == sizeof(float);
  ok &= preferences_.putFloat("gb_x", settings.gyroBias[0]) == sizeof(float);
  ok &= preferences_.putFloat("gb_y", settings.gyroBias[1]) == sizeof(float);
  ok &= preferences_.putFloat("gb_z", settings.gyroBias[2]) == sizeof(float);
  ok &= preferences_.putFloat("pitch_0", settings.pitchOffset) == sizeof(float);
  ok &= preferences_.putFloat("roll_0", settings.rollOffset) == sizeof(float);
  return ok;
}

uint32_t ConfigStore::provisioningRevision() {
  return preferences_.getULong("prov_rev", 0);
}

bool ConfigStore::saveProvisioningRevision(uint32_t revision) {
  return preferences_.putULong("prov_rev", revision) == sizeof(uint32_t);
}

uint32_t ConfigStore::defaultLayoutRevision() {
  return preferences_.getULong("layout_rev", 0);
}

bool ConfigStore::saveDefaultLayoutRevision(uint32_t revision) {
  return preferences_.putULong("layout_rev", revision) == sizeof(uint32_t);
}

void ConfigStore::clear() {
  preferences_.clear();
}
