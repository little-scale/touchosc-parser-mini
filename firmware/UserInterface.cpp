// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include "UserInterface.h"

#include <Wire.h>
#include <WiFi.h>

#include "pin_config.h"

namespace {
constexpr int16_t kScreenWidth = 368;
constexpr int16_t kScreenHeight = 448;
constexpr int16_t kTopHeight = 43;
constexpr int16_t kTopCanvasHeight = kTopHeight + 1;
constexpr int16_t kStatusBadgeShiftX = 10;
constexpr int16_t kControlX = 20;
constexpr int16_t kControlY = 57;
constexpr int16_t kControlW = 328;
constexpr int16_t kControlH = 310;
// CO5300 region transfers are expanded to even starts and odd inclusive ends.
// These invisible padding rows preserve the visible layout while making the
// whole control canvas conform to that boundary requirement.
constexpr int16_t kControlCanvasY = kControlY - 1;
constexpr int16_t kControlCanvasH = kControlH + 2;
constexpr int16_t kControlCanvasInsetY = kControlY - kControlCanvasY;
constexpr int16_t kPageX = 20;
constexpr int16_t kPageY = 385;
constexpr int16_t kPageW = 328;
constexpr int16_t kPageH = 49;
constexpr uint8_t kControlPageCount = 8;
constexpr uint8_t kPageKeyboard = 0;
constexpr uint8_t kPageButtons = 1;
constexpr uint8_t kPageXy = 2;
constexpr uint8_t kPageFaders = 3;
constexpr uint8_t kPageBalls = 4;
constexpr uint8_t kPagePendulums = 5;
constexpr uint8_t kPageParticles = 6;
constexpr uint8_t kPageSpectrum = 7;
constexpr int16_t kControlGap = 12;
constexpr int16_t kButtonSize = 143;
constexpr int16_t kButtonBorder = 25;
constexpr int16_t kButtonInsetX = (kControlW - 2 * kButtonSize - kControlGap) / 2;
constexpr int16_t kButtonInsetY = (kControlH - 2 * kButtonSize - kControlGap) / 2;
constexpr int16_t kBallArenaSize = 310;
constexpr int16_t kBallArenaX = (kControlW - kBallArenaSize) / 2;
constexpr int16_t kBallArenaY = 0;
constexpr int16_t kBallWallWidth = 25;
constexpr int16_t kBallRadius = 13;
constexpr int16_t kBallTapRadius = 21;
constexpr float kBallMaxVelocity = 2.1f;
constexpr float kMinimumBallGravity = 0.5f;
constexpr float kMaximumBallGravity = 4.0f;
constexpr float kBallGravityStep = 0.1f;
constexpr float kMinimumBallBounciness = 0.0f;
constexpr float kMaximumBallBounciness = 1.0f;
constexpr float kBallBouncinessStep = 0.05f;
constexpr int16_t kPendulumPivotSize = 25;
constexpr int16_t kPendulumBobRadius = 13;
constexpr int16_t kPendulumPivotTouchRadius = 21;
constexpr int16_t kPendulumBobTouchRadius = 24;
constexpr float kPendulumMinimumLength = 40.0f;
constexpr float kPendulumMaximumLength = 260.0f;
constexpr float kPendulumGravityPixelScale = 230.0f;
constexpr float kPendulumDamping = 0.35f;
constexpr float kPendulumMaximumAngularVelocity = 10.0f;
constexpr float kPendulumDirectionThreshold = 0.08f;
constexpr float kPendulumCentreSpeedThreshold = 0.15f;
constexpr float kPendulumGravityThreshold = 0.08f;
constexpr uint32_t kPendulumStreamIntervalMs = 40;
constexpr uint32_t kPendulumPingCooldownMs = 120;
constexpr float kParticleMaximumSpawnRate = 48.0f;
constexpr float kParticleMinimumSpeed = 65.0f;
constexpr float kParticleMaximumSpeed = 185.0f;
constexpr float kParticleTiltAcceleration = 145.0f;
constexpr float kParticleMaximumVelocity = 280.0f;
constexpr float kParticleMinimumLifetime = 9.0f;
constexpr float kParticleLifetimeRange = 3.0f;
constexpr uint8_t kParticleMinimumSize = 3;
constexpr uint8_t kParticleMaximumSize = 8;
constexpr uint32_t kParticleDrawIntervalUs = 33333;
constexpr int16_t kLineWidth = 5;
constexpr float kPi = 3.14159265358979323846f;
constexpr uint32_t kStreamIntervalMs = 20;
constexpr uint32_t kLongPressMs = 1200;
constexpr uint32_t kResetHoldMs = 2500;
constexpr uint8_t kAwakeBrightness = 210;
constexpr uint8_t kDimBrightness = 28;
constexpr int16_t kControlInset = 12;
constexpr int16_t kXyCrossWidth = 25;
constexpr int16_t kKeyboardWhiteKeyWidth = kControlW / 8;
constexpr int16_t kKeyboardHeight = 222;
constexpr int16_t kKeyboardBlackKeyWidth = 25;
constexpr int16_t kKeyboardBlackKeyHeight = 125;
constexpr int16_t kKeyboardOctaveY = 242;
constexpr int16_t kKeyboardOctaveHeight = 48;
constexpr int16_t kKeyboardOctaveGap = 12;
constexpr int16_t kKeyboardOctaveWidth =
    (kControlW - kKeyboardOctaveGap) / 2;
constexpr uint8_t kKeyboardMinimumBaseMidiNote = 0;
constexpr uint8_t kKeyboardMaximumBaseMidiNote = 108;
constexpr uint8_t kKeyboardWhiteNotes[8] = {0, 2, 4, 5, 7, 9, 11, 12};
constexpr uint8_t kKeyboardBlackNotes[5] = {1, 3, 6, 8, 10};
constexpr uint8_t kKeyboardBlackBoundaries[5] = {1, 2, 4, 5, 6};
constexpr uint8_t kSpectrumBandCount = 32;
constexpr int16_t kSpectrumHeight = 230;
constexpr int16_t kSpectrumCaptureY = 242;
constexpr int16_t kSpectrumCaptureHeight = 48;
constexpr int16_t kSpectrumCaptureBorder = 10;
constexpr uint32_t kWifiSetupTimeoutMs = 150000;
constexpr uint32_t kWifiScanTimeoutMs = 12000;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint8_t kWifiRowsPerPage = 5;
constexpr int16_t kWifiListButtonY = 366;
constexpr int16_t kWifiListButtonH = 68;
constexpr int16_t kWifiListTouchY = 350;

void IRAM_ATTR touchInterrupt() {
  // The touch driver owns the interrupt flag; polling finger count keeps release handling reliable.
}

bool inside(int16_t x, int16_t y, int16_t left, int16_t top, int16_t width, int16_t height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

bool different(float a, float b) {
  return fabsf(a - b) > 0.001f;
}

float wrapRadians(float value) {
  while (value > kPi) value -= 2.0f * kPi;
  while (value < -kPi) value += 2.0f * kPi;
  return value;
}

void drawThickRoundRect(Arduino_GFX *target, int16_t x, int16_t y, int16_t width,
                        int16_t height, int16_t radius, uint16_t color) {
  for (int16_t inset = 0; inset < kLineWidth; ++inset) {
    const int16_t insetRadius = radius - inset > 1 ? radius - inset : 1;
    target->drawRoundRect(x + inset, y + inset, width - 2 * inset,
                          height - 2 * inset, insetRadius, color);
  }
}

void drawWifiGlyph(Arduino_GFX *target, int16_t centerX, int16_t baselineY,
                   uint16_t color) {
  constexpr uint8_t kSegments = 12;
  for (const int16_t radius : {7, 12}) {
    int16_t previousX = centerX - radius;
    int16_t previousY = baselineY;
    for (uint8_t segment = 1; segment <= kSegments; ++segment) {
      const float angle = kPi - (kPi * segment) / kSegments;
      const int16_t nextX = centerX + lroundf(cosf(angle) * radius);
      const int16_t nextY = baselineY - lroundf(sinf(angle) * radius);
      target->drawLine(previousX, previousY, nextX, nextY, color);
      target->drawLine(previousX, previousY + 1, nextX, nextY + 1, color);
      previousX = nextX;
      previousY = nextY;
    }
  }
  target->fillCircle(centerX, baselineY + 1, 3, color);
}

void drawCenteredText(Arduino_GFX *target, const String &text, int16_t x, int16_t y,
                      int16_t width, uint8_t size, uint16_t color) {
  target->setTextSize(size);
  target->setTextColor(color);
  target->setTextWrap(false);
  const int16_t textWidth = text.length() * 6 * size;
  const int16_t inset = width > textWidth ? (width - textWidth) / 2 : 0;
  target->setCursor(x + inset, y);
  target->print(text);
}

void drawSetupButton(Arduino_GFX *target, int16_t x, int16_t y, int16_t width,
                     int16_t height, const String &label, uint16_t fill,
                     uint16_t border, uint16_t text) {
  target->fillRoundRect(x, y, width, height, 6, fill);
  drawThickRoundRect(target, x, y, width, height, 6, border);
  drawCenteredText(target, label, x, y + (height - 16) / 2, width, 2, text);
}
}  // namespace

bool UserInterface::begin(const DeviceSettings &settings) {
  dimAfterMs_ = settings.dimAfterMs;
  oscTargetText_ = settings.oscTarget;
  oscSendPortText_ = String(settings.oscSendPort);
  oscReceivePortText_ = String(settings.oscReceivePort);
  deviceNameText_ = settings.deviceName;
  savedDeviceName_ = settings.deviceName;
  ballGravity_ = constrain(settings.ballGravity, kMinimumBallGravity,
                           kMaximumBallGravity);
  ballBounciness_ = constrain(settings.ballBounciness,
                              kMinimumBallBounciness,
                              kMaximumBallBounciness);
  physicsGravityEdit_ = ballGravity_;
  physicsBouncinessEdit_ = ballBounciness_;
  char generatedName[16];
  snprintf(generatedName, sizeof(generatedName), "device-%04x",
           static_cast<uint16_t>(ESP.getEfuseMac()));
  defaultDeviceName_ = generatedName;
  resetBalls();
  Wire.begin(IIC_SDA, IIC_SCL);

  if (expander_.begin(0x20, &Wire)) {
    for (uint8_t pin : {0, 1, 2, 6}) {
      expander_.pinMode(pin, OUTPUT);
      expander_.digitalWrite(pin, LOW);
    }
    delay(20);
    for (uint8_t pin : {0, 1, 2, 6}) expander_.digitalWrite(pin, HIGH);
  }

  displayBus_ = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1,
                                      LCD_SDIO2, LCD_SDIO3);
  display_ = new Arduino_CO5300(displayBus_, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT,
                                16, 0, 0, 0);
  if (!display_->begin()) return false;
  display_->setBrightness(kAwakeBrightness);

  // Render changing regions into PSRAM, then transfer each as one image.  Keep
  // the update bounds even-aligned for the CO5300 controller.
  topCanvas_ = new Arduino_Canvas(kScreenWidth, kTopCanvasHeight, display_, 0, 0);
  xyCanvas_ = new Arduino_Canvas(kControlW, kControlCanvasH, display_,
                                 kControlX, kControlCanvasY);
  setupCanvas_ = new Arduino_Canvas(kScreenWidth, kScreenHeight, display_, 0, 0);
  const bool canvasesReady = topCanvas_->begin(GFX_SKIP_OUTPUT_BEGIN) &&
                             xyCanvas_->begin(GFX_SKIP_OUTPUT_BEGIN) &&
                             setupCanvas_->begin(GFX_SKIP_OUTPUT_BEGIN);
  if (!canvasesReady) {
    delete topCanvas_;
    delete xyCanvas_;
    delete setupCanvas_;
    topCanvas_ = nullptr;
    xyCanvas_ = nullptr;
    setupCanvas_ = nullptr;
  } else {
    layoutTransferBuffer_ = static_cast<uint16_t *>(
        ps_malloc(kScreenWidth * kScreenHeight * sizeof(uint16_t)));
  }
  Serial.printf("display canvases=%s layout_partial=%s free_psram=%u\n",
                canvasesReady ? "ok" : "failed",
                layoutTransferBuffer_ ? "ok" : "fallback",
                ESP.getFreePsram());

  i2cBus_ = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
  touch_.reset(new Arduino_CST816x(i2cBus_, CST816T_DEVICE_ADDRESS,
                                   DRIVEBUS_DEFAULT_VALUE, TP_INT, touchInterrupt));
  // The CST816 occasionally wakes a little later than the display after a USB
  // flash/reset. A single failed probe used to disable touch for the whole boot.
  // Give it several chances here, then keep a low-rate recovery path in loop().
  initializeTouch(4);

  powerReady_ = power_.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (powerReady_) {
    power_.enableBattDetection();
    power_.enableBattVoltageMeasure();
  }
  touchOscLayout_.begin();
  lastActivityMs_ = millis();
  forceRedraw();
  return touchReady_;
}

bool UserInterface::initializeTouch(uint8_t attempts) {
  if (!touch_) return false;
  for (uint8_t attempt = 0; attempt < attempts; ++attempt) {
    if (touch_->begin()) {
      touchReady_ = true;
      touch_->IIC_Write_Device_State(
          touch_->Arduino_IIC_Touch::Device::TOUCH_DEVICE_INTERRUPT_MODE,
          touch_->Arduino_IIC_Touch::Device_Mode::TOUCH_DEVICE_INTERRUPT_PERIODIC);
      Serial.printf("touch initialized attempt=%u\n", attempt + 1);
      return true;
    }
    delay(75);
  }
  touchReady_ = false;
  return false;
}

void UserInterface::serviceTouchRecovery() {
  if (touchReady_ || millis() - lastTouchRecoveryMs_ < 2000) return;
  lastTouchRecoveryMs_ = millis();
  if (initializeTouch(1)) Serial.println("touch recovered");
}

void UserInterface::forceRedraw() {
  hasDrawn_ = false;
  topStateValid_ = false;
  layoutOverlayValid_ = false;
  xyPartialValid_ = false;
  keyboardPartialValid_ = false;
  previousBackground_[0] = 255;
  previousBackground_[1] = 255;
  previousBackground_[2] = 255;
  touchOscLayout_.forceRedraw();
}

