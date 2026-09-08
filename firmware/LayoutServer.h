// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include <FFat.h>
#include <WebServer.h>

#include "AppTypes.h"
#include "ConfigStore.h"
#include "TouchOscLayout.h"

using LayoutSettingsHandler = void (*)();
using LayoutChangeHandler = void (*)();

class LayoutServer {
 public:
  void begin(TouchOscLayout &layout, DeviceSettings &settings,
             ConfigStore &store, LayoutSettingsHandler settingsHandler,
             LayoutChangeHandler changeHandler);
  void loop(bool wifiConnected);

 private:
  void configureRoutes();
  void handleRoot();
  void handleUploadData();
  void handleUploadComplete();
  void handleRemove();
  void handleSettings();
  void handleOscSave();

  WebServer server_{80};
  File uploadFile_;
  TouchOscLayout *layout_ = nullptr;
  DeviceSettings *settings_ = nullptr;
  ConfigStore *store_ = nullptr;
  LayoutSettingsHandler settingsHandler_ = nullptr;
  LayoutChangeHandler changeHandler_ = nullptr;
  bool configured_ = false;
  bool running_ = false;
  bool uploadFailed_ = false;
  size_t uploadBytes_ = 0;
};
