// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "AppTypes.h"
#include "ConfigStore.h"
#include "LayoutServer.h"
#include "OscTransport.h"
#include "UserInterface.h"

namespace {
DeviceSettings settings;
ControlState unusedControls;
ImuFrame unusedImu;
const float unusedSpectrum[32] = {};

ConfigStore configStore;
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
      case UiEventType::DeviceName:
        settings.deviceName = event.text[0];
        configStore.save(settings);
        restartAtMs = millis() + 700;
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

  if (configStore.begin()) configStore.load(settings);
  char uniqueName[16];
  char legacyName[20];
  const uint16_t uniqueSuffix = static_cast<uint16_t>(ESP.getEfuseMac());
  snprintf(uniqueName, sizeof(uniqueName), "device-%04x", uniqueSuffix);
  snprintf(legacyName, sizeof(legacyName), "classroom-%04x", uniqueSuffix);
  if (settings.deviceName == "device-0000" || settings.deviceName == "classroom-01" ||
      settings.deviceName == legacyName) {
    settings.deviceName = uniqueName;
    configStore.save(settings);
  }
  const bool uiReady = ui.begin(settings);
  layoutServer.begin(ui.touchOscLayout(), settings, configStore,
                     applyBrowserOscSettings, applyBrowserLayoutChange);
  beginWifi();
  osc.begin(settings, applyRemoteMessage);

  // A new device opens the fully local network picker; no setup hotspot is created.
  if (settings.wifiSsid.isEmpty()) ui.openWifiSetup(true);
  Serial.printf("TouchOSC Parser Mini boot touch=%s device=%s layout=%s\n",
                uiReady ? "ok" : "failed", settings.deviceName.c_str(),
                ui.touchOscLayoutActive() ? "active" : "none");
}

void loop() {
  osc.setLayoutMode(ui.touchOscLayoutActive());
  osc.loop();
  serviceMdns();
  layoutServer.loop(WiFi.status() == WL_CONNECTED);

  ui.loop(unusedControls, unusedImu, 0.0f, unusedSpectrum, false,
          WiFi.status() == WL_CONNECTED, false, false, false);
  handleUiEvents();
  TouchOscEvent touchOscEvent;
  while (ui.popTouchOscEvent(touchOscEvent)) {
    osc.sendLayout(touchOscEvent.address, touchOscEvent.values,
                   touchOscEvent.valueCount);
  }

  if (restartAtMs != 0 && static_cast<int32_t>(millis() - restartAtMs) >= 0) ESP.restart();

  delay(1);
}