uint16_t UserInterface::rgb565(uint8_t r, uint8_t g, uint8_t b) const {
  return static_cast<uint16_t>(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

uint16_t UserInterface::mixColor(const uint8_t background[3], uint8_t foreground,
                                 uint8_t amount) const {
  const uint8_t inverse = 255 - amount;
  return rgb565((background[0] * inverse + foreground * amount) / 255,
                (background[1] * inverse + foreground * amount) / 255,
                (background[2] * inverse + foreground * amount) / 255);
}

void UserInterface::updatePalette(const ControlState &state) {
  backgroundColor_ = rgb565(state.background[0], state.background[1], state.background[2]);
  const uint16_t luminance = (state.background[0] * 54 + state.background[1] * 183 +
                              state.background[2] * 19) >> 8;
  const uint8_t foreground = luminance > 145 ? 0 : 255;
  foregroundColor_ = rgb565(foreground, foreground, foreground);
  mutedColor_ = mixColor(state.background, foreground, 105);
  panelColor_ = mixColor(state.background, foreground, 22);
}

void UserInterface::updatePowerStatus() {
  if (!powerReady_ || millis() - lastBatteryReadMs_ < 5000) return;
  lastBatteryReadMs_ = millis();
  batteryPercent_ = power_.getBatteryPercent();
  charging_ = power_.isCharging();
}

void UserInterface::openWifiSetup(bool openNetworkPicker) {
  if (!display_ || wifiSetupActive_) return;
  wifiSetupActive_ = true;
  lastBallUpdateUs_ = 0;
  wifiSetupTouching_ = false;
  wifiSetupLastActivityMs_ = millis();
  wifiPasswordError_ = false;
  wake();
  if (openNetworkPicker) {
    startWifiScan();
  } else {
    wifiSetupPage_ = WifiSetupPage::Menu;
    drawWifiSetup();
  }
}

void UserInterface::closeWifiSetup() {
  if (!wifiSetupActive_) return;
  WiFi.scanDelete();
  wifiScanPending_ = false;
  wifiSetupActive_ = false;
  wifiSetupTouching_ = false;
  // Settings buttons act on touch-down. Do not allow the same physical touch
  // to become a layout gesture on the following frame. Also discard any stale
  // capture left over from the control that originally opened settings.
  touchOscLayout_.endTouch();
  touching_ = false;
  layoutWifiTouch_ = false;
  layoutPageTouch_ = false;
  activeTouch_ = TouchTarget::None;
  ignoreTouchUntilRelease_ = true;
  ignoreTouchStartedMs_ = millis();
  forceRedraw();
}

void UserInterface::startWifiScan() {
  WiFi.scanDelete();
  WiFi.mode(WIFI_STA);
  wifiNetworkCount_ = 0;
  wifiNetworkPage_ = 0;
  wifiSetupPage_ = WifiSetupPage::Scanning;
  wifiScanStartedMs_ = millis();
  const int16_t scanStart = WiFi.scanNetworks(true, true);
  wifiScanPending_ = scanStart == WIFI_SCAN_RUNNING || scanStart >= 0;
  wifiScanRetryAtMs_ = millis() + 300;
  drawWifiSetup();
}

void UserInterface::serviceWifiSetup() {
  if (!wifiSetupActive_) return;
  if (millis() - wifiSetupLastActivityMs_ >= kWifiSetupTimeoutMs) {
    closeWifiSetup();
    return;
  }

  if (wifiSetupPage_ == WifiSetupPage::Scanning && !wifiScanPending_) {
    if (millis() - wifiScanStartedMs_ >= kWifiScanTimeoutMs) {
      wifiSetupPage_ = WifiSetupPage::Networks;
      drawWifiSetup();
    } else if (static_cast<int32_t>(millis() - wifiScanRetryAtMs_) >= 0) {
      WiFi.scanDelete();
      const int16_t scanStart = WiFi.scanNetworks(true, true);
      wifiScanPending_ = scanStart == WIFI_SCAN_RUNNING || scanStart >= 0;
      wifiScanRetryAtMs_ = millis() + 500;
    }
  } else if (wifiSetupPage_ == WifiSetupPage::Scanning && wifiScanPending_) {
    const int16_t result = WiFi.scanComplete();
    if (result >= 0) {
      wifiNetworkCount_ = 0;
      for (int16_t scanIndex = 0; scanIndex < result; ++scanIndex) {
        const String ssid = WiFi.SSID(scanIndex);
        if (ssid.isEmpty()) continue;
        const int32_t rssi = WiFi.RSSI(scanIndex);
        int16_t existing = -1;
        for (uint8_t i = 0; i < wifiNetworkCount_; ++i) {
          if (wifiNetworks_[i] == ssid) {
            existing = i;
            break;
          }
        }
        if (existing >= 0) {
          if (rssi > wifiRssi_[existing]) wifiRssi_[existing] = rssi;
          continue;
        }

        uint8_t position = wifiNetworkCount_;
        if (position < kMaxWifiNetworks) {
          ++wifiNetworkCount_;
        } else {
          if (rssi <= wifiRssi_[kMaxWifiNetworks - 1]) continue;
          position = kMaxWifiNetworks - 1;
        }
        while (position > 0 && rssi > wifiRssi_[position - 1]) {
          if (position < kMaxWifiNetworks) {
            wifiNetworks_[position] = wifiNetworks_[position - 1];
            wifiRssi_[position] = wifiRssi_[position - 1];
            wifiSecure_[position] = wifiSecure_[position - 1];
          }
          --position;
        }
        if (position < kMaxWifiNetworks) {
          wifiNetworks_[position] = ssid;
          wifiRssi_[position] = rssi;
          wifiSecure_[position] = WiFi.encryptionType(scanIndex) != WIFI_AUTH_OPEN;
        }
      }
      WiFi.scanDelete();
      wifiScanPending_ = false;
      wifiSetupPage_ = WifiSetupPage::Networks;
      drawWifiSetup();
    } else if (result != WIFI_SCAN_RUNNING ||
               millis() - wifiScanStartedMs_ >= kWifiScanTimeoutMs) {
      WiFi.scanDelete();
      wifiScanPending_ = false;
      wifiSetupPage_ = WifiSetupPage::Networks;
      drawWifiSetup();
    }
  }

  if (wifiSetupPage_ == WifiSetupPage::Connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiSetupPage_ = WifiSetupPage::Connected;
      wifiConnectedShownMs_ = millis();
      drawWifiSetup();
    } else if (millis() - wifiConnectStartedMs_ >= kWifiConnectTimeoutMs) {
      WiFi.disconnect(false, false);
      wifiPasswordError_ = true;
      wifiSetupPage_ = WifiSetupPage::Password;
      drawWifiSetup();
    }
  } else if (wifiSetupPage_ == WifiSetupPage::Connected &&
             millis() - wifiConnectedShownMs_ >= 900) {
    closeWifiSetup();
  } else if (wifiSetupPage_ == WifiSetupPage::Saved &&
             millis() - wifiConnectedShownMs_ >= 900) {
    closeWifiSetup();
  }
}

void UserInterface::readWifiSetupTouch() {
  if (!touchReady_ || millis() - lastTouchPollMs_ < 8) return;
  lastTouchPollMs_ = millis();
  bool pressed = false;
  int16_t x = 0;
  int16_t y = 0;
  if (!readTouchPoint(pressed, x, y)) return;
  if (pressed && !wifiSetupTouching_) {
    wifiSetupTouching_ = true;
    wifiSetupLastActivityMs_ = millis();
    wake();
    handleWifiSetupTap(x, y);
  } else if (!pressed) {
    wifiSetupTouching_ = false;
  }
}

const char *UserInterface::keyboardRow(uint8_t row) const {
  static const char *lower[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
  static const char *upper[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
  static const char *symbols[] = {"!@#$%^&*()", "-_=+[]{}\\/", ".,:;'\"?~`", "<>|"};
  return wifiSymbols_ ? symbols[row] : (wifiShift_ ? upper[row] : lower[row]);
}

void UserInterface::handleWifiSetupTap(int16_t x, int16_t y) {
  if (wifiSetupPage_ == WifiSetupPage::Scanning) {
    WiFi.scanDelete();
    wifiScanPending_ = false;
    wifiSetupPage_ = WifiSetupPage::Menu;
    drawWifiSetup();
    return;
  }
  if (wifiSetupPage_ == WifiSetupPage::Connecting ||
      wifiSetupPage_ == WifiSetupPage::Connected ||
      wifiSetupPage_ == WifiSetupPage::Saved ||
      wifiSetupPage_ == WifiSetupPage::Restarting) return;

  if (wifiSetupPage_ == WifiSetupPage::Menu) {
    if (inside(x, y, 24, 52, 320, 60)) {
      startWifiScan();
    } else if (inside(x, y, 24, 128, 320, 60)) {
      oscActiveField_ = 0;
      oscInputError_ = false;
      wifiSetupPage_ = WifiSetupPage::Osc;
      drawWifiSetup();
    } else if (inside(x, y, 24, 204, 320, 60)) {
      deviceNameText_ = savedDeviceName_;
      deviceNameInputError_ = false;
      wifiSetupPage_ = WifiSetupPage::Device;
      drawWifiSetup();
    } else if (inside(x, y, 24, 344, 320, 64)) {
      closeWifiSetup();
    }
    return;
  }

  if (wifiSetupPage_ == WifiSetupPage::Networks) {
    if (y >= 56 && y < kWifiListTouchY) {
      const uint8_t row = (y - 56) / 60;
      const uint8_t index = wifiNetworkPage_ * kWifiRowsPerPage + row;
      if (index < wifiNetworkCount_) {
        wifiSelectedSsid_ = wifiNetworks_[index];
        wifiPassword_ = "";
        wifiShift_ = false;
        wifiSymbols_ = false;
        wifiPasswordVisible_ = false;
        wifiPasswordError_ = false;
        wifiSetupPage_ = WifiSetupPage::Password;
        drawWifiSetup();
      }
      return;
    }
    if (y >= kWifiListTouchY) {
      if (x < 124) {
        wifiSetupPage_ = WifiSetupPage::Menu;
        drawWifiSetup();
      }
      else if (x < 246) startWifiScan();
      else {
        const uint8_t calculatedPages =
            (wifiNetworkCount_ + kWifiRowsPerPage - 1) / kWifiRowsPerPage;
        const uint8_t pages = calculatedPages > 0 ? calculatedPages : 1;
        wifiNetworkPage_ = (wifiNetworkPage_ + 1) % pages;
        drawWifiSetup();
      }
    }
    return;
  }

  if (wifiSetupPage_ == WifiSetupPage::Osc) {
    const int16_t fieldY[3] = {30, 78, 126};
    for (uint8_t field = 0; field < 3; ++field) {
      if (inside(x, y, 10, fieldY[field], 348, 42)) {
        oscActiveField_ = field;
        if (x >= 310) {
          if (field == 0) oscTargetText_ = "";
          else if (field == 1) oscSendPortText_ = "";
          else oscReceivePortText_ = "";
        }
        oscInputError_ = false;
        drawWifiSetup();
        return;
      }
    }

    if (y >= 178 && y < 366) {
      const uint8_t row = (y - 178) / 47;
      if ((y - 178) % 47 < 41) {
        const int16_t keyX[3] = {10, 130, 250};
        for (uint8_t col = 0; col < 3; ++col) {
          if (!inside(x, y, keyX[col], 178 + row * 47, 108, 41)) continue;
          static const char keys[] = "123456789.0<";
          const char key = keys[row * 3 + col];
          String *active = oscActiveField_ == 0 ? &oscTargetText_
                            : (oscActiveField_ == 1 ? &oscSendPortText_
                                                    : &oscReceivePortText_);
          if (key == '<') {
            if (!active->isEmpty()) active->remove(active->length() - 1);
          } else if (key != '.' || oscActiveField_ == 0) {
            const size_t limit = oscActiveField_ == 0 ? 15 : 5;
            if (active->length() < limit) *active += key;
          }
          oscInputError_ = false;
          drawWifiSetup();
          return;
        }
      }
    }

    if (y >= 378) {
      if (x < kScreenWidth / 2) {
        wifiSetupPage_ = WifiSetupPage::Menu;
        drawWifiSetup();
      } else if (oscSettingsValid()) {
        UiEvent event;
        event.type = UiEventType::OscSettings;
        event.text[0] = oscTargetText_;
        event.numbers[0] = static_cast<uint16_t>(oscSendPortText_.toInt());
        event.numbers[1] = static_cast<uint16_t>(oscReceivePortText_.toInt());
        enqueue(event);
        wifiSetupPage_ = WifiSetupPage::Saved;
        wifiConnectedShownMs_ = millis();
        drawWifiSetup();
      } else {
        oscInputError_ = true;
        drawWifiSetup();
      }
    }
    return;
  }

  if (wifiSetupPage_ == WifiSetupPage::Device) {
    if (y >= 110 && y < 306) {
      const uint8_t row = (y - 110) / 50;
      if ((y - 110) % 50 < 42) {
        static const char *rows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm-"};
        const char *keys = rows[row];
        const uint8_t count = strlen(keys);
        const int16_t gap = 3;
        const int16_t keyWidth = (kScreenWidth - 16 - (count - 1) * gap) / count;
        for (uint8_t col = 0; col < count; ++col) {
          const int16_t keyX = 8 + col * (keyWidth + gap);
          if (!inside(x, y, keyX, 110 + row * 50, keyWidth, 42)) continue;
          if (deviceNameText_.length() < 24) deviceNameText_ += keys[col];
          deviceNameInputError_ = false;
          drawWifiSetup();
          return;
        }
      }
    }
    if (y >= 314 && y < 360) {
      if (x < 122) {
        deviceNameText_ = "";
      } else if (x < 246) {
        deviceNameText_ = defaultDeviceName_;
      } else if (!deviceNameText_.isEmpty()) {
        deviceNameText_.remove(deviceNameText_.length() - 1);
      }
      deviceNameInputError_ = false;
      drawWifiSetup();
      return;
    }
    if (y >= 376) {
      if (x < kScreenWidth / 2) {
        wifiSetupPage_ = WifiSetupPage::Menu;
        drawWifiSetup();
      } else if (deviceNameValid()) {
        UiEvent event;
        event.type = UiEventType::DeviceName;
        event.text[0] = deviceNameText_;
        enqueue(event);
        wifiSetupPage_ = WifiSetupPage::Restarting;
        drawWifiSetup();
      } else {
        deviceNameInputError_ = true;
        drawWifiSetup();
      }
    }
    return;
  }

  if (wifiSetupPage_ == WifiSetupPage::Physics) {
    if (y >= 125 && y < 189) {
      const float direction = x < kScreenWidth / 2 ? -1.0f : 1.0f;
      physicsGravityEdit_ = constrain(
          roundf((physicsGravityEdit_ + direction * kBallGravityStep) * 10.0f) /
              10.0f,
          kMinimumBallGravity, kMaximumBallGravity);
      drawWifiSetup();
      return;
    }
    if (y >= 292 && y < 356) {
      const float direction = x < kScreenWidth / 2 ? -1.0f : 1.0f;
      physicsBouncinessEdit_ = constrain(
          roundf((physicsBouncinessEdit_ + direction * kBallBouncinessStep) * 100.0f) /
              100.0f,
          kMinimumBallBounciness, kMaximumBallBounciness);
      drawWifiSetup();
      return;
    }
    if (y >= 378) {
      if (x < kScreenWidth / 2) {
        wifiSetupPage_ = WifiSetupPage::Menu;
        drawWifiSetup();
      } else {
        ballGravity_ = physicsGravityEdit_;
        ballBounciness_ = physicsBouncinessEdit_;
        UiEvent event;
        event.type = UiEventType::PhysicsSettings;
        event.values[0] = ballGravity_;
        event.values[1] = ballBounciness_;
        enqueue(event);
        wifiSetupPage_ = WifiSetupPage::Saved;
        wifiConnectedShownMs_ = millis();
        drawWifiSetup();
      }
    }
    return;
  }

  if (wifiSetupPage_ != WifiSetupPage::Password) return;
  if (inside(x, y, 278, 48, 80, 48)) {
    wifiPasswordVisible_ = !wifiPasswordVisible_;
    drawWifiSetup();
    return;
  }
  if (y >= 110 && y < 306) {
    const uint8_t row = (y - 110) / 50;
    if ((y - 110) % 50 >= 42) return;
    const char *keys = keyboardRow(row);
    const uint8_t count = strlen(keys);
    const int16_t gap = 3;
    const int16_t keyWidth = (kScreenWidth - 16 - (count - 1) * gap) / count;
    for (uint8_t col = 0; col < count; ++col) {
      const int16_t keyX = 8 + col * (keyWidth + gap);
      if (inside(x, y, keyX, 110 + row * 50, keyWidth, 42)) {
        if (wifiPassword_.length() < 63) wifiPassword_ += keys[col];
        wifiPasswordError_ = false;
        drawWifiSetup();
        return;
      }
    }
  }
  if (y >= 314 && y < 360) {
    if (x < 90) {
      if (!wifiSymbols_) wifiShift_ = !wifiShift_;
    } else if (x < 180) {
      wifiSymbols_ = !wifiSymbols_;
    } else if (x < 284) {
      if (wifiPassword_.length() < 63) wifiPassword_ += ' ';
    } else if (!wifiPassword_.isEmpty()) {
      wifiPassword_.remove(wifiPassword_.length() - 1);
    }
    wifiPasswordError_ = false;
    drawWifiSetup();
    return;
  }
  if (y >= 376) {
    if (x < kScreenWidth / 2) {
      wifiSetupPage_ = WifiSetupPage::Networks;
      drawWifiSetup();
    } else {
      UiEvent event;
      event.type = UiEventType::WifiCredentials;
      event.text[0] = wifiSelectedSsid_;
      event.text[1] = wifiPassword_;
      enqueue(event);
      wifiSetupPage_ = WifiSetupPage::Connecting;
      wifiConnectStartedMs_ = millis();
      drawWifiSetup();
    }
  }
}

void UserInterface::drawWifiMessage(Arduino_GFX *target, const char *message) {
  drawCenteredText(target, message, 0, 198, kScreenWidth, 2, foregroundColor_);
}

bool UserInterface::oscSettingsValid() const {
  IPAddress address;
  if (!address.fromString(oscTargetText_)) return false;
  const long sendPort = oscSendPortText_.toInt();
  const long receivePort = oscReceivePortText_.toInt();
  return sendPort >= 1 && sendPort <= 65535 &&
         receivePort >= 1 && receivePort <= 65535;
}

bool UserInterface::deviceNameValid() const {
  if (deviceNameText_.isEmpty() || deviceNameText_.length() > 24 ||
      deviceNameText_.startsWith("-") || deviceNameText_.endsWith("-") ||
      deviceNameText_.indexOf("--") >= 0) return false;
  for (size_t i = 0; i < deviceNameText_.length(); ++i) {
    const char value = deviceNameText_[i];
    if (!((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
          value == '-')) return false;
  }
  return true;
}

void UserInterface::drawSettingsMenu(Arduino_GFX *target) {
  drawCenteredText(target, "SETTINGS", 0, 12, kScreenWidth, 2, foregroundColor_);
  drawSetupButton(target, 24, 52, 320, 60, "NETWORK", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 24, 128, 320, 60, "OSC", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 24, 204, 320, 60, "DEVICE", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 24, 344, 320, 64, "EXIT", panelColor_, mutedColor_,
                  foregroundColor_);
}

void UserInterface::drawWifiNetworks(Arduino_GFX *target) {
  drawCenteredText(target, "WI-FI", 0, 18, kScreenWidth, 2, foregroundColor_);
  const uint8_t calculatedPages =
      (wifiNetworkCount_ + kWifiRowsPerPage - 1) / kWifiRowsPerPage;
  const uint8_t pages = calculatedPages > 0 ? calculatedPages : 1;
  if (wifiNetworkCount_ == 0) {
    drawCenteredText(target, "NO NETWORKS", 0, 195, kScreenWidth, 2, mutedColor_);
  }
  for (uint8_t row = 0; row < kWifiRowsPerPage; ++row) {
    const uint8_t index = wifiNetworkPage_ * kWifiRowsPerPage + row;
    if (index >= wifiNetworkCount_) break;
    const int16_t y = 56 + row * 60;
    target->fillRoundRect(10, y, 348, 52, 6, panelColor_);
    drawThickRoundRect(target, 10, y, 348, 52, 6, mutedColor_);
    String label = wifiNetworks_[index];
    if (label.length() > 20) label = label.substring(0, 19) + ">";
    target->setTextSize(2);
    target->setTextColor(foregroundColor_);
    target->setCursor(20, y + 18);
    target->print(label);
    const uint8_t bars = constrain(map(wifiRssi_[index], -90, -40, 1, 4), 1L, 4L);
    for (uint8_t bar = 0; bar < 4; ++bar) {
      const int16_t height = 5 + bar * 5;
      if (bar < bars) target->fillRect(300 + bar * 6, y + 38 - height, 4, height,
                                       foregroundColor_);
      else target->drawRect(300 + bar * 6, y + 38 - height, 4, height, mutedColor_);
    }
    if (wifiSecure_[index]) {
      target->drawRect(332, y + 24, 14, 12, foregroundColor_);
      target->drawRoundRect(335, y + 15, 8, 12, 4, foregroundColor_);
    }
  }
  drawSetupButton(target, 10, kWifiListButtonY, 108, kWifiListButtonH,
                  "BACK", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 130, kWifiListButtonY, 108, kWifiListButtonH,
                  "SCAN", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 250, kWifiListButtonY, 108, kWifiListButtonH,
                  pages > 1 ? "MORE" : "-", panelColor_,
                  foregroundColor_, pages > 1 ? foregroundColor_ : mutedColor_);
}

void UserInterface::drawOscSettings(Arduino_GFX *target) {
  const char *labels[3] = {"TARGET", "SEND", "RECV"};
  const String *values[3] = {&oscTargetText_, &oscSendPortText_, &oscReceivePortText_};
  const int16_t fieldY[3] = {30, 78, 126};
  drawCenteredText(target, "OSC", 0, 8, kScreenWidth, 2, foregroundColor_);
  for (uint8_t field = 0; field < 3; ++field) {
    target->fillRoundRect(10, fieldY[field], 348, 42, 6, panelColor_);
    const uint16_t border = oscInputError_ ? rgb565(255, 82, 82)
                                           : (field == oscActiveField_ ? foregroundColor_
                                                                      : mutedColor_);
    drawThickRoundRect(target, 10, fieldY[field], 348, 42, 6, border);
    target->setTextSize(1);
    target->setTextColor(mutedColor_);
    target->setCursor(20, fieldY[field] + 17);
    target->print(labels[field]);
    target->setTextSize(2);
    target->setTextColor(foregroundColor_);
    target->setCursor(86, fieldY[field] + 13);
    target->print(*values[field]);
    drawCenteredText(target, "X", 314, fieldY[field] + 13, 34, 2, mutedColor_);
  }

  static const char keys[] = "123456789.0<";
  const int16_t keyX[3] = {10, 130, 250};
  for (uint8_t row = 0; row < 4; ++row) {
    for (uint8_t col = 0; col < 3; ++col) {
      const int16_t x = keyX[col];
      const int16_t y = 178 + row * 47;
      String label(keys[row * 3 + col]);
      if (label == "<") label = "DEL";
      const bool unavailableDot = label == "." && oscActiveField_ != 0;
      drawSetupButton(target, x, y, 108, 41, unavailableDot ? "-" : label,
                      panelColor_, mutedColor_,
                      unavailableDot ? mutedColor_ : foregroundColor_);
    }
  }

  drawSetupButton(target, 10, 378, 168, 56, "BACK", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 190, 378, 168, 56, "SAVE", foregroundColor_, foregroundColor_,
                  backgroundColor_);
}

void UserInterface::drawDeviceSettings(Arduino_GFX *target) {
  drawCenteredText(target, "DEVICE", 0, 12, kScreenWidth, 2, foregroundColor_);
  target->fillRoundRect(10, 48, 348, 48, 6, panelColor_);
  drawThickRoundRect(target, 10, 48, 348, 48, 6,
                     deviceNameInputError_ ? rgb565(255, 82, 82) : foregroundColor_);
  drawCenteredText(target, deviceNameText_, 16, 64, 336, 2, foregroundColor_);

  static const char *rows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm-"};
  for (uint8_t row = 0; row < 4; ++row) {
    const char *keys = rows[row];
    const uint8_t count = strlen(keys);
    const int16_t gap = 3;
    const int16_t keyWidth = (kScreenWidth - 16 - (count - 1) * gap) / count;
    for (uint8_t col = 0; col < count; ++col) {
      const int16_t x = 8 + col * (keyWidth + gap);
      const int16_t y = 110 + row * 50;
      target->fillRoundRect(x, y, keyWidth, 42, 4, panelColor_);
      target->drawRoundRect(x, y, keyWidth, 42, 4, mutedColor_);
      drawCenteredText(target, String(keys[col]), x, y + 13, keyWidth, 2,
                       foregroundColor_);
    }
  }

  drawSetupButton(target, 8, 314, 110, 46, "CLEAR", panelColor_, mutedColor_,
                  foregroundColor_);
  drawSetupButton(target, 129, 314, 110, 46, "DEFAULT", panelColor_, mutedColor_,
                  foregroundColor_);
  drawSetupButton(target, 250, 314, 110, 46, "DEL", panelColor_, mutedColor_,
                  foregroundColor_);
  drawSetupButton(target, 10, 378, 168, 56, "BACK", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 190, 378, 168, 56, "SAVE", foregroundColor_, foregroundColor_,
                  backgroundColor_);
}

void UserInterface::drawPhysicsSettings(Arduino_GFX *target) {
  drawCenteredText(target, "PHYSICS", 0, 12, kScreenWidth, 2, foregroundColor_);

  drawCenteredText(target, "GRAVITY", 0, 49, kScreenWidth, 2, mutedColor_);
  char gravityValue[8];
  snprintf(gravityValue, sizeof(gravityValue), "%.1f", physicsGravityEdit_);
  drawCenteredText(target, gravityValue, 0, 77, kScreenWidth, 4, foregroundColor_);
  drawSetupButton(target, 10, 125, 168, 64, "-", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 190, 125, 168, 64, "+", panelColor_, foregroundColor_,
                  foregroundColor_);

  drawCenteredText(target, "BOUNCE", 0, 216, kScreenWidth, 2, mutedColor_);
  char bounceValue[8];
  snprintf(bounceValue, sizeof(bounceValue), "%d%%",
           static_cast<int>(roundf(physicsBouncinessEdit_ * 100.0f)));
  drawCenteredText(target, bounceValue, 0, 244, kScreenWidth, 4, foregroundColor_);
  drawSetupButton(target, 10, 292, 168, 64, "-", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 190, 292, 168, 64, "+", panelColor_, foregroundColor_,
                  foregroundColor_);

  drawSetupButton(target, 10, 378, 168, 56, "BACK", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 190, 378, 168, 56, "SAVE", foregroundColor_, foregroundColor_,
                  backgroundColor_);
}

void UserInterface::drawWifiKeyboard(Arduino_GFX *target) {
  String title = wifiSelectedSsid_;
  if (title.length() > 24) title = title.substring(0, 23) + ">";
  drawCenteredText(target, title, 0, 12, kScreenWidth, 2, foregroundColor_);
  target->fillRoundRect(10, 48, 348, 48, 6, panelColor_);
  drawThickRoundRect(target, 10, 48, 348, 48, 6,
                     wifiPasswordError_ ? rgb565(255, 82, 82) : mutedColor_);
  String passwordDisplay;
  const uint8_t visibleCharacters = 20;
  const size_t start = wifiPassword_.length() > visibleCharacters
                           ? wifiPassword_.length() - visibleCharacters
                           : 0;
  if (wifiPasswordVisible_) {
    passwordDisplay = wifiPassword_.substring(start);
  } else {
    for (size_t i = start; i < wifiPassword_.length(); ++i) passwordDisplay += '*';
  }
  if (start > 0) passwordDisplay = ">" + passwordDisplay;
  drawCenteredText(target, passwordDisplay, 18, 64, 250, 2, foregroundColor_);
  drawSetupButton(target, 278, 55, 72, 34, wifiPasswordVisible_ ? "HIDE" : "SHOW",
                  panelColor_, foregroundColor_, foregroundColor_);

  for (uint8_t row = 0; row < 4; ++row) {
    const char *keys = keyboardRow(row);
    const uint8_t count = strlen(keys);
    const int16_t gap = 3;
    const int16_t keyWidth = (kScreenWidth - 16 - (count - 1) * gap) / count;
    for (uint8_t col = 0; col < count; ++col) {
      const int16_t x = 8 + col * (keyWidth + gap);
      const int16_t y = 110 + row * 50;
      target->fillRoundRect(x, y, keyWidth, 42, 4, panelColor_);
      target->drawRoundRect(x, y, keyWidth, 42, 4, mutedColor_);
      String label(keys[col]);
      drawCenteredText(target, label, x, y + 13, keyWidth, 2, foregroundColor_);
    }
  }

  drawSetupButton(target, 8, 314, 80, 46, wifiSymbols_ ? "-" : (wifiShift_ ? "abc" : "ABC"),
                  panelColor_, mutedColor_, foregroundColor_);
  drawSetupButton(target, 94, 314, 80, 46, wifiSymbols_ ? "ABC" : "#+=", panelColor_,
                  mutedColor_, foregroundColor_);
  drawSetupButton(target, 180, 314, 98, 46, "SPACE", panelColor_, mutedColor_,
                  foregroundColor_);
  drawSetupButton(target, 284, 314, 76, 46, "DEL", panelColor_, mutedColor_,
                  foregroundColor_);
  drawSetupButton(target, 10, 378, 168, 56, "BACK", panelColor_, foregroundColor_,
                  foregroundColor_);
  drawSetupButton(target, 190, 378, 168, 56, "CONNECT", foregroundColor_,
                  foregroundColor_, backgroundColor_);
}

void UserInterface::drawWifiSetup() {
  if (!display_ || !wifiSetupActive_) return;
  Arduino_GFX *target = setupCanvas_ ? static_cast<Arduino_GFX *>(setupCanvas_)
                                     : static_cast<Arduino_GFX *>(display_);
  target->fillScreen(backgroundColor_);
  switch (wifiSetupPage_) {
    case WifiSetupPage::Menu:
      drawSettingsMenu(target);
      break;
    case WifiSetupPage::Scanning:
      drawWifiMessage(target, "SCANNING...");
      break;
    case WifiSetupPage::Networks:
      drawWifiNetworks(target);
      break;
    case WifiSetupPage::Password:
      drawWifiKeyboard(target);
      break;
    case WifiSetupPage::Connecting:
      drawWifiMessage(target, "CONNECTING...");
      break;
    case WifiSetupPage::Connected:
      drawWifiMessage(target, "CONNECTED");
      break;
    case WifiSetupPage::Osc:
      drawOscSettings(target);
      break;
    case WifiSetupPage::Device:
      drawDeviceSettings(target);
      break;
    case WifiSetupPage::Physics:
      drawPhysicsSettings(target);
      break;
    case WifiSetupPage::Saved:
      drawWifiMessage(target, "SAVED");
      break;
    case WifiSetupPage::Restarting:
      drawWifiMessage(target, "RESTARTING...");
      break;
  }
  if (setupCanvas_) setupCanvas_->flush();
}

void UserInterface::loop(ControlState &state, const ImuFrame &imu, float micEnergy,
                         const float spectrum[32], bool spectrumReady,
                         bool wifiConnected, bool bleEnabled, bool bleConnected,
                         bool imuOutputEnabled) {
  if (!display_) return;
  serviceTouchRecovery();
  if (wifiSetupActive_) {
    readWifiSetupTouch();
    serviceWifiSetup();
    return;
  }
  if (touchOscLayout_.active()) {
    readTouch(state);
    if (wifiSetupActive_) return;
    updatePowerStatus();
    if (!dimmed_ && millis() - lastActivityMs_ >= dimAfterMs_) {
      display_->setBrightness(kDimBrightness);
      dimmed_ = true;
    }
    const bool overlayChanged = !layoutOverlayValid_ ||
                                drawnLayoutWifiConnected_ != wifiConnected ||
                                drawnLayoutBatteryPercent_ != batteryPercent_ ||
                                drawnLayoutCharging_ != charging_;
    const uint32_t drawNowUs = micros();
    bool layoutDrawn = false;
    if (touchOscLayout_.dirty() &&
        static_cast<uint32_t>(drawNowUs - lastControlDrawUs_) >= 20000) {
      lastControlDrawUs_ = drawNowUs;
      Arduino_GFX *target = setupCanvas_
          ? static_cast<Arduino_GFX *>(setupCanvas_)
          : static_cast<Arduino_GFX *>(display_);
      int16_t updateX, updateY, updateWidth, updateHeight;
      touchOscLayout_.draw(target, kScreenWidth, kScreenHeight,
                           setupCanvas_ && layoutTransferBuffer_,
                           updateX, updateY, updateWidth, updateHeight);
      drawLayoutStatusOverlay(target, wifiConnected);
      layoutDrawn = true;
      if (setupCanvas_) {
        flushLayoutRegion(updateX, updateY, updateWidth, updateHeight);
        if (overlayChanged) {
          flushLayoutRegion(0, 0, 48, 44);
          flushLayoutRegion(316, 0, 52, 44);
        }
      }
    }
    if (!layoutDrawn && overlayChanged) {
      Arduino_GFX *target = setupCanvas_
          ? static_cast<Arduino_GFX *>(setupCanvas_)
          : static_cast<Arduino_GFX *>(display_);
      drawLayoutStatusOverlay(target, wifiConnected);
      if (setupCanvas_) {
        flushLayoutRegion(0, 0, 48, 44);
        flushLayoutRegion(316, 0, 52, 44);
      }
    }
    return;
  }
  // Without an installed layout, Parser Mini presents a purpose-built home
  // screen. The inherited demonstration apps are not part of this runtime.
  readTouch(state);
  if (wifiSetupActive_) return;
  updatePowerStatus();
  if (!dimmed_ && millis() - lastActivityMs_ >= dimAfterMs_) {
    display_->setBrightness(kDimBrightness);
    dimmed_ = true;
  }

  const bool overlayChanged = !layoutOverlayValid_ ||
                              drawnLayoutWifiConnected_ != wifiConnected ||
                              drawnLayoutBatteryPercent_ != batteryPercent_ ||
                              drawnLayoutCharging_ != charging_;
  Arduino_GFX *target = setupCanvas_
      ? static_cast<Arduino_GFX *>(setupCanvas_)
      : static_cast<Arduino_GFX *>(display_);
  if (!hasDrawn_) {
    drawLandingScreen(target, wifiConnected);
    if (setupCanvas_) setupCanvas_->flush();
    hasDrawn_ = true;
  } else if (overlayChanged) {
    drawLayoutStatusOverlay(target, wifiConnected);
    if (setupCanvas_) {
      flushLayoutRegion(0, 0, 48, 44);
      flushLayoutRegion(316, 0, 52, 44);
    }
  }
}

void UserInterface::wake() {
  lastActivityMs_ = millis();
  if (dimmed_ && display_) {
    display_->setBrightness(kAwakeBrightness);
    dimmed_ = false;
  }
}

void UserInterface::noteMotion() {
  wake();
}

UserInterface::TouchTarget UserInterface::hitTest(int16_t x, int16_t y) const {
  if (inside(x, y, kStatusBadgeShiftX, 0, 48, kTopHeight)) return TouchTarget::Wifi;
  if (inside(x, y, 48 + kStatusBadgeShiftX, 0, 42, kTopHeight)) return TouchTarget::Ble;
  if (inside(x, y, 90 + kStatusBadgeShiftX, 0, 42, kTopHeight)) {
    return TouchTarget::ImuOutput;
  }
  // Treat the complete strip below the page canvas as the page control. The
  // expanded target compensates for the rounded glass and reduced touch
  // accuracy at the bottom edge while preserving the visible geometry.
  if (y >= kControlY + kControlH) return TouchTarget::Page;
  if (inside(x, y, kControlX, kControlY, kControlW, kControlH)) {
    if (controlPage_ == kPageXy) return TouchTarget::Xy;
    if (controlPage_ == kPageFaders) {
      const uint8_t index = constrain((x - kControlX) * 4 / kControlW, 0, 3);
      return static_cast<TouchTarget>(static_cast<uint8_t>(TouchTarget::Fader0) + index);
    }
    if (controlPage_ == kPageBalls) return TouchTarget::BallArena;
    if (controlPage_ == kPagePendulums) return TouchTarget::Pendulum;
    if (controlPage_ == kPageKeyboard) {
      if (keyboardKeyAt(x, y) >= 0) return TouchTarget::Keyboard;
      if (inside(x, y, kControlX, kControlY + kKeyboardOctaveY,
                 kKeyboardOctaveWidth, kKeyboardOctaveHeight)) {
        return TouchTarget::OctaveDown;
      }
      if (inside(x, y,
                 kControlX + kKeyboardOctaveWidth + kKeyboardOctaveGap,
                 kControlY + kKeyboardOctaveY,
                 kKeyboardOctaveWidth, kKeyboardOctaveHeight)) {
        return TouchTarget::OctaveUp;
      }
      return TouchTarget::None;
    }
    if (controlPage_ == kPageSpectrum) {
      if (inside(x, y, kControlX, kControlY, kControlW, kSpectrumHeight)) {
        return TouchTarget::Spectrum;
      }
      if (inside(x, y, kControlX, kControlY + kSpectrumCaptureY,
                 kControlW, kSpectrumCaptureHeight)) {
        return TouchTarget::FftCapture;
      }
      return TouchTarget::None;
    }
    if (controlPage_ != kPageButtons) return TouchTarget::None;
    for (uint8_t index = 0; index < 4; ++index) {
      const int16_t buttonX = kControlX + kButtonInsetX +
                              (index % 2) * (kButtonSize + kControlGap);
      const int16_t buttonY = kControlY + kButtonInsetY +
                              (index / 2) * (kButtonSize + kControlGap);
      if (inside(x, y, buttonX, buttonY, kButtonSize, kButtonSize)) {
        return static_cast<TouchTarget>(
            static_cast<uint8_t>(TouchTarget::Button0) + index);
      }
    }
  }
  return TouchTarget::None;
}

bool UserInterface::readTouchPoint(bool &pressed, int16_t &x, int16_t &y) {
  if (!i2cBus_) return false;
  uint8_t sample[5] = {};
  i2cBus_->BeginTransmission(CST816T_DEVICE_ADDRESS);
  if (!i2cBus_->Write(CST816x_RD_DEVICE_FINGERNUM) || !i2cBus_->EndTransmission() ||
      !i2cBus_->RequestFrom(CST816T_DEVICE_ADDRESS, sizeof(sample))) {
    return false;
  }
  for (uint8_t &value : sample) value = i2cBus_->Read();
  pressed = (sample[0] & 0x0f) > 0;
  if (!pressed) return true;
  x = constrain(static_cast<int16_t>(((sample[1] & 0x0f) << 8) | sample[2]),
                static_cast<int16_t>(0), static_cast<int16_t>(kScreenWidth - 1));
  y = constrain(static_cast<int16_t>(((sample[3] & 0x0f) << 8) | sample[4]),
                static_cast<int16_t>(0), static_cast<int16_t>(kScreenHeight - 1));
  return true;
}

void UserInterface::readTouch(ControlState &state) {
  if (!touchReady_ || millis() - lastTouchPollMs_ < 8) return;
  lastTouchPollMs_ = millis();
  bool pressed = false;
  int16_t x = 0;
  int16_t y = 0;
  if (!readTouchPoint(pressed, x, y)) return;
  if (ignoreTouchUntilRelease_) {
    // A timeout prevents a damaged/missed release report from locking the UI,
    // while the normal path clears immediately on the first finger-up sample.
    if (!pressed || millis() - ignoreTouchStartedMs_ >= 500) {
      ignoreTouchUntilRelease_ = false;
    }
    return;
  }
  if (touchOscLayout_.active()) {
    if (pressed) {
      if (!touching_) {
        touching_ = true;
        touchStartedMs_ = millis();
        longActionSent_ = false;
        wake();
        layoutWifiTouch_ = inside(x, y, 0, 0, 48, 48);
        layoutPageTouch_ = touchOscLayout_.pageCount() > 1 &&
                           inside(x, y, 132, 0, 104, 48);
        if (!layoutWifiTouch_ && !layoutPageTouch_) {
          touchOscLayout_.beginTouch(x, y, kScreenWidth, kScreenHeight);
        }
      } else if (!layoutWifiTouch_ && !layoutPageTouch_) {
        touchOscLayout_.moveTouch(x, y, kScreenWidth, kScreenHeight);
      }
    } else if (touching_) {
      if (layoutWifiTouch_) {
        layoutWifiTouch_ = false;
        touching_ = false;
        openWifiSetup();
        return;
      }
      if (layoutPageTouch_) {
        layoutPageTouch_ = false;
        touching_ = false;
        touchOscLayout_.nextPage();
        wake();
        return;
      }
      touchOscLayout_.endTouch();
      touching_ = false;
    }
    return;
  }
  if (pressed) {
    if (!touching_) {
      touching_ = true;
      touchStartedMs_ = millis();
      longActionSent_ = false;
      wake();
      layoutWifiTouch_ = inside(x, y, 0, 0, 48, 48);
    }
  } else if (touching_) {
    const bool openSettings = layoutWifiTouch_;
    touching_ = false;
    layoutWifiTouch_ = false;
    if (openSettings) openWifiSetup();
  }
}

void UserInterface::beginTouch(TouchTarget target, int16_t x, int16_t y, ControlState &state) {
  UiEvent event;
  switch (target) {
    case TouchTarget::Xy:
      xyFilterInitialized_ = false;
      updateXy(x, y, state);
      break;
    case TouchTarget::Fader0:
    case TouchTarget::Fader1:
    case TouchTarget::Fader2:
    case TouchTarget::Fader3:
      updateFader(static_cast<uint8_t>(target) - static_cast<uint8_t>(TouchTarget::Fader0),
                  y, state);
      break;
    case TouchTarget::Button0:
    case TouchTarget::Button1:
    case TouchTarget::Button2:
    case TouchTarget::Button3:
      event.control = MessageType::Button;
      event.index = static_cast<uint8_t>(target) - static_cast<uint8_t>(TouchTarget::Button0);
      state.buttons[event.index] = true;
      event.state = true;
      enqueue(event);
      break;
    case TouchTarget::BallArena:
      handleBallTap(x, y);
      break;
    case TouchTarget::Pendulum:
      beginPendulumDraw(x, y);
      break;
    case TouchTarget::Keyboard:
      updateKeyboardKey(keyboardKeyAt(x, y), y, state);
      break;
    case TouchTarget::OctaveDown:
      octaveButtonPressed_ = 0;
      shiftKeyboardOctave(-1, state);
      break;
    case TouchTarget::OctaveUp:
      octaveButtonPressed_ = 1;
      shiftKeyboardOctave(1, state);
      break;
    case TouchTarget::Spectrum:
      lastSpectrumTouchIndex_ = -1;
      updateSpectrumSlider(x, y, state, true);
      break;
    case TouchTarget::FftCapture:
      startFftCapture(state);
      break;
    case TouchTarget::Page:
      pageButtonPressed_ = true;
      controlPage_ = (controlPage_ + 1) % kControlPageCount;
      drawCurrentPage(state);
      drawPageButton();
      drawnState_ = state;
      break;
    default:
      break;
  }
}

void UserInterface::moveTouch(int16_t x, int16_t y, ControlState &state) {
  if (activeTouch_ == TouchTarget::Ble && !longActionSent_ &&
      millis() - touchStartedMs_ >= kLongPressMs) {
    UiEvent event;
    event.type = UiEventType::ClearBleBonds;
    enqueue(event);
    longActionSent_ = true;
    return;
  }
  if (activeTouch_ == TouchTarget::BallArena && !longActionSent_ &&
      millis() - touchStartedMs_ >= kResetHoldMs) {
    resetBalls();
    longActionSent_ = true;
    return;
  }
  if (activeTouch_ == TouchTarget::Pendulum && pendulumResetHoldEligible_ &&
      !longActionSent_ && millis() - touchStartedMs_ >= kResetHoldMs) {
    resetPendulumsToCentre();
    longActionSent_ = true;
    return;
  }
  if (activeTouch_ == TouchTarget::Xy) updateXy(x, y, state);
  if (activeTouch_ == TouchTarget::Pendulum) movePendulumDraw(x, y);
  if (activeTouch_ == TouchTarget::Keyboard) {
    updateKeyboardKey(keyboardKeyAt(x, y), y, state);
  }
  if (activeTouch_ == TouchTarget::Spectrum) {
    updateSpectrumSlider(x, y, state);
  }
  if (activeTouch_ >= TouchTarget::Fader0 && activeTouch_ <= TouchTarget::Fader3) {
    const uint8_t previousIndex = static_cast<uint8_t>(activeTouch_) -
                                  static_cast<uint8_t>(TouchTarget::Fader0);
    const uint8_t index = constrain((x - kControlX) * 4 / kControlW, 0, 3);
    const bool enteredNewFader = index != previousIndex;
    if (enteredNewFader) {
      activeTouch_ = static_cast<TouchTarget>(
          static_cast<uint8_t>(TouchTarget::Fader0) + index);
    }
    updateFader(index, y, state, enteredNewFader);
  }
}

void UserInterface::endTouch(ControlState &state) {
  UiEvent event;
  if (activeTouch_ >= TouchTarget::Button0 && activeTouch_ <= TouchTarget::Button3) {
    event.control = MessageType::Button;
    event.index = static_cast<uint8_t>(activeTouch_) -
                  static_cast<uint8_t>(TouchTarget::Button0);
    state.buttons[event.index] = false;
    event.state = false;
    enqueue(event);
  } else if (activeTouch_ == TouchTarget::Page) {
    pageButtonPressed_ = false;
    drawPageButton();
  } else if (activeTouch_ == TouchTarget::Wifi) {
    openWifiSetup();
  } else if (activeTouch_ == TouchTarget::Ble && !longActionSent_) {
    event.type = UiEventType::ToggleBle;
    enqueue(event);
  } else if (activeTouch_ == TouchTarget::ImuOutput) {
    event.type = UiEventType::ToggleImuOutput;
    enqueue(event);
  } else if (activeTouch_ == TouchTarget::Pendulum) {
    finishPendulumDraw();
  } else if (activeTouch_ == TouchTarget::Keyboard) {
    updateKeyboardKey(-1, kControlY, state);
  } else if (activeTouch_ == TouchTarget::OctaveDown ||
             activeTouch_ == TouchTarget::OctaveUp) {
    octaveButtonPressed_ = -1;
    keyboardPartialValid_ = false;
    drawCurrentPage(state);
    drawnState_ = state;
  } else if (activeTouch_ == TouchTarget::Spectrum && touchValueChanged_) {
    event.control = MessageType::Spectrum;
    memcpy(event.values, state.spectrum, sizeof(state.spectrum));
    enqueue(event);
    lastSpectrumTouchIndex_ = -1;
  } else if (activeTouch_ == TouchTarget::FftCapture) {
    finishFftCapture(state);
  } else if (activeTouch_ == TouchTarget::Xy && touchValueChanged_) {
    event.control = MessageType::Xy;
    event.values[0] = state.xyX;
    event.values[1] = state.xyY;
    enqueue(event);
  } else if (activeTouch_ >= TouchTarget::Fader0 && activeTouch_ <= TouchTarget::Fader3 &&
             touchValueChanged_) {
    event.control = MessageType::Fader;
    event.index = static_cast<uint8_t>(activeTouch_) -
                  static_cast<uint8_t>(TouchTarget::Fader0);
    event.values[0] = state.faders[event.index];
    enqueue(event);
  }
}

void UserInterface::updateXy(int16_t x, int16_t y, ControlState &state, bool) {
  const float rawX = x;
  const float rawY = y;
  if (!xyFilterInitialized_) {
    xyFilteredPixelX_ = rawX;
    xyFilteredPixelY_ = rawY;
    xyFilterInitialized_ = true;
  } else {
    const float deltaX = rawX - xyFilteredPixelX_;
    const float deltaY = rawY - xyFilteredPixelY_;
    const float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
    constexpr float kTouchDeadbandPixels = 1.5f;
    if (distance > kTouchDeadbandPixels) {
      // Small motion is strongly damped; deliberate movement quickly approaches
      // direct tracking. This keeps a held cross still without making sweeps laggy.
      const float alpha = constrain(0.20f +
          (distance - kTouchDeadbandPixels) * 0.055f, 0.20f, 0.88f);
      xyFilteredPixelX_ += deltaX * alpha;
      xyFilteredPixelY_ += deltaY * alpha;
    }
  }

  const float nextX = constrain((xyFilteredPixelX_ - kControlX) / (kControlW - 1),
                                0.0f, 1.0f);
  const float nextY = 1.0f - constrain((xyFilteredPixelY_ - kControlY) / (kControlH - 1),
                                       0.0f, 1.0f);
  if (!different(nextX, state.xyX) && !different(nextY, state.xyY)) return;
  state.xyX = nextX;
  state.xyY = nextY;
  touchValueChanged_ = true;
  if (millis() - lastXyStreamSendMs_ >= kStreamIntervalMs) {
    UiEvent event;
    event.control = MessageType::Xy;
    event.values[0] = nextX;
    event.values[1] = nextY;
    enqueue(event);
    lastXyStreamSendMs_ = millis();
  }
}

void UserInterface::updateFader(uint8_t index, int16_t y, ControlState &state,
                                bool forceSend) {
  if (index >= 4) return;
  const float next = 1.0f - constrain(static_cast<float>(y - kControlY) / (kControlH - 1),
                                      0.0f, 1.0f);
  if (!different(next, state.faders[index])) return;
  state.faders[index] = next;
  touchValueChanged_ = true;
  if (forceSend || millis() - lastFaderStreamSendMs_[index] >= kStreamIntervalMs) {
    UiEvent event;
    event.control = MessageType::Fader;
    event.index = index;
    event.values[0] = next;
    enqueue(event);
    lastFaderStreamSendMs_[index] = millis();
  }
}

int8_t UserInterface::keyboardKeyAt(int16_t x, int16_t y) const {
  if (!inside(x, y, kControlX, kControlY, kControlW, kKeyboardHeight)) return -1;
  const int16_t localX = x - kControlX;
  const int16_t localY = y - kControlY;

  // Black keys sit above the white keys and therefore win in their overlap.
  if (localY < kKeyboardBlackKeyHeight) {
    for (uint8_t index = 0; index < 5; ++index) {
      const int16_t centre = kKeyboardBlackBoundaries[index] * kKeyboardWhiteKeyWidth;
      if (inside(localX, localY, centre - kKeyboardBlackKeyWidth / 2, 0,
                 kKeyboardBlackKeyWidth, kKeyboardBlackKeyHeight)) {
        return kKeyboardBlackNotes[index];
      }
    }
  }

  const uint8_t whiteIndex = constrain(localX / kKeyboardWhiteKeyWidth, 0, 7);
  return kKeyboardWhiteNotes[whiteIndex];
}

uint8_t UserInterface::keyboardVelocityAt(int8_t key, int16_t y) const {
  if (key < 0 || key > 12) return 0;
  bool blackKey = false;
  for (uint8_t index = 0; index < 5; ++index) {
    if (kKeyboardBlackNotes[index] == key) {
      blackKey = true;
      break;
    }
  }
  const int16_t keyHeight = blackKey ? kKeyboardBlackKeyHeight : kKeyboardHeight;
  const int16_t localY = constrain(y - kControlY, 0, keyHeight - 1);
  return static_cast<uint8_t>(20 +
      (static_cast<int32_t>(localY) * 107 + (keyHeight - 1) / 2) /
          (keyHeight - 1));
}

void UserInterface::updateKeyboardKey(int8_t key, int16_t y, ControlState &state) {
  if (key < -1 || key > 12 || key == activeKeyboardKey_) return;

  UiEvent event;
  event.control = MessageType::Note;
  if (activeKeyboardKey_ >= 0) {
    event.index = keyboardBaseMidiNote_ + activeKeyboardKey_;
    state.keyboardNotes[event.index] = false;
    event.state = false;
    event.numbers[0] = 0;
    enqueue(event);
  }
  activeKeyboardKey_ = key;
  if (activeKeyboardKey_ >= 0) {
    event.index = keyboardBaseMidiNote_ + activeKeyboardKey_;
    state.keyboardNotes[event.index] = true;
    event.state = true;
    event.numbers[0] = keyboardVelocityAt(activeKeyboardKey_, y);
    enqueue(event);
  }
}

void UserInterface::shiftKeyboardOctave(int8_t direction, ControlState &state) {
  if (activeKeyboardKey_ >= 0) updateKeyboardKey(-1, kControlY, state);
  const int16_t requested = static_cast<int16_t>(keyboardBaseMidiNote_) +
                            static_cast<int16_t>(direction) * 12;
  keyboardBaseMidiNote_ = static_cast<uint8_t>(constrain(
      requested, static_cast<int16_t>(kKeyboardMinimumBaseMidiNote),
      static_cast<int16_t>(kKeyboardMaximumBaseMidiNote)));
  keyboardPartialValid_ = false;
  drawCurrentPage(state);
  drawnState_ = state;
}

void UserInterface::updateSpectrumSlider(int16_t x, int16_t y, ControlState &state,
                                         bool forceSend) {
  const int16_t localX = constrain(x - kControlX, 0, kControlW - 1);
  const int8_t index = constrain(localX * kSpectrumBandCount / kControlW,
                                 0, kSpectrumBandCount - 1);
  const float value = 1.0f - constrain(
      static_cast<float>(y - kControlY) / (kSpectrumHeight - 1), 0.0f, 1.0f);

  bool changed = false;
  if (lastSpectrumTouchIndex_ < 0) {
    changed = different(state.spectrum[index], value);
    state.spectrum[index] = value;
  } else {
    const int8_t first = min(lastSpectrumTouchIndex_, index);
    const int8_t last = max(lastSpectrumTouchIndex_, index);
    for (int8_t band = first; band <= last; ++band) {
      const float amount = index == lastSpectrumTouchIndex_ ? 1.0f :
          static_cast<float>(band - lastSpectrumTouchIndex_) /
          static_cast<float>(index - lastSpectrumTouchIndex_);
      const float interpolated = lastSpectrumTouchValue_ +
                                 (value - lastSpectrumTouchValue_) * amount;
      changed |= different(state.spectrum[band], interpolated);
      state.spectrum[band] = interpolated;
    }
  }
  lastSpectrumTouchIndex_ = index;
  lastSpectrumTouchValue_ = value;
  if (!changed) return;

  touchValueChanged_ = true;
  if (forceSend || millis() - lastSpectrumStreamSendMs_ >= kStreamIntervalMs) {
    UiEvent event;
    event.control = MessageType::Spectrum;
    memcpy(event.values, state.spectrum, sizeof(state.spectrum));
    enqueue(event);
    lastSpectrumStreamSendMs_ = millis();
  }
}

void UserInterface::startFftCapture(ControlState &state) {
  fftCaptureActive_ = true;
  memset(state.spectrum, 0, sizeof(state.spectrum));
  keyboardPartialValid_ = false;
  spectrumPartialValid_ = false;
  drawCurrentPage(state);
  drawnState_ = state;
}

void UserInterface::finishFftCapture(ControlState &state) {
  if (!fftCaptureActive_) return;
  fftCaptureActive_ = false;
  UiEvent event;
  event.control = MessageType::Spectrum;
  memcpy(event.values, state.spectrum, sizeof(state.spectrum));
  enqueue(event);
  spectrumPartialValid_ = false;
  drawCurrentPage(state);
  drawnState_ = state;
}

void UserInterface::applyFftSpectrum(const float spectrum[32], ControlState &state) {
  if (!spectrum || !fftCaptureActive_) return;
  for (uint8_t index = 0; index < kSpectrumBandCount; ++index) {
    state.spectrum[index] = constrain(spectrum[index], 0.0f, 1.0f);
  }
}

void UserInterface::handleBallTap(int16_t x, int16_t y) {
  const int16_t minimumX = kControlX + kBallArenaX + kBallWallWidth + kBallRadius;
  const int16_t maximumX = kControlX + kBallArenaX + kBallArenaSize -
                           kBallWallWidth - kBallRadius - 1;
  const int16_t minimumY = kControlY + kBallArenaY + kBallWallWidth + kBallRadius;
  const int16_t maximumY = kControlY + kBallArenaY + kBallArenaSize -
                           kBallWallWidth - kBallRadius - 1;

  // Removal gets a slightly larger hit target than the drawn ball so a fingertip
  // can select it reliably without changing the visual design.
  for (uint8_t index = 0; index < kMaxBalls; ++index) {
    BallState &ball = balls_[index];
    if (!ball.active) continue;
    const float ballPixelX = minimumX + ball.x * (maximumX - minimumX);
    const float ballPixelY = minimumY + ball.y * (maximumY - minimumY);
    const float dx = x - ballPixelX;
    const float dy = y - ballPixelY;
    if (dx * dx + dy * dy <= kBallTapRadius * kBallTapRadius) {
      ball = BallState{};
      uint8_t pairBit = 0;
      for (uint8_t first = 0; first < kMaxBalls; ++first) {
        for (uint8_t second = first + 1; second < kMaxBalls; ++second, ++pairBit) {
          if (first == index || second == index) {
            ballPairContacts_ &= ~(1UL << pairBit);
          }
        }
      }
      ballDirty_ = true;
      return;
    }
  }

  if (x < minimumX || x > maximumX || y < minimumY || y > maximumY) return;
  const float nextX = static_cast<float>(x - minimumX) / (maximumX - minimumX);
  const float nextY = static_cast<float>(y - minimumY) / (maximumY - minimumY);
  const float minimumDistance = static_cast<float>(2 * kBallRadius) /
                                static_cast<float>(maximumX - minimumX);
  for (const BallState &ball : balls_) {
    if (!ball.active) continue;
    const float dx = nextX - ball.x;
    const float dy = nextY - ball.y;
    if (dx * dx + dy * dy < minimumDistance * minimumDistance) return;
  }

  for (BallState &ball : balls_) {
    if (ball.active) continue;
    ball = BallState{};
    ball.active = true;
    ball.x = nextX;
    ball.y = nextY;
    ballDirty_ = true;
    return;
  }
}

void UserInterface::resetBalls() {
  for (BallState &ball : balls_) ball = BallState{};
  ballPairContacts_ = 0;
  ballDirty_ = true;
}

void UserInterface::updateBall(const ImuFrame &imu) {
  const uint32_t now = micros();
  if (lastBallUpdateUs_ == 0) {
    lastBallUpdateUs_ = now;
    return;
  }
  const uint32_t elapsedUs = static_cast<uint32_t>(now - lastBallUpdateUs_);
  if (elapsedUs < 10000) return;
  lastBallUpdateUs_ = now;
  const float dt = constrain(elapsedUs / 1000000.0f, 0.001f, 0.05f);

  // A stationary accelerometer measures support force, which points opposite
  // the gravity vector needed by the ball simulation. Y is then converted
  // from logical screen-up coordinates to display pixel-down coordinates.
  const float damping = expf(-0.55f * dt);
  bool anyActive = false;
  for (BallState &ball : balls_) {
    if (!ball.active) continue;
    anyActive = true;
    ball.velocityX -= imu.accel[0] * ballGravity_ * dt;
    ball.velocityY += imu.accel[1] * ballGravity_ * dt;
    ball.velocityX = constrain(ball.velocityX * damping,
                               -kBallMaxVelocity, kBallMaxVelocity);
    ball.velocityY = constrain(ball.velocityY * damping,
                               -kBallMaxVelocity, kBallMaxVelocity);
    ball.x += ball.velocityX * dt;
    ball.y += ball.velocityY * dt;
  }

  // Resolve equal-mass ball contacts. A pair stays latched while overlapping so
  // one physical contact produces one OSC event rather than a stream of repeats.
  constexpr float kCenterTravel = kBallArenaSize - 2 * kBallWallWidth -
                                  2 * kBallRadius - 1;
  constexpr float kMinimumDistance = 2.0f * kBallRadius / kCenterTravel;
  constexpr float kMinimumDistanceSquared = kMinimumDistance * kMinimumDistance;
  uint32_t currentContacts = 0;
  uint8_t pairBit = 0;
  for (uint8_t first = 0; first < kMaxBalls; ++first) {
    for (uint8_t second = first + 1; second < kMaxBalls; ++second, ++pairBit) {
      BallState &a = balls_[first];
      BallState &b = balls_[second];
      if (!a.active || !b.active) continue;
      float dx = b.x - a.x;
      float dy = b.y - a.y;
      const float distanceSquared = dx * dx + dy * dy;
      if (distanceSquared >= kMinimumDistanceSquared) continue;

      const uint32_t contactMask = 1UL << pairBit;
      currentContacts |= contactMask;
      float distance = sqrtf(distanceSquared);
      if (distance < 0.0001f) {
        dx = 1.0f;
        dy = 0.0f;
        distance = 0.0f;
      } else {
        dx /= distance;
        dy /= distance;
      }

      const float overlap = kMinimumDistance - distance;
      a.x -= dx * overlap * 0.5f;
      a.y -= dy * overlap * 0.5f;
      b.x += dx * overlap * 0.5f;
      b.y += dy * overlap * 0.5f;

      const float relativeVelocity = (b.velocityX - a.velocityX) * dx +
                                     (b.velocityY - a.velocityY) * dy;
      if (relativeVelocity < 0.0f) {
        const float impulse = -(1.0f + ballBounciness_) * relativeVelocity * 0.5f;
        a.velocityX -= impulse * dx;
        a.velocityY -= impulse * dy;
        b.velocityX += impulse * dx;
        b.velocityY += impulse * dy;
        if ((ballPairContacts_ & contactMask) == 0) {
          UiEvent event;
          event.type = UiEventType::BallCollision;
          event.index = first;
          event.numbers[0] = second;
          event.values[0] = constrain(-relativeVelocity / (2.0f * kBallMaxVelocity),
                                      0.0f, 1.0f);
          enqueue(event);
        }
      }
    }
  }
  ballPairContacts_ = currentContacts;

  auto wallCollision = [&](uint8_t ball, uint8_t wall, float velocity) {
    UiEvent event;
    event.type = UiEventType::WallCollision;
    event.index = ball;
    event.numbers[0] = wall;
    event.values[0] = constrain(fabsf(velocity) / kBallMaxVelocity, 0.0f, 1.0f);
    enqueue(event);
  };

  for (uint8_t index = 0; index < kMaxBalls; ++index) {
    BallState &ball = balls_[index];
    if (!ball.active) continue;
    if (ball.x < 0.0f) {
      wallCollision(index, 0, ball.velocityX);
      ball.x = -ball.x;
      ball.velocityX = fabsf(ball.velocityX) * ballBounciness_;
    } else if (ball.x > 1.0f) {
      wallCollision(index, 1, ball.velocityX);
      ball.x = 2.0f - ball.x;
      ball.velocityX = -fabsf(ball.velocityX) * ballBounciness_;
    }
    if (ball.y < 0.0f) {
      wallCollision(index, 2, ball.velocityY);
      ball.y = -ball.y;
      ball.velocityY = fabsf(ball.velocityY) * ballBounciness_;
    } else if (ball.y > 1.0f) {
      wallCollision(index, 3, ball.velocityY);
      ball.y = 2.0f - ball.y;
      ball.velocityY = -fabsf(ball.velocityY) * ballBounciness_;
    }
    ball.x = constrain(ball.x, 0.0f, 1.0f);
    ball.y = constrain(ball.y, 0.0f, 1.0f);
    ball.velocityX = constrain(ball.velocityX, -kBallMaxVelocity, kBallMaxVelocity);
    ball.velocityY = constrain(ball.velocityY, -kBallMaxVelocity, kBallMaxVelocity);
  }
  if (anyActive) ballDirty_ = true;
}

void UserInterface::beginPendulumDraw(int16_t x, int16_t y) {
  const float touchX = constrain(static_cast<float>(x - kControlX), 0.0f,
                                 static_cast<float>(kControlW - 1));
  const float touchY = constrain(static_cast<float>(y - kControlY), 0.0f,
                                 static_cast<float>(kControlH - 1));
  pendulumResetHoldEligible_ = false;
  pendulumCreatedOnTouch_ = -1;

  // Fixtures take priority over weights so a fixture can always be deleted,
  // even when several pendulums overlap.
  for (uint8_t index = 0; index < kMaxPendulums; ++index) {
    PendulumState &pendulum = pendulums_[index];
    if (!pendulum.active) continue;
    const float pivotDx = touchX - pendulum.ax;
    const float pivotDy = touchY - pendulum.ay;
    if (pivotDx * pivotDx + pivotDy * pivotDy <=
        kPendulumPivotTouchRadius * kPendulumPivotTouchRadius) {
      pendulum.active = false;
      pendulumDrawing_ = false;
      activePendulumIndex_ = -1;
      pendulum.angularVelocity = 0.0f;
      pendulum.historyInitialized = false;
      pendulum.motionDirection = 0;
      pendulum.lastUpdateUs = 0;
      pendulumDirty_ = true;
      enqueuePendulumActive(index, false);
      return;
    }
  }

  for (uint8_t index = 0; index < kMaxPendulums; ++index) {
    PendulumState &pendulum = pendulums_[index];
    if (!pendulum.active) continue;
    const float bobX = pendulum.ax + sinf(pendulum.angle) * pendulum.length;
    const float bobY = pendulum.ay + cosf(pendulum.angle) * pendulum.length;
    const float bobDx = touchX - bobX;
    const float bobDy = touchY - bobY;
    if (bobDx * bobDx + bobDy * bobDy <=
        kPendulumBobTouchRadius * kPendulumBobTouchRadius) {
      pendulumDrawing_ = true;
      activePendulumIndex_ = index;
      pendulum.angularVelocity = 0.0f;
      pendulum.historyInitialized = false;
      pendulum.motionDirection = 0;
      pendulum.lastUpdateUs = 0;
      movePendulumDraw(x, y, true);
      return;
    }
  }

  // An empty-space gesture may become either a new pendulum or, if held, a
  // reset of all existing pendulums to their gravity-defined centre.
  pendulumResetHoldEligible_ = true;
  int8_t freeIndex = -1;
  for (uint8_t index = 0; index < kMaxPendulums; ++index) {
    if (!pendulums_[index].active) {
      freeIndex = index;
      break;
    }
  }
  if (freeIndex < 0) {
    pendulumDrawing_ = false;
    activePendulumIndex_ = -1;
    return;
  }

  PendulumState &pendulum = pendulums_[freeIndex];
  const float pivotMargin = kPendulumMinimumLength + kPendulumBobRadius;
  pendulum.ax = constrain(touchX, pivotMargin,
                          static_cast<float>(kControlW - 1) - pivotMargin);
  pendulum.ay = constrain(touchY, static_cast<float>(kPendulumBobRadius),
                          static_cast<float>(kControlH - 1) - pivotMargin);
  pendulum.length = kPendulumMinimumLength;
  pendulum.angle = 0.0f;
  pendulum.angularVelocity = 0.0f;
  pendulum.active = true;
  pendulumDrawing_ = true;
  activePendulumIndex_ = freeIndex;
  pendulumCreatedOnTouch_ = freeIndex;
  pendulum.historyInitialized = false;
  pendulum.motionDirection = 0;
  pendulum.lastUpdateUs = 0;
  pendulum.lastStreamMs = 0;
  pendulum.lastCentrePingMs = 0;
  pendulum.lastTurnPingMs = 0;
  enqueuePendulumActive(freeIndex, true);
  movePendulumDraw(x, y, true);
}

void UserInterface::movePendulumDraw(int16_t x, int16_t y, bool forceSend) {
  if (!pendulumDrawing_ || activePendulumIndex_ < 0) return;
  PendulumState &pendulum = pendulums_[activePendulumIndex_];
  if (!pendulum.active) return;
  const float targetX = constrain(static_cast<float>(x - kControlX), 0.0f,
                                  static_cast<float>(kControlW - 1));
  const float targetY = constrain(static_cast<float>(y - kControlY), 0.0f,
                                  static_cast<float>(kControlH - 1));
  float dx = targetX - pendulum.ax;
  float dy = targetY - pendulum.ay;
  float distance = sqrtf(dx * dx + dy * dy);
  if (distance < 0.001f) {
    dx = 0.0f;
    dy = 1.0f;
    distance = 1.0f;
  }
  // Limit the arm along the direction being drawn. The previous calculation
  // reserved room for a complete circle around A, unnecessarily shortening a
  // mostly downward pendulum to the nearest side-wall distance.
  const float unitX = dx / distance;
  const float unitY = dy / distance;
  const float minimumX = kPendulumBobRadius;
  const float maximumX = kControlW - 1 - kPendulumBobRadius;
  const float minimumY = kPendulumBobRadius;
  const float maximumY = kControlH - 1 - kPendulumBobRadius;
  float visibleLength = kPendulumMaximumLength;
  if (unitX > 0.001f) {
    visibleLength = min(visibleLength, (maximumX - pendulum.ax) / unitX);
  } else if (unitX < -0.001f) {
    visibleLength = min(visibleLength, (pendulum.ax - minimumX) / -unitX);
  }
  if (unitY > 0.001f) {
    visibleLength = min(visibleLength, (maximumY - pendulum.ay) / unitY);
  } else if (unitY < -0.001f) {
    visibleLength = min(visibleLength, (pendulum.ay - minimumY) / -unitY);
  }
  const float maximumLength = constrain(visibleLength, kPendulumMinimumLength,
                                        kPendulumMaximumLength);
  pendulum.length = constrain(distance, kPendulumMinimumLength, maximumLength);
  pendulum.angle = atan2f(dx, dy);
  pendulum.angularVelocity = 0.0f;
  pendulumDirty_ = true;
  enqueuePendulumState(activePendulumIndex_, forceSend);
}

void UserInterface::finishPendulumDraw() {
  if (!pendulumDrawing_ || activePendulumIndex_ < 0) {
    pendulumResetHoldEligible_ = false;
    pendulumCreatedOnTouch_ = -1;
    return;
  }
  const uint8_t index = activePendulumIndex_;
  PendulumState &pendulum = pendulums_[index];
  pendulumDrawing_ = false;
  activePendulumIndex_ = -1;
  pendulum.angularVelocity = 0.0f;
  pendulum.historyInitialized = false;
  pendulum.motionDirection = 0;
  pendulum.lastUpdateUs = 0;
  pendulumDirty_ = true;
  enqueuePendulumState(index, true);
  pendulumResetHoldEligible_ = false;
  pendulumCreatedOnTouch_ = -1;
}

void UserInterface::resetPendulumsToCentre() {
  // Undo the provisional pendulum created by this empty-space gesture so the
  // reset does not alter the number of pendulums already on the page.
  if (pendulumCreatedOnTouch_ >= 0) {
    PendulumState &created = pendulums_[pendulumCreatedOnTouch_];
    if (created.active) {
      created = PendulumState{};
      enqueuePendulumActive(pendulumCreatedOnTouch_, false);
    }
  }

  pendulumDrawing_ = false;
  activePendulumIndex_ = -1;
  pendulumResetHoldEligible_ = false;
  pendulumCreatedOnTouch_ = -1;
  for (uint8_t index = 0; index < kMaxPendulums; ++index) {
    PendulumState &pendulum = pendulums_[index];
    if (!pendulum.active) continue;
    pendulum.angle = pendulumGravityAngle_;
    pendulum.angularVelocity = 0.0f;
    pendulum.historyInitialized = false;
    pendulum.motionDirection = 0;
    pendulum.lastUpdateUs = 0;
    enqueuePendulumState(index, true);
  }
  pendulumDirty_ = true;
}

void UserInterface::enqueuePendulumState(uint8_t index, bool force) {
  if (index >= kMaxPendulums || !pendulums_[index].active) return;
  PendulumState &pendulum = pendulums_[index];
  const uint32_t now = millis();
  if (!force && now - pendulum.lastStreamMs < kPendulumStreamIntervalMs) return;
  pendulum.lastStreamMs = now;

  const float bobX = pendulum.ax + sinf(pendulum.angle) * pendulum.length;
  const float bobY = pendulum.ay + cosf(pendulum.angle) * pendulum.length;
  UiEvent event;
  event.type = UiEventType::PendulumState;
  event.index = index;
  event.values[0] = pendulum.ax / (kControlW - 1);
  event.values[1] = 1.0f - pendulum.ay / (kControlH - 1);
  event.values[2] = bobX / (kControlW - 1);
  event.values[3] = 1.0f - bobY / (kControlH - 1);
  event.values[4] = pendulum.angle * 180.0f / kPi;
  event.values[5] = pendulum.angularVelocity * 180.0f / kPi;
  enqueue(event);
}

void UserInterface::enqueuePendulumPing(uint8_t index, uint8_t ping) {
  UiEvent event;
  event.type = UiEventType::PendulumPing;
  event.index = index;
  event.numbers[0] = ping;
  enqueue(event);
}

void UserInterface::enqueuePendulumActive(uint8_t index, bool active) {
  UiEvent event;
  event.type = UiEventType::PendulumActive;
  event.index = index;
  event.state = active;
  enqueue(event);
}

void UserInterface::updatePendulum(const ImuFrame &imu) {
  const uint32_t now = micros();
  // Accelerometers measure support force, so negate logical X to get screen-right
  // gravity and convert logical screen-up Y into display-down coordinates.
  const float gravityX = -imu.accel[0] * ballGravity_ * kPendulumGravityPixelScale;
  const float gravityY = imu.accel[1] * ballGravity_ * kPendulumGravityPixelScale;
  const uint32_t nowMs = millis();
  const float gravityMagnitude = sqrtf(gravityX * gravityX + gravityY * gravityY) /
                                 (ballGravity_ * kPendulumGravityPixelScale);
  if (gravityMagnitude >= kPendulumGravityThreshold) {
    pendulumGravityAngle_ = atan2f(gravityX, gravityY);
  }

  for (uint8_t index = 0; index < kMaxPendulums; ++index) {
    PendulumState &pendulum = pendulums_[index];
    if (!pendulum.active) {
      pendulum.lastUpdateUs = 0;
      continue;
    }
    if (pendulumDrawing_ && activePendulumIndex_ == index) {
      pendulum.lastUpdateUs = now;
      continue;
    }
    if (pendulum.lastUpdateUs == 0) {
      pendulum.lastUpdateUs = now;
      continue;
    }
    const uint32_t elapsedUs = static_cast<uint32_t>(now - pendulum.lastUpdateUs);
    if (elapsedUs < 10000) continue;
    pendulum.lastUpdateUs = now;
    const float dt = constrain(elapsedUs / 1000000.0f, 0.001f, 0.05f);

    const float tangentX = cosf(pendulum.angle);
    const float tangentY = -sinf(pendulum.angle);
    const float angularAcceleration =
        (gravityX * tangentX + gravityY * tangentY) / pendulum.length;
    pendulum.angularVelocity += angularAcceleration * dt;
    pendulum.angularVelocity *= expf(-kPendulumDamping * dt);
    pendulum.angularVelocity = constrain(pendulum.angularVelocity,
                                         -kPendulumMaximumAngularVelocity,
                                         kPendulumMaximumAngularVelocity);
    pendulum.angle = wrapRadians(pendulum.angle + pendulum.angularVelocity * dt);

    if (gravityMagnitude >= kPendulumGravityThreshold) {
      const float gravityAngle = atan2f(gravityX, gravityY);
      const float relativeAngle = wrapRadians(pendulum.angle - gravityAngle);
      const bool crossedCentre = pendulum.historyInitialized &&
          ((pendulum.previousRelativeAngle < 0.0f && relativeAngle >= 0.0f) ||
           (pendulum.previousRelativeAngle > 0.0f && relativeAngle <= 0.0f)) &&
          fabsf(relativeAngle - pendulum.previousRelativeAngle) < kPi &&
          fabsf(pendulum.angularVelocity) >= kPendulumCentreSpeedThreshold;
      if (crossedCentre &&
          nowMs - pendulum.lastCentrePingMs >= kPendulumPingCooldownMs) {
        enqueuePendulumPing(index, 0);
        pendulum.lastCentrePingMs = nowMs;
      }
      pendulum.previousRelativeAngle = relativeAngle;
      pendulum.historyInitialized = true;
    } else {
      pendulum.historyInitialized = false;
    }

    if (pendulum.angularVelocity > kPendulumDirectionThreshold) {
      if (pendulum.motionDirection < 0 &&
          nowMs - pendulum.lastTurnPingMs >= kPendulumPingCooldownMs) {
        enqueuePendulumPing(index, 1);
        pendulum.lastTurnPingMs = nowMs;
      }
      pendulum.motionDirection = 1;
    } else if (pendulum.angularVelocity < -kPendulumDirectionThreshold) {
      if (pendulum.motionDirection > 0 &&
          nowMs - pendulum.lastTurnPingMs >= kPendulumPingCooldownMs) {
        enqueuePendulumPing(index, 2);
        pendulum.lastTurnPingMs = nowMs;
      }
      pendulum.motionDirection = -1;
    }

    pendulumDirty_ = true;
    enqueuePendulumState(index);
  }
}

float UserInterface::nextParticleRandom() {
  particleRandomState_ = particleRandomState_ * 1664525u + 1013904223u;
  return static_cast<float>((particleRandomState_ >> 8) & 0x00ffffffu) /
         static_cast<float>(0x01000000u);
}

void UserInterface::updateParticles(float micEnergy, const ImuFrame &imu) {
  const uint32_t now = micros();
  if (lastParticleUpdateUs_ == 0) {
    lastParticleUpdateUs_ = now;
    return;
  }
  const uint32_t elapsedUs = static_cast<uint32_t>(now - lastParticleUpdateUs_);
  if (elapsedUs < 10000) return;
  lastParticleUpdateUs_ = now;
  const float dt = constrain(elapsedUs / 1000000.0f, 0.001f, 0.05f);

  // Use the same verified screen-frame gravity mapping as the ball page.
  // Microphone energy controls density; tilt controls the path independently.
  const float tiltX = constrain(-imu.accel[0], -1.5f, 1.5f);
  const float tiltY = constrain(imu.accel[1], -1.5f, 1.5f);
  const float accelerationX = tiltX * kParticleTiltAcceleration;
  const float accelerationY = tiltY * kParticleTiltAcceleration;
  const float drag = expf(-0.12f * dt);

  bool anyActive = false;
  bool visualChanged = false;
  for (ParticleState &particle : particles_) {
    if (!particle.active) continue;
    visualChanged = true;
    particle.age += dt;
    particle.velocityX += accelerationX * dt;
    particle.velocityY += accelerationY * dt;
    particle.x += particle.velocityX * dt;
    particle.y += particle.velocityY * dt;
    particle.velocityX *= drag;
    particle.velocityY *= drag;
    particle.velocityX = constrain(particle.velocityX, -kParticleMaximumVelocity,
                                   kParticleMaximumVelocity);
    particle.velocityY = constrain(particle.velocityY, -kParticleMaximumVelocity,
                                   kParticleMaximumVelocity);
    const float halfSize = particle.size * 0.5f;
    const float minimumX = kLineWidth + halfSize;
    const float maximumX = kControlW - 1 - kLineWidth - halfSize;
    const float minimumY = kLineWidth + halfSize;
    const float maximumY = kControlH - 1 - kLineWidth - halfSize;
    int8_t wall = -1;
    if (particle.x <= minimumX) {
      wall = 0;
    } else if (particle.x >= maximumX) {
      wall = 1;
    } else if (particle.y <= minimumY) {
      wall = 2;
    } else if (particle.y >= maximumY) {
      wall = 3;
    }
    if (wall >= 0 || particle.age >= particle.lifetime) {
      if (wall >= 0) {
        const float normalizedSize =
            static_cast<float>(particle.size - kParticleMinimumSize) /
            static_cast<float>(kParticleMaximumSize - kParticleMinimumSize);
        enqueueParticleWall(static_cast<uint8_t>(wall), normalizedSize);
      }
      particle.active = false;
      continue;
    }
    anyActive = true;
  }

  const float density = constrain(micEnergy, 0.0f, 1.0f);
  particleSpawnAccumulator_ += density * kParticleMaximumSpawnRate * dt;
  uint8_t particlesToSpawn = min(static_cast<int>(particleSpawnAccumulator_), 4);
  particleSpawnAccumulator_ -= particlesToSpawn;
  while (particlesToSpawn > 0) {
    ParticleState *available = nullptr;
    for (ParticleState &particle : particles_) {
      if (!particle.active) {
        available = &particle;
        break;
      }
    }
    if (!available) {
      particleSpawnAccumulator_ = 0.0f;
      break;
    }
    const float angle = 0.28f + nextParticleRandom() * 0.85f;
    const float speed = kParticleMinimumSpeed +
        nextParticleRandom() * (kParticleMaximumSpeed - kParticleMinimumSpeed) +
        density * 45.0f;
    *available = ParticleState{};
    available->active = true;
    available->size = kParticleMinimumSize + static_cast<uint8_t>(
        nextParticleRandom() * (kParticleMaximumSize - kParticleMinimumSize + 1));
    available->x = kLineWidth + available->size * 0.5f + 2.0f;
    available->y = kControlH - 1 - kLineWidth - available->size * 0.5f -
                   2.0f - nextParticleRandom() * 16.0f;
    available->velocityX = cosf(angle) * speed;
    available->velocityY = -sinf(angle) * speed;
    available->velocityX += accelerationX * 0.22f;
    available->velocityY += accelerationY * 0.22f;
    available->lifetime = kParticleMinimumLifetime +
                          nextParticleRandom() * kParticleLifetimeRange;
    anyActive = true;
    visualChanged = true;
    --particlesToSpawn;
  }

  if (visualChanged || anyActive || density > 0.0f) particleDirty_ = true;
}

void UserInterface::enqueueParticleWall(uint8_t wall, float normalizedSize) {
  UiEvent event;
  event.type = UiEventType::ParticleWall;
  event.index = wall;
  event.values[0] = constrain(normalizedSize, 0.0f, 1.0f);
  enqueue(event);
}

void UserInterface::enqueue(const UiEvent &event) {
  const uint8_t next = (eventWrite_ + 1) % kEventQueueSize;
  if (next == eventRead_) return;
  events_[eventWrite_] = event;
  eventWrite_ = next;
}

bool UserInterface::popEvent(UiEvent &event) {
  if (eventRead_ == eventWrite_) return false;
  event = events_[eventRead_];
  eventRead_ = (eventRead_ + 1) % kEventQueueSize;
  return true;
}

bool UserInterface::isControlActive(MessageType type, uint8_t index) const {
  if (!touching_) return false;
  if (type == MessageType::Xy) return activeTouch_ == TouchTarget::Xy;
  if (type == MessageType::Fader && index < 4) {
    return activeTouch_ == static_cast<TouchTarget>(
        static_cast<uint8_t>(TouchTarget::Fader0) + index);
  }
  if (type == MessageType::Button && index < 4) {
    return activeTouch_ == static_cast<TouchTarget>(
        static_cast<uint8_t>(TouchTarget::Button0) + index);
  }
  if (type == MessageType::Note && index >= keyboardBaseMidiNote_ &&
      index <= keyboardBaseMidiNote_ + 12) {
    return activeTouch_ == TouchTarget::Keyboard &&
           activeKeyboardKey_ == static_cast<int8_t>(index - keyboardBaseMidiNote_);
  }
  if (type == MessageType::Spectrum) return activeTouch_ == TouchTarget::Spectrum ||
                                            activeTouch_ == TouchTarget::FftCapture;
  return false;
}

void UserInterface::drawAll(const ControlState &state, bool wifiConnected, bool bleEnabled,
                            bool bleConnected, bool imuOutputEnabled) {
  display_->fillScreen(backgroundColor_);
  drawTop(wifiConnected, bleEnabled, bleConnected, imuOutputEnabled);
  xyPartialValid_ = false;
  keyboardPartialValid_ = false;
  spectrumPartialValid_ = false;
  drawCurrentPage(state);
  drawPageButton();
}

void UserInterface::drawTop(bool wifiConnected, bool bleEnabled, bool bleConnected,
                            bool imuOutputEnabled) {
  Arduino_GFX *target = topCanvas_ ? static_cast<Arduino_GFX *>(topCanvas_)
                                   : static_cast<Arduino_GFX *>(display_);
  target->fillRect(0, 0, kScreenWidth,
                   topCanvas_ ? kTopCanvasHeight : kTopHeight, backgroundColor_);
  target->fillRect(0, kTopHeight - kLineWidth, kScreenWidth, kLineWidth, mutedColor_);

  const uint16_t wifiColor = wifiConnected ? rgb565(70, 224, 132) : mutedColor_;
  target->fillRoundRect(14 + kStatusBadgeShiftX, 6, 35, 31, 6, wifiColor);
  const uint16_t wifiGlyph = rgb565(0, 0, 0);
  drawWifiGlyph(target, 31 + kStatusBadgeShiftX, 28, wifiGlyph);

  // Give each status control its own stable colour identity: green for
  // settings/Wi-Fi, amber for BLE, and pink for raw IMU output.
  const uint16_t bleColor = bleEnabled ? rgb565(255, 178, 46) : mutedColor_;
  target->fillRoundRect(51 + kStatusBadgeShiftX, 6, 36, 31, 6, bleColor);
  const uint16_t bleGlyph = rgb565(0, 0, 0);
  for (uint8_t thickness = 0; thickness < 2; ++thickness) {
    const int16_t offset = thickness;
    target->drawLine(69 + kStatusBadgeShiftX + offset, 10,
                     69 + kStatusBadgeShiftX + offset, 33, bleGlyph);
    target->drawLine(69 + kStatusBadgeShiftX + offset, 10,
                     78 + kStatusBadgeShiftX + offset, 18, bleGlyph);
    target->drawLine(78 + kStatusBadgeShiftX + offset, 18,
                     64 + kStatusBadgeShiftX + offset, 29, bleGlyph);
    target->drawLine(64 + kStatusBadgeShiftX + offset, 14,
                     78 + kStatusBadgeShiftX + offset, 25, bleGlyph);
    target->drawLine(78 + kStatusBadgeShiftX + offset, 25,
                     69 + kStatusBadgeShiftX + offset, 33, bleGlyph);
  }

  const uint16_t imuColor = imuOutputEnabled ? rgb565(255, 72, 176) : mutedColor_;
  target->fillRoundRect(89 + kStatusBadgeShiftX, 6, 36, 31, 6, imuColor);
  const uint16_t imuGlyph = rgb565(0, 0, 0);
  const int16_t imuCenterX = 107 + kStatusBadgeShiftX;
  const int16_t imuCenterY = 22;
  target->fillCircle(imuCenterX, imuCenterY, 3, imuGlyph);
  for (uint8_t thickness = 0; thickness < 2; ++thickness) {
    target->drawLine(imuCenterX, imuCenterY - thickness,
                     imuCenterX + 11, imuCenterY - thickness, imuGlyph);
    target->drawLine(imuCenterX + thickness, imuCenterY,
                     imuCenterX + thickness, imuCenterY - 11, imuGlyph);
    target->drawLine(imuCenterX - thickness, imuCenterY + thickness,
                     imuCenterX - 8 - thickness, imuCenterY + 8 + thickness, imuGlyph);
  }
  target->fillTriangle(imuCenterX + 12, imuCenterY,
                       imuCenterX + 7, imuCenterY - 4,
                       imuCenterX + 7, imuCenterY + 4, imuGlyph);
  target->fillTriangle(imuCenterX, imuCenterY - 12,
                       imuCenterX - 4, imuCenterY - 7,
                       imuCenterX + 4, imuCenterY - 7, imuGlyph);
  target->fillTriangle(imuCenterX - 9, imuCenterY + 9,
                       imuCenterX - 3, imuCenterY + 8,
                       imuCenterX - 8, imuCenterY + 3, imuGlyph);

  const int16_t bx = 313;
  drawThickRoundRect(target, bx, 12, 30, 17, 3, foregroundColor_);
  target->fillRect(bx + 30, 17, 3, 7, foregroundColor_);
  if (batteryPercent_ >= 0) {
    const int16_t fill = map(constrain(batteryPercent_, 0, 100), 0, 100, 0, 24);
    target->fillRect(bx + 3, 15, fill, 11, foregroundColor_);
  } else {
    target->drawFastHLine(bx + 6, 20, 18, mutedColor_);
  }
  if (charging_) {
    target->drawLine(bx + 16, 13, bx + 12, 21, backgroundColor_);
    target->drawLine(bx + 12, 21, bx + 18, 21, backgroundColor_);
    target->drawLine(bx + 18, 21, bx + 14, 28, backgroundColor_);
  }
  if (topCanvas_) topCanvas_->flush();
  drawnWifiConnected_ = wifiConnected;
  drawnBleEnabled_ = bleEnabled;
  drawnBleConnected_ = bleConnected;
  drawnImuOutputEnabled_ = imuOutputEnabled;
  drawnBatteryPercent_ = batteryPercent_;
  drawnCharging_ = charging_;
  topStateValid_ = true;
}

void UserInterface::drawLayoutStatusOverlay(Arduino_GFX *target,
                                            bool wifiConnected) {
  if (!target) return;
  const uint16_t black = rgb565(0, 0, 0);
  const uint16_t white = rgb565(244, 244, 244);
  const uint16_t wifiColor = wifiConnected ? rgb565(70, 224, 132)
                                            : rgb565(110, 110, 110);

  // Opaque compact badges keep the indicators readable over arbitrary uploaded
  // layouts while leaving almost the entire TouchOSC surface untouched.
  target->fillRoundRect(4, 4, 40, 36, 8, black);
  target->drawRoundRect(4, 4, 40, 36, 8, wifiColor);
  drawWifiGlyph(target, 24, 30, wifiColor);

  if (touchOscLayout_.pageCount() > 1) {
    target->fillRoundRect(138, 4, 92, 36, 8, black);
    target->drawRoundRect(138, 4, 92, 36, 8, white);
    const String pageLabel = String(touchOscLayout_.activePage() + 1) + "/" +
                             String(touchOscLayout_.pageCount());
    drawCenteredText(target, pageLabel, 138, 15, 92, 2, white);
  }

  target->fillRoundRect(320, 4, 44, 36, 8, black);
  const int16_t bx = 326;
  const int16_t by = 14;
  for (uint8_t inset = 0; inset < 2; ++inset) {
    target->drawRoundRect(bx + inset, by + inset, 29 - inset * 2,
                          17 - inset * 2, 3, white);
  }
  target->fillRect(bx + 29, by + 5, 3, 7, white);
  if (batteryPercent_ >= 0) {
    const int16_t fill = map(constrain(batteryPercent_, 0, 100), 0, 100, 0, 23);
    const uint16_t fillColor = batteryPercent_ <= 15
        ? rgb565(255, 72, 72)
        : (batteryPercent_ <= 35 ? rgb565(255, 178, 46)
                                 : rgb565(70, 224, 132));
    if (fill > 0) target->fillRect(bx + 3, by + 3, fill, 11, fillColor);
  } else {
    target->drawFastHLine(bx + 7, by + 8, 15, rgb565(110, 110, 110));
  }
  if (charging_) {
    const uint16_t bolt = rgb565(255, 220, 72);
    target->drawLine(bx + 15, by + 1, bx + 11, by + 9, bolt);
    target->drawLine(bx + 11, by + 9, bx + 17, by + 9, bolt);
    target->drawLine(bx + 17, by + 9, bx + 13, by + 16, bolt);
  }

  drawnLayoutWifiConnected_ = wifiConnected;
  drawnLayoutBatteryPercent_ = batteryPercent_;
  drawnLayoutCharging_ = charging_;
  layoutOverlayValid_ = true;
}

void UserInterface::drawLandingScreen(Arduino_GFX *target,
                                      bool wifiConnected) {
  if (!target) return;
  const uint16_t black = rgb565(0, 0, 0);
  const uint16_t white = rgb565(244, 244, 244);
  const uint16_t muted = rgb565(128, 128, 128);
  const uint16_t ready = rgb565(70, 224, 132);

  target->fillScreen(black);
  drawCenteredText(target, "TOUCHOSC", 0, 108, kScreenWidth, 3, white);
  drawCenteredText(target, "PARSER MINI", 0, 150, kScreenWidth, 3, white);
  target->fillRect(94, 202, 180, 3, muted);

  if (wifiConnected) {
    drawCenteredText(target, "READY TO UPLOAD", 0, 246, kScreenWidth, 2, ready);
    drawCenteredText(target, "OPEN IN A BROWSER", 0, 287, kScreenWidth, 1, muted);
    drawCenteredText(target, String("http://") + savedDeviceName_ + ".local",
                     0, 310, kScreenWidth, 1, white);
  } else {
    drawCenteredText(target, "WI-FI NOT CONNECTED", 0, 246,
                     kScreenWidth, 2, muted);
    drawCenteredText(target, "TAP THE WI-FI ICON", 0, 287,
                     kScreenWidth, 1, white);
  }

  drawCenteredText(target, "INSTALL A LAYOUT TO BEGIN", 0, 374,
                   kScreenWidth, 1, muted);
  drawLayoutStatusOverlay(target, wifiConnected);
}

void UserInterface::drawCurrentPage(const ControlState &state) {
  if (controlPage_ != kPageXy) xyPartialValid_ = false;
  if (controlPage_ != kPageKeyboard) keyboardPartialValid_ = false;
  if (controlPage_ != kPageSpectrum) spectrumPartialValid_ = false;
  if (xyCanvas_) {
    // Paint the two invisible alignment rows as well as the visible region so
    // no pixels from a previous page can survive in the padded transfer.
    xyCanvas_->fillRect(0, 0, kControlW, kControlCanvasH, backgroundColor_);
  }
  if (controlPage_ == kPageKeyboard) {
    drawKeyboard(state);
  } else if (controlPage_ == kPageButtons) {
    drawButtons(state);
  } else if (controlPage_ == kPageXy) {
    drawXy(state);
  } else if (controlPage_ == kPageFaders) {
    drawFaders(state);
  } else if (controlPage_ == kPageBalls) {
    drawBall();
  } else if (controlPage_ == kPagePendulums) {
    drawPendulum();
  } else if (controlPage_ == kPageParticles) {
    drawParticles();
  } else {
    drawSpectrum(state);
  }

  drawnActiveTouch_ = TouchTarget::None;
  if (touching_) {
    if (controlPage_ == kPageXy && activeTouch_ == TouchTarget::Xy) {
      drawnActiveTouch_ = activeTouch_;
    } else if (controlPage_ == kPageFaders &&
               activeTouch_ >= TouchTarget::Fader0 && activeTouch_ <= TouchTarget::Fader3) {
      drawnActiveTouch_ = activeTouch_;
    } else if (controlPage_ == kPageButtons &&
               activeTouch_ >= TouchTarget::Button0 && activeTouch_ <= TouchTarget::Button3) {
      drawnActiveTouch_ = activeTouch_;
    }
  }
}

void UserInterface::drawXy(const ControlState &state) {
  const int16_t travelW = kControlW - 1 - 2 * kControlInset;
  const int16_t travelH = kControlH - 1 - 2 * kControlInset;
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, white);
  const int16_t dotX = originX + kControlInset + roundf(state.xyX * travelW);
  const int16_t dotY = originY + kControlInset + roundf((1.0f - state.xyY) * travelH);
  target->fillRect(dotX - kXyCrossWidth / 2, originY, kXyCrossWidth,
                   kControlH, backgroundColor_);
  target->fillRect(originX, dotY - kXyCrossWidth / 2, kControlW,
                   kXyCrossWidth, backgroundColor_);
  const bool xyPressed = touching_ && activeTouch_ == TouchTarget::Xy;
  target->fillRect(dotX - kXyCrossWidth / 2, dotY - kXyCrossWidth / 2,
                   kXyCrossWidth, kXyCrossWidth,
                   xyPressed ? backgroundColor_ : white);
  if (xyCanvas_) {
    if (!xyPartialValid_) {
      xyCanvas_->flush();
    } else {
      const int16_t oldX = kControlInset +
          roundf(drawnState_.xyX * travelW);
      const int16_t oldY = kControlCanvasInsetY + kControlInset +
          roundf((1.0f - drawnState_.xyY) * travelH);
      const int16_t halfCross = kXyCrossWidth / 2;

      // Horizontal pixels are contiguous in the canvas, so update the union
      // of nearby positions with one short transfer. Large jumps use two
      // bands rather than rewriting the intervening area.
      const int16_t horizontalStart = min(oldY, dotY) - halfCross;
      const int16_t horizontalEnd = max(oldY, dotY) + halfCross;
      if (horizontalEnd - horizontalStart + 1 <= kXyCrossWidth * 2) {
        flushXyRegion(0, horizontalStart, kControlW,
                      horizontalEnd - horizontalStart + 1);
      } else {
        // Put the replacement on screen before restoring the old location.
        // A large jump can therefore show a momentary second line, but never a
        // conspicuous white gap where the moving line should be.
        flushXyRegion(0, dotY - halfCross, kControlW, kXyCrossWidth);
        flushXyRegion(0, oldY - halfCross, kControlW, kXyCrossWidth);
      }

      // Pack the much narrower vertical update into a contiguous buffer. It
      // cuts the transfer from a complete 328-pixel row to roughly 25 pixels,
      // greatly shortening the interval in which panel scan tearing can show.
      const int16_t verticalStart = min(oldX, dotX) - halfCross;
      const int16_t verticalEnd = max(oldX, dotX) + halfCross;
      if (verticalEnd - verticalStart + 1 <= kXyStripBufferWidth - 2) {
        flushXyRegion(verticalStart, 0,
                      verticalEnd - verticalStart + 1, kControlCanvasH);
      } else {
        flushXyRegion(dotX - halfCross, 0, kXyCrossWidth, kControlCanvasH);
        flushXyRegion(oldX - halfCross, 0, kXyCrossWidth, kControlCanvasH);
      }
    }
    xyPartialValid_ = true;
  } else {
    xyPartialValid_ = false;
  }
}

void UserInterface::flushLayoutRegion(int16_t x, int16_t y,
                                      int16_t width, int16_t height) {
  if (!setupCanvas_ || !display_ || width <= 0 || height <= 0) return;

  int16_t x1 = constrain(x, static_cast<int16_t>(0),
                         static_cast<int16_t>(kScreenWidth - 1));
  int16_t y1 = constrain(y, static_cast<int16_t>(0),
                         static_cast<int16_t>(kScreenHeight - 1));
  int16_t x2 = constrain(static_cast<int16_t>(x + width - 1),
                         static_cast<int16_t>(0),
                         static_cast<int16_t>(kScreenWidth - 1));
  int16_t y2 = constrain(static_cast<int16_t>(y + height - 1),
                         static_cast<int16_t>(0),
                         static_cast<int16_t>(kScreenHeight - 1));
  if (x2 < x1 || y2 < y1) return;

  // The CO5300 is most reliable with even starts and odd inclusive ends.
  x1 &= ~1;
  y1 &= ~1;
  x2 = min<int16_t>(kScreenWidth - 1, x2 | 1);
  y2 = min<int16_t>(kScreenHeight - 1, y2 | 1);
  const int16_t alignedWidth = x2 - x1 + 1;
  const int16_t alignedHeight = y2 - y1 + 1;
  if (!layoutTransferBuffer_ ||
      (alignedWidth == kScreenWidth && alignedHeight == kScreenHeight)) {
    setupCanvas_->flush();
    return;
  }

  uint16_t *framebuffer = setupCanvas_->getFramebuffer();
  for (int16_t row = 0; row < alignedHeight; ++row) {
    memcpy(layoutTransferBuffer_ + row * alignedWidth,
           framebuffer + (y1 + row) * kScreenWidth + x1,
           alignedWidth * sizeof(uint16_t));
  }
  display_->draw16bitRGBBitmap(x1, y1, layoutTransferBuffer_,
                               alignedWidth, alignedHeight);
}

void UserInterface::flushXyRegion(int16_t x, int16_t y,
                                  int16_t width, int16_t height) {
  if (!xyCanvas_ || !display_ || width <= 0 || height <= 0) return;

  int16_t x1 = constrain(x, static_cast<int16_t>(0),
                         static_cast<int16_t>(kControlW - 1));
  int16_t y1 = constrain(y, static_cast<int16_t>(0),
                         static_cast<int16_t>(kControlCanvasH - 1));
  int16_t x2 = constrain(static_cast<int16_t>(x + width - 1),
                         static_cast<int16_t>(0),
                         static_cast<int16_t>(kControlW - 1));
  int16_t y2 = constrain(static_cast<int16_t>(y + height - 1),
                         static_cast<int16_t>(0),
                         static_cast<int16_t>(kControlCanvasH - 1));
  if (x2 < x1 || y2 < y1) return;

  // Match the CO5300's preferred even start / odd inclusive end boundaries.
  int16_t globalX1 = (kControlX + x1) & ~1;
  int16_t globalY1 = (kControlCanvasY + y1) & ~1;
  int16_t globalX2 = (kControlX + x2) | 1;
  int16_t globalY2 = (kControlCanvasY + y2) | 1;
  globalX1 = max<int16_t>(kControlX, globalX1);
  globalY1 = max<int16_t>(kControlCanvasY, globalY1);
  globalX2 = min<int16_t>(kControlX + kControlW - 1, globalX2);
  globalY2 = min<int16_t>(kControlCanvasY + kControlCanvasH - 1, globalY2);
  x1 = globalX1 - kControlX;
  y1 = globalY1 - kControlCanvasY;
  const int16_t alignedWidth = globalX2 - globalX1 + 1;
  const int16_t alignedHeight = globalY2 - globalY1 + 1;
  uint16_t *framebuffer = xyCanvas_->getFramebuffer();

  if (alignedWidth == kControlW) {
    display_->draw16bitRGBBitmap(globalX1, globalY1,
                                 framebuffer + y1 * kControlW,
                                 alignedWidth, alignedHeight);
    return;
  }
  if (alignedWidth > kXyStripBufferWidth ||
      alignedHeight > kXyStripBufferHeight) {
    xyCanvas_->flush();
    return;
  }
  for (int16_t row = 0; row < alignedHeight; ++row) {
    memcpy(xyStripBuffer_ + row * alignedWidth,
           framebuffer + (y1 + row) * kControlW + x1,
           alignedWidth * sizeof(uint16_t));
  }
  display_->draw16bitRGBBitmap(globalX1, globalY1, xyStripBuffer_,
                               alignedWidth, alignedHeight);
}

void UserInterface::drawFaders(const ControlState &state) {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const int16_t faderWidth = (kControlW - 3 * kControlGap) / 4;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, backgroundColor_);
  for (uint8_t index = 0; index < 4; ++index) {
    const int16_t x = originX + index * (faderWidth + kControlGap);
    const int16_t fillHeight = roundf(state.faders[index] * kControlH);
    if (fillHeight > 0) {
      target->fillRect(x, originY + kControlH - fillHeight,
                       faderWidth, fillHeight, white);
    }
  }
  if (xyCanvas_) xyCanvas_->flush();
}

void UserInterface::drawButtons(const ControlState &state) {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, backgroundColor_);
  for (uint8_t index = 0; index < 4; ++index) {
    const int16_t x = originX + kButtonInsetX +
                      (index % 2) * (kButtonSize + kControlGap);
    const int16_t y = originY + kButtonInsetY +
                      (index / 2) * (kButtonSize + kControlGap);
    const bool pressed = state.buttons[index];
    target->fillRect(x, y, kButtonSize, kButtonSize, white);
    if (!pressed) {
      target->fillRect(x + kButtonBorder, y + kButtonBorder,
                       kButtonSize - 2 * kButtonBorder,
                       kButtonSize - 2 * kButtonBorder, backgroundColor_);
    }
  }
  if (xyCanvas_) xyCanvas_->flush();
}

void UserInterface::drawBall() {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, backgroundColor_);

  const int16_t arenaX = originX + kBallArenaX;
  const int16_t arenaY = originY + kBallArenaY;
  target->fillRect(arenaX, arenaY, kBallArenaSize, kBallArenaSize, white);
  target->fillRect(arenaX + kBallWallWidth, arenaY + kBallWallWidth,
                   kBallArenaSize - 2 * kBallWallWidth,
                   kBallArenaSize - 2 * kBallWallWidth, backgroundColor_);

  const int16_t minimumX = arenaX + kBallWallWidth + kBallRadius;
  const int16_t maximumX = arenaX + kBallArenaSize - kBallWallWidth - kBallRadius - 1;
  const int16_t minimumY = arenaY + kBallWallWidth + kBallRadius;
  const int16_t maximumY = arenaY + kBallArenaSize - kBallWallWidth - kBallRadius - 1;
  for (const BallState &ball : balls_) {
    if (!ball.active) continue;
    const int16_t ballX = minimumX + roundf(ball.x * (maximumX - minimumX));
    const int16_t ballY = minimumY + roundf(ball.y * (maximumY - minimumY));
    target->fillCircle(ballX, ballY, kBallRadius, white);
  }
  ballDirty_ = false;
  if (xyCanvas_) xyCanvas_->flush();
}

