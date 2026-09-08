# Private fleet provisioning porting guide

This document describes the revision-controlled provisioning system used by TouchOSC Parser Mini. It is intended for porting the same behavior to the Micro Apps firmware without importing Parser Mini-specific layout or OSC code.

The implementation is first-party TouchOSC Parser Mini code and is available under the repository's MIT License.

## Intended behavior

- One private firmware build can give many ESP32-S3 boards the same Wi-Fi and OSC destination.
- Every board keeps a unique MAC-derived device name.
- Provisioning is applied once per revision and then recorded in ESP32 Preferences/NVS.
- Reflashing the same revision preserves settings changed through the device UI.
- Increasing the revision deliberately reapplies the shared settings.
- A normal public build contains no credentials and never changes saved provisioning.

## Files to add

Use three parts:

1. A checked-in provisioning loader such as `Provisioning.h`.
2. A checked-in `Provisioning.local.h.example` template.
3. A private `Provisioning.local.h` containing the real values.

Add the private path to `.gitignore`:

```gitignore
firmware/Provisioning.local.h
```

The private file should define only shared fleet values—not the device name:

```cpp
#pragma once

#define APP_PROVISIONING_REVISION 1
#define APP_WIFI_SSID "Your Wi-Fi network"
#define APP_WIFI_PASSWORD "Your Wi-Fi password"
#define APP_OSC_TARGET "192.168.1.100"
#define APP_OSC_SEND_PORT 9000
#define APP_OSC_RECEIVE_PORT 9001
```

## Safe public defaults

The checked-in loader conditionally imports the private file and supplies inert defaults when it is absent:

```cpp
#pragma once

#include "AppTypes.h"

#if __has_include("Provisioning.local.h")
#include "Provisioning.local.h"
#endif

#ifndef APP_PROVISIONING_REVISION
#define APP_PROVISIONING_REVISION 0
#endif
#ifndef APP_WIFI_SSID
#define APP_WIFI_SSID ""
#endif
#ifndef APP_WIFI_PASSWORD
#define APP_WIFI_PASSWORD ""
#endif
#ifndef APP_OSC_TARGET
#define APP_OSC_TARGET "192.168.1.2"
#endif
#ifndef APP_OSC_SEND_PORT
#define APP_OSC_SEND_PORT 9000
#endif
#ifndef APP_OSC_RECEIVE_PORT
#define APP_OSC_RECEIVE_PORT 9001
#endif
```

Provisioning is available only when the revision is nonzero and the SSID is nonempty. An empty password remains valid for an open network.

```cpp
inline bool provisioningAvailable() {
  return APP_PROVISIONING_REVISION > 0 &&
         String(APP_WIFI_SSID).length() > 0;
}
```

When applying values, validate ports before narrowing them to `uint16_t`. Adapt the field names to the Micro Apps settings structure.

## Preferences revision

Store the last successfully applied revision in the same Preferences namespace as the settings:

```cpp
uint32_t provisioningRevision() {
  return preferences.getULong("prov_rev", 0);
}

bool saveProvisioningRevision(uint32_t revision) {
  return preferences.putULong("prov_rev", revision) == sizeof(uint32_t);
}
```

The revision must be written only after all provisioned settings have been saved successfully. Otherwise, a partial settings write could be incorrectly marked complete.

## Boot sequence

Apply provisioning after loading Preferences but before starting the UI, Wi-Fi, OSC, mDNS, or other transports:

```cpp
const bool configReady = configStore.begin();
if (configReady) configStore.load(settings);

const uint32_t storedRevision =
    configReady ? configStore.provisioningRevision() : 0;
const bool shouldApply =
    provisioningAvailable() &&
    APP_PROVISIONING_REVISION > storedRevision;

if (shouldApply) {
  settings.wifiSsid = APP_WIFI_SSID;
  settings.wifiPassword = APP_WIFI_PASSWORD;
  settings.oscTarget = APP_OSC_TARGET;
  settings.oscSendPort = validatedSendPort;
  settings.oscReceivePort = validatedReceivePort;
}

generateUniqueDeviceNameIfRequired(settings);

if (configReady && shouldApply && configStore.save(settings)) {
  configStore.saveProvisioningRevision(APP_PROVISIONING_REVISION);
}
```

After this block, initialize every subsystem from the resulting `settings` object. Never start Wi-Fi using old settings and then apply provisioning later.

## Unique device identity

Do not put a fixed device name in the shared provisioning file. Derive a deterministic suffix from each ESP32-S3 instead:

```cpp
char uniqueName[16];
const uint16_t suffix = static_cast<uint16_t>(ESP.getEfuseMac());
snprintf(uniqueName, sizeof(uniqueName), "device-%04x", suffix);
```

Only replace an unset or known legacy default name. Preserve a custom name previously chosen by the user. Micro Apps may use its own prefix, but the suffix should remain unique so mDNS names and device-owned OSC paths do not collide.

## Revision rules

| Compiled revision | Stored revision | Result |
| --- | --- | --- |
| `0` or no private file | any | Do nothing |
| `1` | `0` | Apply and store revision 1 |
| `1` | `1` | Preserve saved settings |
| `2` | `1` | Reapply and store revision 2 |
| lower than stored | higher | Preserve saved settings |

Do not increment the revision for an ordinary firmware update. Increment it only when the shared network or OSC destination must intentionally replace the saved values across the fleet.

## Security requirements

- Never commit `Provisioning.local.h`.
- Never print the SSID password to Serial or show it in a browser response.
- Keep provisioned `.bin` and merged flash images private; the password is embedded in them.
- Preferences/NVS is not encrypted by default, so physical access to a board may expose saved credentials.
- Do not include real credentials in build logs, CI variables visible to untrusted jobs, release artifacts, or GitHub Actions output.
- For untrusted or commercial deployment, use ESP32 flash encryption, secure boot, and a proper per-device provisioning flow instead of a shared embedded password.

## Porting checklist for Micro Apps

1. Identify the Micro Apps settings structure and Preferences namespace.
2. Map its Wi-Fi and OSC fields into the loader.
3. Add the `prov_rev` Preferences key.
4. Insert the provisioning check before Wi-Fi and transports start.
5. Preserve Micro Apps' existing MAC-derived identity behavior.
6. Add the private header to `.gitignore` before creating it.
7. Compile once without the private file and once with test-only dummy values.
8. Flash a blank test board and confirm revision application without logging secrets.
9. Change a setting on-device, flash the same revision, and confirm it is preserved.
10. Increase the test revision and confirm the shared values are reapplied.

The Parser Mini reference implementation is in `firmware/Provisioning.h`, `firmware/Provisioning.local.h.example`, `firmware/ConfigStore.*`, and the opening portion of `setup()` in `firmware/firmware.ino`.
