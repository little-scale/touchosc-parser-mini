// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include "AppTypes.h"

// Provisioning.local.h is intentionally private and excluded from Git. The
// checked-in example documents every supported value.
#if __has_include("Provisioning.local.h")
#include "Provisioning.local.h"
#endif

#ifndef TPM_PROVISIONING_REVISION
#define TPM_PROVISIONING_REVISION 0
#endif

#ifndef TPM_WIFI_SSID
#define TPM_WIFI_SSID ""
#endif

#ifndef TPM_WIFI_PASSWORD
#define TPM_WIFI_PASSWORD ""
#endif

#ifndef TPM_OSC_TARGET
#define TPM_OSC_TARGET "192.168.1.2"
#endif

#ifndef TPM_OSC_SEND_PORT
#define TPM_OSC_SEND_PORT 9000
#endif

#ifndef TPM_OSC_RECEIVE_PORT
#define TPM_OSC_RECEIVE_PORT 9001
#endif

namespace Provisioning {

inline uint32_t revision() {
  return static_cast<uint32_t>(TPM_PROVISIONING_REVISION);
}

inline bool available() {
  return revision() > 0 && String(TPM_WIFI_SSID).length() > 0;
}

inline uint16_t validPort(uint32_t value, uint16_t fallback) {
  return value >= 1 && value <= 65535 ? static_cast<uint16_t>(value) : fallback;
}

inline void apply(DeviceSettings &settings) {
  settings.wifiSsid = TPM_WIFI_SSID;
  settings.wifiPassword = TPM_WIFI_PASSWORD;
  settings.oscTarget = TPM_OSC_TARGET;
  settings.oscTarget.trim();
  settings.oscSendPort = validPort(TPM_OSC_SEND_PORT, 9000);
  settings.oscReceivePort = validPort(TPM_OSC_RECEIVE_PORT, 9001);
}

}  // namespace Provisioning