void UserInterface::drawPendulum() {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, backgroundColor_);

  for (const PendulumState &pendulum : pendulums_) {
    if (!pendulum.active) continue;
    const float bobXValue = pendulum.ax + sinf(pendulum.angle) * pendulum.length;
    const float bobYValue = pendulum.ay + cosf(pendulum.angle) * pendulum.length;
    const int16_t anchorX = originX + roundf(pendulum.ax);
    const int16_t anchorY = originY + roundf(pendulum.ay);
    const int16_t bobX = originX + roundf(bobXValue);
    const int16_t bobY = originY + roundf(bobYValue);
    const float armX = bobXValue - pendulum.ax;
    const float armY = bobYValue - pendulum.ay;
    const float armLength = max(1.0f, sqrtf(armX * armX + armY * armY));
    const float normalX = -armY / armLength;
    const float normalY = armX / armLength;
    for (int8_t offset = -kLineWidth / 2; offset <= kLineWidth / 2; ++offset) {
      target->drawLine(anchorX + roundf(normalX * offset),
                       anchorY + roundf(normalY * offset),
                       bobX + roundf(normalX * offset),
                       bobY + roundf(normalY * offset), white);
    }

    const int16_t halfPivot = kPendulumPivotSize / 2;
    target->fillRect(anchorX - halfPivot, anchorY - halfPivot,
                     kPendulumPivotSize, kPendulumPivotSize, white);
    target->fillRect(anchorX - halfPivot + kLineWidth,
                     anchorY - halfPivot + kLineWidth,
                     kPendulumPivotSize - 2 * kLineWidth,
                     kPendulumPivotSize - 2 * kLineWidth, backgroundColor_);
    target->fillCircle(bobX, bobY, kPendulumBobRadius, white);
  }

  pendulumDirty_ = false;
  if (xyCanvas_) xyCanvas_->flush();
}

