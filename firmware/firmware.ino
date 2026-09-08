// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "AppTypes.h"
#include "ConfigStore.h"
#include "DefaultLayout.h"
#include "ImuService.h"
#include "LayoutServer.h"
#include "OscTransport.h"
#include "Provisioning.h"
#include "UserInterface.h"

namespace {
DeviceSettings settings;
ControlState unusedControls;
ImuFrame imuFrame;
const float unusedSpectrum[32] = {};

ConfigStore configStore;
ImuService imu;
OscTransport osc;
UserInterface ui;
LayoutServer layoutServer;

bool mdnsStarted = false;
uint32_t restartAtMs = 0;

void applyRemoteMessage(const RemoteMessage &message, InputSource) {
  switch (message.type) {
    case MessageType::Layout:
      if (!ui.applyTouchOscRemote(message.address, message.values,
                                  message.valueCount)) return;
      break;
    case MessageType::LayoutText:
      if (!ui.applyTouchOscRemoteText(message.address, message.textValue)) return;
      break;
    default:
      return;
  }
  // Remote changes redraw and wake the device, but are never forwarded to either transport.
  ui.wake();
}

void handleUiEvents() {
  UiEvent event;
  while (ui.popEvent(event)) {
    switch (event.type) {
      case UiEventType::WifiCredentials:
        settings.wifiSsid = event.text[0];
        settings.wifiPassword = event.text[1];
        configStore.save(settings);
        WiFi.disconnect(false, false);
        WiFi.setAutoReconnect(true);
        WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
        ui.wake();
        break;
      case UiEventType::OscSettings:
        settings.oscTarget = event.text[0];
        settings.oscSendPort = event.numbers[0];
        settings.oscReceivePort = event.numbers[1];
        configStore.save(settings);
        osc.reconfigure(settings);
        if (mdnsStarted) {
          MDNS.end();
          mdnsStarted = false;
        }
        ui.wake();
        break;
      case UiEventType::ToggleOscDeviceName:
        settings.oscIncludeDeviceName = event.state;
        configStore.save(settings);
        osc.reconfigure(settings);
        ui.wake();
        break;
      case UiEventType::DeviceName:
        settings.deviceName = event.text[0];
        configStore.save(settings);
        restartAtMs = millis() + 700;
        break;
      case UiEventType::ToggleImuOutput:
        settings.imuOutputEnabled = !settings.imuOutputEnabled;
        configStore.save(settings);
        ui.wake();
        break;
      default:
        break;
    }
  }
}

void beginWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(settings.deviceName.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  if (!settings.wifiSsid.isEmpty()) {
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
  }
}

void applyBrowserOscSettings() {
  osc.reconfigure(settings);
  ui.setOscIncludeDeviceName(settings.oscIncludeDeviceName);
  if (mdnsStarted) {
    MDNS.end();
    mdnsStarted = false;
  }
  ui.wake();
}

void applyBrowserLayoutChange() {
  ui.forceRedraw();
  ui.wake();
}

void serviceMdns() {
  if (WiFi.status() == WL_CONNECTED && !mdnsStarted) {
    mdnsStarted = MDNS.begin(settings.deviceName.c_str());
    if (mdnsStarted) {
      MDNS.addService("osc", "udp", settings.oscReceivePort);
      MDNS.addService("http", "tcp", 80);
    }
  } else if (WiFi.status() != WL_CONNECTED && mdnsStarted) {
    MDNS.end();
    mdnsStarted = false;
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);

  const bool configReady = configStore.begin();
  if (configReady) configStore.load(settings);
  const uint32_t storedProvisioningRevision =
      configReady ? configStore.provisioningRevision() : 0;
  const bool applyProvisioning =
      Provisioning::available() &&
      Provisioning::revision() > storedProvisioningRevision;
  if (applyProvisioning) Provisioning::apply(settings);

  const String uniqueName = hardwareDeviceName();
  char correctedLegacyName[20];
  char buggyDeviceName[16];
  char buggyLegacyName[20];
  const uint16_t buggySuffix = static_cast<uint16_t>(ESP.getEfuseMac());
  snprintf(correctedLegacyName, sizeof(correctedLegacyName), "classroom-%04x",
           hardwareDeviceSuffix());
  snprintf(buggyDeviceName, sizeof(buggyDeviceName), "device-%04x", buggySuffix);
  snprintf(buggyLegacyName, sizeof(buggyLegacyName), "classroom-%04x", buggySuffix);
  bool generatedUniqueName = false;
  if (settings.deviceName == "device-0000" || settings.deviceName == "classroom-01" ||
      settings.deviceName == correctedLegacyName || settings.deviceName == buggyDeviceName ||
      settings.deviceName == buggyLegacyName) {
    settings.deviceName = uniqueName;
    generatedUniqueName = true;
  }
  if (configReady && (applyProvisioning || generatedUniqueName)) {
    const bool settingsSaved = configStore.save(settings);
    if (settingsSaved && applyProvisioning) {
      configStore.saveProvisioningRevision(Provisioning::revision());
      Serial.printf("local provisioning applied revision=%lu\n",
                    static_cast<unsigned long>(Provisioning::revision()));
    }
  }
  const bool uiReady = ui.begin(settings);
  const uint32_t storedLayoutRevision =
      configReady ? configStore.defaultLayoutRevision() : 0;
  const bool applyDefaultLayout =
      DefaultLayout::available() &&
      DefaultLayout::revision() > storedLayoutRevision;
  if (applyDefaultLayout) {
    if (ui.touchOscLayout().installEmbedded(DefaultLayout::data(),
                                            DefaultLayout::size())) {
      if (configReady) {
        configStore.saveDefaultLayoutRevision(DefaultLayout::revision());
      }
      ui.forceRedraw();
      Serial.printf("private default layout applied revision=%lu pages=%u controls=%u\n",
                    static_cast<unsigned long>(DefaultLayout::revision()),
                    ui.touchOscLayout().pageCount(),
                    ui.touchOscLayout().controlCount());
    } else {
      Serial.printf("private default layout failed: %s\n",
                    ui.touchOscLayout().lastError().c_str());
    }
  }
  const bool imuReady = imu.begin(settings);
  layoutServer.begin(ui.touchOscLayout(), settings, configStore,
                     applyBrowserOscSettings, applyBrowserLayoutChange);
  beginWifi();
  osc.begin(settings, applyRemoteMessage);

  // A new device opens the fully local network picker; no setup hotspot is created.
  if (settings.wifiSsid.isEmpty()) ui.openWifiSetup(true);
  Serial.printf("TouchOSC Parser Mini boot touch=%s imu=%s device=%s layout=%s\n",
                uiReady ? "ok" : "failed", imuReady ? "ok" : "failed",
                settings.deviceName.c_str(),
                ui.touchOscLayoutActive() ? "active" : "none");
}

void loop() {
  osc.setLayoutMode(ui.touchOscLayoutActive());
  osc.loop();
  serviceMdns();
  layoutServer.loop(WiFi.status() == WL_CONNECTED);

  if (imu.update(imuFrame) && settings.imuOutputEnabled) {
    osc.sendImu(imuFrame);
  }

  ui.loop(unusedControls, imuFrame, 0.0f, unusedSpectrum, false,
          WiFi.status() == WL_CONNECTED, false, false,
          settings.imuOutputEnabled);
  handleUiEvents();
  TouchOscEvent touchOscEvent;
  while (ui.popTouchOscEvent(touchOscEvent)) {
    osc.sendLayout(touchOscEvent.address, touchOscEvent.values,
                   touchOscEvent.valueCount);
  }

  if (restartAtMs != 0 && static_cast<int32_t>(millis() - restartAtMs) >= 0) ESP.restart();

  delay(1);
}
