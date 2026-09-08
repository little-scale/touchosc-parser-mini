// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

enum class TouchOscControlType : uint8_t {
  Button = 1,
  Fader = 2,
  Radial = 3,
  Xy = 4,
  Radar = 5,
  Encoder = 6,
  Grid = 7,
  Label = 8,
  Text = 9,
};

enum TouchOscControlFlag : uint32_t {
  TocVisible = 1u << 0,
  TocInteractive = 1u << 1,
  TocBackground = 1u << 2,
  TocOutline = 1u << 3,
  TocBracketOutline = 1u << 4,
  TocCircle = 1u << 5,
  TocCursor = 1u << 6,
  TocGridX = 1u << 7,
  TocGridY = 1u << 8,
  TocBar = 1u << 9,
  TocLines = 1u << 10,
  TocCentered = 1u << 11,
  TocInverted = 1u << 12,
  TocLockX = 1u << 13,
  TocLockY = 1u << 14,
  TocSend = 1u << 15,
  TocReceive = 1u << 16,
  TocRelative = 1u << 17,
  TocExclusive = 1u << 18,
  TocTextWrap = 1u << 19,
  TocTextClip = 1u << 20,
  TocMonospaced = 1u << 21,
};

struct TouchOscEvent {
  char address[96] = {};
  float values[2] = {};
  uint8_t valueCount = 0;
};

class TouchOscLayout {
 public:
  static constexpr const char *kActivePath = "/touchosc-layout.bin";
  static constexpr const char *kUploadPath = "/touchosc-upload.tmp";

  bool begin();
  bool loadActive();
  bool installUploaded();
  bool removeActive();
  bool active() const { return active_; }
  const String &lastError() const { return lastError_; }
  uint8_t controlCount() const { return controlCount_; }
  uint8_t pageCount() const { return pageCount_; }
  uint8_t activePage() const { return activePage_; }
  void nextPage();

  void draw(Arduino_GFX *target, int16_t screenWidth, int16_t screenHeight,
            bool allowPartial, int16_t &updateX, int16_t &updateY,
            int16_t &updateWidth, int16_t &updateHeight);
  bool dirty() const { return dirty_; }
  void forceRedraw();

  void beginTouch(int16_t x, int16_t y, int16_t screenWidth, int16_t screenHeight);
  void moveTouch(int16_t x, int16_t y, int16_t screenWidth, int16_t screenHeight);
  void endTouch();
  bool popEvent(TouchOscEvent &event);
  bool applyRemote(const String &address, const float *values, uint8_t valueCount);
  bool applyRemoteText(const String &address, const String &text);
  bool controlActive(const String &address) const;

 private:
  static constexpr uint8_t kMaxControls = 64;
  static constexpr uint8_t kMaxPages = 8;
  static constexpr uint8_t kEventQueueSize = 16;
  struct Rgba {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
  };
  struct Control {
    TouchOscControlType type = TouchOscControlType::Button;
    uint8_t page = 0;
    uint32_t flags = 0;
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;
    Rgba color;
    Rgba gridColor{0, 0, 0, 64};
    Rgba textColor;
    uint8_t gridStepsX = 0;
    uint8_t gridStepsY = 0;
    uint8_t valueCount = 1;
    uint8_t parentGrid = 0xff;
    float values[2] = {};
    char address[96] = {};
    char text[128] = {};
    uint16_t textSize = 16;
    uint8_t textAlignH = 2;
    uint8_t textAlignV = 2;
  };
  struct Page {
    uint16_t width = 368;
    uint16_t height = 448;
    Rgba background{0, 0, 0, 255};
  };

  bool loadFile(const char *path);
  int8_t hitTest(float x, float y) const;
  void updateControl(uint8_t index, float x, float y, bool forceSend);
  void markControlDirty(uint8_t index);
  void enqueue(const Control &control);
  void calculateTransform(int16_t screenWidth, int16_t screenHeight,
                          float &scale, float &offsetX, float &offsetY) const;
  uint16_t composite(const Rgba &color, float opacity = 1.0f) const;
  uint16_t blend(uint16_t under, const Rgba &over, float opacity = 1.0f) const;
  void drawControl(Arduino_GFX *target, const Control &control,
                   float scale, float offsetX, float offsetY) const;
  void drawTextControl(Arduino_GFX *target, const Control &control,
                       int16_t x, int16_t y, int16_t width, int16_t height,
                       float scale) const;
  void drawOutline(Arduino_GFX *target, int16_t x, int16_t y,
                   int16_t width, int16_t height, int16_t radius,
                   uint16_t color, bool brackets) const;
  void drawRing(Arduino_GFX *target, int16_t centerX, int16_t centerY,
                int16_t outerRadius, int16_t innerRadius,
                float startAngle, float endAngle, uint16_t color) const;

  Control controls_[kMaxControls];
  Page pages_[kMaxPages];
  TouchOscEvent events_[kEventQueueSize];
  Rgba background_{0, 0, 0, 255};
  uint16_t canvasWidth_ = 368;
  uint16_t canvasHeight_ = 448;
  uint8_t controlCount_ = 0;
  uint8_t pageCount_ = 1;
  uint8_t activePage_ = 0;
  uint8_t eventRead_ = 0;
  uint8_t eventWrite_ = 0;
  int8_t activeControl_ = -1;
  float previousAngle_ = 0.0f;
  bool active_ = false;
  bool dirty_ = false;
  bool fullRedraw_ = true;
  int8_t dirtyControl_ = -1;
  uint32_t lastSendMs_ = 0;
  String lastError_;
};