void UserInterface::drawParticles() {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, white);
  target->fillRect(originX + kLineWidth, originY + kLineWidth,
                   kControlW - 2 * kLineWidth, kControlH - 2 * kLineWidth,
                   backgroundColor_);
  for (const ParticleState &particle : particles_) {
    if (!particle.active) continue;
    const int16_t size = particle.size;
    const int16_t x = originX + roundf(particle.x) - size / 2;
    const int16_t y = originY + roundf(particle.y) - size / 2;
    target->fillRect(x, y, size, size, white);
  }
  particleDirty_ = false;
  if (xyCanvas_) xyCanvas_->flush();
}

void UserInterface::drawKeyboard(const ControlState &state) {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kKeyboardHeight, white);

  // White notes occupy eight equal columns. An active white note retains a
  // five-pixel white rim so its shape remains legible against the background.
  for (uint8_t index = 0; index < 8; ++index) {
    if (!state.keyboardNotes[keyboardBaseMidiNote_ + kKeyboardWhiteNotes[index]]) continue;
    const int16_t x = originX + index * kKeyboardWhiteKeyWidth;
    const int16_t width = index == 7
        ? kControlW - index * kKeyboardWhiteKeyWidth
        : kKeyboardWhiteKeyWidth;
    target->fillRect(x + kLineWidth, originY + kLineWidth,
                     width - 2 * kLineWidth, kKeyboardHeight - 2 * kLineWidth,
                     backgroundColor_);
  }

  // Five-pixel divisions use the configurable background colour and echo the
  // strong geometry used by the rest of the interface.
  for (uint8_t boundary = 1; boundary < 8; ++boundary) {
    const int16_t x = originX + boundary * kKeyboardWhiteKeyWidth - kLineWidth / 2;
    target->fillRect(x, originY, kLineWidth, kKeyboardHeight, backgroundColor_);
  }

  // Black notes are 25-pixel blocks. When active they invert while keeping a
  // background-coloured rim, so both white and black presses remain obvious.
  for (uint8_t index = 0; index < 5; ++index) {
    const int16_t centre = originX +
        kKeyboardBlackBoundaries[index] * kKeyboardWhiteKeyWidth;
    const int16_t x = centre - kKeyboardBlackKeyWidth / 2;
    target->fillRect(x, originY, kKeyboardBlackKeyWidth,
                     kKeyboardBlackKeyHeight, backgroundColor_);
    if (state.keyboardNotes[keyboardBaseMidiNote_ + kKeyboardBlackNotes[index]]) {
      target->fillRect(x + kLineWidth, originY + kLineWidth,
                       kKeyboardBlackKeyWidth - 2 * kLineWidth,
                       kKeyboardBlackKeyHeight - 2 * kLineWidth, white);
    }
  }

  // Two large local octave controls occupy the space beneath the shortened
  // keybed. Their chevrons avoid text while remaining unambiguous.
  for (uint8_t button = 0; button < 2; ++button) {
    const bool enabled = button == 0
        ? keyboardBaseMidiNote_ > kKeyboardMinimumBaseMidiNote
        : keyboardBaseMidiNote_ < kKeyboardMaximumBaseMidiNote;
    const bool pressed = octaveButtonPressed_ == static_cast<int8_t>(button);
    const int16_t x = originX + button *
        (kKeyboardOctaveWidth + kKeyboardOctaveGap);
    const int16_t y = originY + kKeyboardOctaveY;
    const uint16_t fill = pressed ? backgroundColor_ : (enabled ? white : mutedColor_);
    const uint16_t glyph = pressed ? white : backgroundColor_;
    target->fillRect(x, y, kKeyboardOctaveWidth, kKeyboardOctaveHeight, fill);

    const int16_t centreX = x + kKeyboardOctaveWidth / 2;
    const int16_t centreY = y + kKeyboardOctaveHeight / 2;
    const int16_t direction = button == 0 ? 1 : -1;
    for (int8_t thickness = -2; thickness <= 2; ++thickness) {
      target->drawLine(centreX - 13, centreY - direction * 7 + thickness,
                       centreX, centreY + direction * 7 + thickness, glyph);
      target->drawLine(centreX, centreY + direction * 7 + thickness,
                       centreX + 13, centreY - direction * 7 + thickness, glyph);
    }
  }

  if (xyCanvas_) {
    uint8_t changedCount = 0;
    for (uint8_t key = 0; key < 13; ++key) {
      const uint8_t note = keyboardBaseMidiNote_ + key;
      if (drawnState_.keyboardNotes[note] != state.keyboardNotes[note]) ++changedCount;
    }
    if (!keyboardPartialValid_ || changedCount > 4) {
      xyCanvas_->flush();
    } else {
      for (uint8_t key = 0; key < 13; ++key) {
        const uint8_t note = keyboardBaseMidiNote_ + key;
        if (drawnState_.keyboardNotes[note] != state.keyboardNotes[note]) {
          flushKeyboardKey(key);
        }
      }
    }
    keyboardPartialValid_ = true;
  } else {
    keyboardPartialValid_ = false;
  }
}

void UserInterface::flushKeyboardKey(uint8_t key) {
  for (uint8_t index = 0; index < 8; ++index) {
    if (kKeyboardWhiteNotes[index] != key) continue;
    const int16_t width = index == 7
        ? kControlW - index * kKeyboardWhiteKeyWidth
        : kKeyboardWhiteKeyWidth;
    flushXyRegion(index * kKeyboardWhiteKeyWidth, kControlCanvasInsetY,
                  width, kKeyboardHeight);
    return;
  }
  for (uint8_t index = 0; index < 5; ++index) {
    if (kKeyboardBlackNotes[index] != key) continue;
    const int16_t centre = kKeyboardBlackBoundaries[index] * kKeyboardWhiteKeyWidth;
    flushXyRegion(centre - kKeyboardBlackKeyWidth / 2,
                  kControlCanvasInsetY, kKeyboardBlackKeyWidth,
                  kKeyboardBlackKeyHeight);
    return;
  }
}

void UserInterface::drawSpectrum(const ControlState &state) {
  Arduino_GFX *target = xyCanvas_ ? static_cast<Arduino_GFX *>(xyCanvas_)
                                  : static_cast<Arduino_GFX *>(display_);
  const int16_t originX = xyCanvas_ ? 0 : kControlX;
  const int16_t originY = xyCanvas_ ? kControlCanvasInsetY : kControlY;
  const uint16_t white = rgb565(255, 255, 255);
  target->fillRect(originX, originY, kControlW, kControlH, backgroundColor_);

  for (uint8_t index = 0; index < kSpectrumBandCount; ++index) {
    const int16_t cellLeft = index * kControlW / kSpectrumBandCount;
    const int16_t cellRight = (index + 1) * kControlW / kSpectrumBandCount;
    const int16_t barWidth = max<int16_t>(1, cellRight - cellLeft - 2);
    const int16_t barHeight = roundf(constrain(state.spectrum[index], 0.0f, 1.0f) *
                                    kSpectrumHeight);
    if (barHeight > 0) {
      target->fillRect(originX + cellLeft + 1,
                       originY + kSpectrumHeight - barHeight,
                       barWidth, barHeight, white);
    }
  }

  const int16_t captureY = originY + kSpectrumCaptureY;
  target->fillRect(originX, captureY, kControlW, kSpectrumCaptureHeight, white);
  if (!fftCaptureActive_) {
    target->fillRect(originX + kSpectrumCaptureBorder,
                     captureY + kSpectrumCaptureBorder,
                     kControlW - 2 * kSpectrumCaptureBorder,
                     kSpectrumCaptureHeight - 2 * kSpectrumCaptureBorder,
                     backgroundColor_);
  }
  if (xyCanvas_) {
    if (spectrumPartialValid_) {
      flushXyRegion(0, kControlCanvasInsetY, kControlW, kSpectrumHeight);
    } else {
      xyCanvas_->flush();
    }
    spectrumPartialValid_ = true;
  }
}

void UserInterface::drawPageButton() {
  const uint16_t white = rgb565(255, 255, 255);
  display_->fillRect(kPageX, kPageY, kPageW, kPageH,
                     pageButtonPressed_ ? backgroundColor_ : white);
  const uint16_t indicatorColor = pageButtonPressed_ ? white : backgroundColor_;
  const int16_t centerY = kPageY + kPageH / 2;
  for (uint8_t page = 0; page < kControlPageCount; ++page) {
    const int16_t size = page == controlPage_ ? 25 : 9;
    const int16_t centerX = kPageX + kPageW / 2 +
        (2 * static_cast<int16_t>(page) - (kControlPageCount - 1)) * 14;
    display_->fillRect(centerX - size / 2, centerY - size / 2, size, size,
                       indicatorColor);
  }
}
