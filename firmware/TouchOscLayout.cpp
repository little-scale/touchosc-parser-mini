// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include "TouchOscLayout.h"

#include <FFat.h>
#include <FS.h>
#include <memory>

namespace {
constexpr uint8_t kFormatVersion = 3;
constexpr uint32_t kMaximumFileSize = 32 * 1024;
constexpr int16_t kStatusSafeHeight = 48;
constexpr float kPi = 3.14159265358979323846f;
constexpr uint32_t kSendIntervalMs = 20;

bool readExact(File &file, void *target, size_t length) {
  return file.read(static_cast<uint8_t *>(target), length) == length;
}

bool readU8(File &file, uint8_t &value) { return readExact(file, &value, 1); }

bool readU16(File &file, uint16_t &value) {
  uint8_t bytes[2];
  if (!readExact(file, bytes, sizeof(bytes))) return false;
  value = static_cast<uint16_t>(bytes[0]) |
          (static_cast<uint16_t>(bytes[1]) << 8);
  return true;
}

bool readI16(File &file, int16_t &value) {
  uint16_t raw;
  if (!readU16(file, raw)) return false;
  value = static_cast<int16_t>(raw);
  return true;
}

bool readU32(File &file, uint32_t &value) {
  uint8_t bytes[4];
  if (!readExact(file, bytes, sizeof(bytes))) return false;
  value = static_cast<uint32_t>(bytes[0]) |
          (static_cast<uint32_t>(bytes[1]) << 8) |
          (static_cast<uint32_t>(bytes[2]) << 16) |
          (static_cast<uint32_t>(bytes[3]) << 24);
  return true;
}

bool readFloat(File &file, float &value) {
  uint32_t bits;
  if (!readU32(file, bits)) return false;
  memcpy(&value, &bits, sizeof(value));
  return isfinite(value);
}

float normalizedAngle(float angle) {
  while (angle < 0.0f) angle += 2.0f * kPi;
  while (angle >= 2.0f * kPi) angle -= 2.0f * kPi;
  return angle;
}

uint8_t controlOrientation(uint32_t flags) {
  return static_cast<uint8_t>((flags & TocOrientationMask) >> 22);
}

bool horizontalOrientation(uint8_t orientation) {
  return orientation == 1 || orientation == 3;
}

float orientationAngle(uint8_t orientation) {
  return orientation * kPi * 0.5f;
}
}  // namespace

bool TouchOscLayout::begin() {
  if (!FFat.begin(true)) {
    lastError_ = "Could not mount layout storage";
    return false;
  }
  return loadActive();
}

bool TouchOscLayout::loadActive() {
  if (!FFat.exists(kActivePath)) {
    active_ = false;
    lastError_ = "No uploaded layout";
    return false;
  }
  return loadFile(kActivePath);
}

bool TouchOscLayout::installUploaded() {
  if (!FFat.exists(kUploadPath)) {
    lastError_ = "Upload was not received";
    return false;
  }
  if (!loadFile(kUploadPath)) {
    FFat.remove(kUploadPath);
    return false;
  }
  constexpr const char *backupPath = "/touchosc-layout.backup";
  FFat.remove(backupPath);
  const bool hadActive = FFat.exists(kActivePath);
  if (hadActive && !FFat.rename(kActivePath, backupPath)) {
    FFat.remove(kUploadPath);
    lastError_ = "Could not preserve the previous layout";
    return false;
  }
  if (!FFat.rename(kUploadPath, kActivePath)) {
    if (hadActive) FFat.rename(backupPath, kActivePath);
    loadActive();
    lastError_ = "Could not activate uploaded layout";
    return false;
  }
  FFat.remove(backupPath);
  forceRedraw();
  return true;
}

bool TouchOscLayout::installEmbedded(const uint8_t *data, size_t size) {
  if (!data || size < 16 || size > kMaximumFileSize) {
    lastError_ = "Embedded layout size is invalid";
    return false;
  }
  FFat.remove(kUploadPath);
  File file = FFat.open(kUploadPath, FILE_WRITE);
  if (!file) {
    lastError_ = "Could not create embedded layout";
    return false;
  }
  uint8_t buffer[512];
  size_t offset = 0;
  bool writeOk = true;
  while (offset < size) {
    const size_t amount = min(sizeof(buffer), size - offset);
    memcpy_P(buffer, data + offset, amount);
    if (file.write(buffer, amount) != amount) {
      writeOk = false;
      break;
    }
    offset += amount;
  }
  file.close();
  if (!writeOk) {
    FFat.remove(kUploadPath);
    lastError_ = "Could not write embedded layout";
    return false;
  }
  return installUploaded();
}

bool TouchOscLayout::removeActive() {
  endTouch();
  FFat.remove(kUploadPath);
  FFat.remove("/touchosc-layout.backup");
  if (FFat.exists(kActivePath) && !FFat.remove(kActivePath)) {
    lastError_ = "Could not remove the installed layout";
    return false;
  }

  controlCount_ = 0;
  pageCount_ = 1;
  activePage_ = 0;
  activeControl_ = -1;
  eventRead_ = eventWrite_ = 0;
  active_ = false;
  dirty_ = false;
  fullRedraw_ = true;
  dirtyControl_ = -1;
  lastError_ = "";
  Serial.println("touchosc layout removed");
  return true;
}

void TouchOscLayout::forceRedraw() {
  dirty_ = true;
  fullRedraw_ = true;
  dirtyControl_ = -1;
}

void TouchOscLayout::nextPage() {
  if (!active_ || pageCount_ < 2) return;
  endTouch();
  activePage_ = (activePage_ + 1) % pageCount_;
  canvasWidth_ = pages_[activePage_].width;
  canvasHeight_ = pages_[activePage_].height;
  background_ = pages_[activePage_].background;
  forceRedraw();
}

bool TouchOscLayout::loadFile(const char *path) {
  File file = FFat.open(path, FILE_READ);
  if (!file) {
    lastError_ = "Could not open layout";
    return false;
  }
  if (file.size() < 16 || file.size() > kMaximumFileSize) {
    lastError_ = "Layout file size is invalid";
    return false;
  }
  char magic[4];
  uint8_t version = 0;
  uint8_t count = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  Rgba background;
  uint16_t reserved = 0;
  if (!readExact(file, magic, sizeof(magic)) || memcmp(magic, "TLAY", 4) != 0 ||
      !readU8(file, version) || version < 1 || version > kFormatVersion ||
      !readU8(file, count) || count == 0 || count > kMaxControls ||
      !readU16(file, width) || !readU16(file, height) ||
      !readExact(file, &background, sizeof(background)) ||
      !readU16(file, reserved) || width == 0 || height == 0) {
    lastError_ = "Layout header is invalid";
    return false;
  }

  const uint8_t parsedPageCount = version >= 3
      ? static_cast<uint8_t>(reserved & 0xff)
      : 1;
  if (parsedPageCount == 0 || parsedPageCount > kMaxPages) {
    lastError_ = "Layout page count is invalid";
    return false;
  }
  Page parsedPages[kMaxPages];
  parsedPages[0].width = width;
  parsedPages[0].height = height;
  parsedPages[0].background = background;
  for (uint8_t page = 1; page < parsedPageCount; ++page) {
    if (!readU16(file, parsedPages[page].width) ||
        !readU16(file, parsedPages[page].height) ||
        !readExact(file, &parsedPages[page].background,
                   sizeof(parsedPages[page].background)) ||
        parsedPages[page].width == 0 || parsedPages[page].height == 0) {
      lastError_ = "A layout page header is invalid";
      return false;
    }
  }

  std::unique_ptr<Control[]> parsed(new (std::nothrow) Control[kMaxControls]);
  if (!parsed) {
    lastError_ = "Not enough memory to validate layout";
    return false;
  }
  for (uint8_t index = 0; index < count; ++index) {
    Control &control = parsed[index];
    uint8_t type = 0;
    uint8_t page = 0;
    uint8_t addressLength = 0;
    if (!readU8(file, type) || type < static_cast<uint8_t>(TouchOscControlType::Button) ||
        type > static_cast<uint8_t>(version == 1 ? TouchOscControlType::Grid
                                                 : TouchOscControlType::Text) ||
        (version >= 3 && !readU8(file, page)) || page >= parsedPageCount ||
        !readU32(file, control.flags) ||
        !readI16(file, control.x) || !readI16(file, control.y) ||
        !readI16(file, control.width) || !readI16(file, control.height) ||
        !readExact(file, &control.color, sizeof(control.color)) ||
        !readExact(file, &control.gridColor, sizeof(control.gridColor)) ||
        !readU8(file, control.gridStepsX) || !readU8(file, control.gridStepsY) ||
        !readU8(file, control.valueCount) || !readU8(file, control.parentGrid) ||
        !readFloat(file, control.values[0]) || !readFloat(file, control.values[1]) ||
        !readU8(file, addressLength) || addressLength >= sizeof(control.address) ||
        !readExact(file, control.address, addressLength)) {
      lastError_ = "A control record is invalid";
      return false;
    }
    control.type = static_cast<TouchOscControlType>(type);
    control.page = page;
    control.address[addressLength] = '\0';
    if (version >= 2) {
      uint8_t textLength = 0;
      if (!readExact(file, &control.textColor, sizeof(control.textColor)) ||
          !readU16(file, control.textSize) ||
          !readU8(file, control.textAlignH) ||
          !readU8(file, control.textAlignV) ||
          !readU8(file, textLength) || textLength >= sizeof(control.text) ||
          !readExact(file, control.text, textLength)) {
        lastError_ = "A text control record is invalid";
        return false;
      }
      control.text[textLength] = '\0';
    }
    control.values[0] = constrain(control.values[0], 0.0f, 1.0f);
    control.values[1] = constrain(control.values[1], 0.0f, 1.0f);
    const bool textControl = control.type == TouchOscControlType::Label ||
                             control.type == TouchOscControlType::Text;
    const bool valueShapeValid = control.valueCount <= 2 ||
                                 (textControl && control.valueCount == 3);
    if (control.width <= 0 || control.height <= 0 || !valueShapeValid ||
        (control.valueCount > 0 && control.address[0] != '/') ||
        (textControl && (control.textSize == 0 || control.textAlignH < 1 ||
                         control.textAlignH > 3 || control.textAlignV < 1 ||
                         control.textAlignV > 3))) {
      lastError_ = "A control has invalid geometry or OSC data";
      return false;
    }
  }
  if (file.position() != file.size()) {
    lastError_ = "Layout contains trailing data";
    return false;
  }

  memcpy(controls_, parsed.get(), sizeof(Control) * count);
  memcpy(pages_, parsedPages, sizeof(Page) * parsedPageCount);
  pageCount_ = parsedPageCount;
  activePage_ = 0;
  background_ = pages_[0].background;
  canvasWidth_ = pages_[0].width;
  canvasHeight_ = pages_[0].height;
  controlCount_ = count;
  activeControl_ = -1;
  eventRead_ = eventWrite_ = 0;
  active_ = true;
  forceRedraw();
  lastError_ = "";
  Serial.printf("touchosc layout pages=%u controls=%u canvas=%ux%u\n",
                pageCount_, count, width, height);
  return true;
}

void TouchOscLayout::calculateTransform(int16_t screenWidth, int16_t screenHeight,
                                        float &scale, float &offsetX, float &offsetY) const {
  const bool safeAreaLayout = canvasHeight_ <= screenHeight - kStatusSafeHeight;
  const int16_t availableHeight = safeAreaLayout
      ? screenHeight - kStatusSafeHeight
      : screenHeight;
  scale = min(static_cast<float>(screenWidth) / canvasWidth_,
              static_cast<float>(availableHeight) / canvasHeight_);
  offsetX = (screenWidth - canvasWidth_ * scale) * 0.5f;
  offsetY = safeAreaLayout
      ? screenHeight - canvasHeight_ * scale
      : (screenHeight - canvasHeight_ * scale) * 0.5f;
}

uint16_t TouchOscLayout::blend(uint16_t under, const Rgba &over, float opacity) const {
  const float alpha = constrain((over.a / 255.0f) * opacity, 0.0f, 1.0f);
  const uint8_t ur = ((under >> 11) & 0x1f) * 255 / 31;
  const uint8_t ug = ((under >> 5) & 0x3f) * 255 / 63;
  const uint8_t ub = (under & 0x1f) * 255 / 31;
  const uint8_t r = roundf(ur + (over.r - ur) * alpha);
  const uint8_t g = roundf(ug + (over.g - ug) * alpha);
  const uint8_t b = roundf(ub + (over.b - ub) * alpha);
  return static_cast<uint16_t>(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

uint16_t TouchOscLayout::composite(const Rgba &color, float opacity) const {
  const uint16_t base = static_cast<uint16_t>(((background_.r & 0xf8) << 8) |
                                               ((background_.g & 0xfc) << 3) |
                                               (background_.b >> 3));
  return blend(base, color, opacity);
}

void TouchOscLayout::drawOutline(Arduino_GFX *target, int16_t x, int16_t y,
                                 int16_t width, int16_t height, int16_t radius,
                                 uint16_t color, bool brackets) const {
  if (!brackets) {
    if (radius > 0) target->drawRoundRect(x, y, width, height, radius, color);
    else target->drawRect(x, y, width, height, color);
    return;
  }
  const int16_t mark = min<int16_t>(8, min(width, height) / 4);
  target->drawFastHLine(x, y, mark, color);
  target->drawFastVLine(x, y, mark, color);
  target->drawFastHLine(x + width - mark, y, mark, color);
  target->drawFastVLine(x + width - 1, y, mark, color);
  target->drawFastHLine(x, y + height - 1, mark, color);
  target->drawFastVLine(x, y + height - mark, mark, color);
  target->drawFastHLine(x + width - mark, y + height - 1, mark, color);
  target->drawFastVLine(x + width - 1, y + height - mark, mark, color);
}

void TouchOscLayout::drawRing(Arduino_GFX *target, int16_t cx, int16_t cy,
                              int16_t outer, int16_t inner, float start,
                              float end, uint16_t color) const {
  const uint16_t segments = max<uint16_t>(2, ceilf(fabsf(end - start) * outer / 5.0f));
  float cosine0 = cosf(start);
  float sine0 = sinf(start);
  for (uint16_t i = 0; i < segments; ++i) {
    const float a1 = start + (end - start) * (i + 1) / segments;
    const float cosine1 = cosf(a1);
    const float sine1 = sinf(a1);
    const int16_t ox0 = cx + roundf(cosine0 * outer);
    const int16_t oy0 = cy + roundf(sine0 * outer);
    const int16_t ox1 = cx + roundf(cosine1 * outer);
    const int16_t oy1 = cy + roundf(sine1 * outer);
    const int16_t ix0 = cx + roundf(cosine0 * inner);
    const int16_t iy0 = cy + roundf(sine0 * inner);
    const int16_t ix1 = cx + roundf(cosine1 * inner);
    const int16_t iy1 = cy + roundf(sine1 * inner);
    target->fillTriangle(ox0, oy0, ox1, oy1, ix0, iy0, color);
    target->fillTriangle(ox1, oy1, ix0, iy0, ix1, iy1, color);
    cosine0 = cosine1;
    sine0 = sine1;
  }
}

void TouchOscLayout::drawControl(Arduino_GFX *target, const Control &c,
                                 float scale, float offsetX, float offsetY) const {
  if (!(c.flags & TocVisible)) return;
  const int16_t x = roundf(offsetX + c.x * scale);
  const int16_t y = roundf(offsetY + c.y * scale);
  const int16_t w = max<int16_t>(2, roundf(c.width * scale));
  const int16_t h = max<int16_t>(2, roundf(c.height * scale));
  const int16_t radius = max<int16_t>(1, min(w, h) / 40);
  const uint16_t dim = composite(c.color, 0.27f);
  const uint16_t medium = composite(c.color, 0.58f);
  const uint16_t bright = composite(c.color);
  const uint16_t grid = blend(dim, c.gridColor);
  const bool brackets = c.flags & TocBracketOutline;

  if (c.type == TouchOscControlType::Label ||
      c.type == TouchOscControlType::Text) {
    if (c.flags & TocBackground) {
      target->fillRoundRect(x, y, w, h, radius, composite(c.color));
    }
    if (c.flags & TocOutline) drawOutline(target, x, y, w, h, radius, bright, brackets);
    drawTextControl(target, c, x, y, w, h, scale);
    return;
  }

  if (c.type == TouchOscControlType::Grid) {
    if (c.flags & TocOutline) drawOutline(target, x, y, w, h, radius, bright, brackets);
    return;
  }
  if (c.type == TouchOscControlType::Button) {
    if (c.flags & TocBackground) target->fillRoundRect(x, y, w, h, radius,
                                                       c.values[0] > 0.5f ? bright : dim);
    if (c.flags & TocOutline) drawOutline(target, x, y, w, h, radius, bright, brackets);
    return;
  }
  if (c.type == TouchOscControlType::Fader) {
    if (c.flags & TocBackground) target->fillRoundRect(x, y, w, h, radius, dim);
    const uint8_t orientation = controlOrientation(c.flags);
    const bool horizontal = horizontalOrientation(orientation);
    const uint8_t gridSteps = max(c.gridStepsX, c.gridStepsY);
    float value = (c.flags & TocInverted) ? 1.0f - c.values[0] : c.values[0];
    if (horizontal) {
      const int16_t fill = roundf(value * w);
      const bool west = orientation == 3;
      if ((c.flags & TocBar) && fill > 0) {
        target->fillRect(west ? x + w - fill : x, y, fill, h, medium);
      }
      if (c.flags & TocCursor) {
        const int16_t cursorX = constrain(
            static_cast<int16_t>(west ? x + w - roundf(value * w)
                                      : x + roundf(value * w)),
            static_cast<int16_t>(x + 2), static_cast<int16_t>(x + w - 3));
        const int16_t cursorWidth = max<int16_t>(5, min<int16_t>(24, w / 7));
        target->fillRoundRect(cursorX - cursorWidth / 2, y, cursorWidth, h,
                              radius, bright);
      }
    } else {
      const int16_t fill = roundf(value * h);
      const bool south = orientation == 2;
      if ((c.flags & TocBar) && fill > 0) {
        target->fillRect(x, south ? y : y + h - fill, w, fill, medium);
      }
      if (c.flags & TocCursor) {
        const int16_t cursorY = constrain(
            static_cast<int16_t>(south ? y + roundf(value * h)
                                       : y + h - roundf(value * h)),
            static_cast<int16_t>(y + 2), static_cast<int16_t>(y + h - 3));
        const int16_t cursorHeight = max<int16_t>(5, min<int16_t>(24, h / 7));
        target->fillRoundRect(x, cursorY - cursorHeight / 2, w, cursorHeight,
                              radius, bright);
      }
    }
    // TouchOSC keeps the scale visible across the inactive background, bar
    // fill, and cursor. Draw it last so every fader style behaves consistently
    // with the rotary controls.
    if ((c.flags & (TocGridX | TocGridY)) && gridSteps > 1) {
      for (uint8_t i = 1; i < gridSteps; ++i) {
        if (horizontal) {
          target->drawFastVLine(x + i * w / gridSteps, y, h, grid);
        } else {
          target->drawFastHLine(x, y + i * h / gridSteps, w, grid);
        }
      }
    }
    if (c.flags & TocOutline) drawOutline(target, x, y, w, h, radius, bright, brackets);
    return;
  }
  if (c.type == TouchOscControlType::Xy) {
    if (c.flags & TocBackground) target->fillRoundRect(x, y, w, h, radius, dim);
    const uint8_t orientation = controlOrientation(c.flags);
    const bool horizontal = horizontalOrientation(orientation);
    if ((c.flags & TocGridX) && c.gridStepsX > 1) {
      for (uint8_t i = 1; i < c.gridStepsX; ++i) {
        if (horizontal) target->drawFastHLine(x, y + i * h / c.gridStepsX, w, grid);
        else target->drawFastVLine(x + i * w / c.gridStepsX, y, h, grid);
      }
    }
    if ((c.flags & TocGridY) && c.gridStepsY > 1) {
      for (uint8_t i = 1; i < c.gridStepsY; ++i) {
        if (horizontal) target->drawFastVLine(x + i * w / c.gridStepsY, y, h, grid);
        else target->drawFastHLine(x, y + i * h / c.gridStepsY, w, grid);
      }
    }
    float displayX = c.values[0];
    float displayY = 1.0f - c.values[1];
    if (orientation == 1) {
      displayX = c.values[1];
      displayY = c.values[0];
    } else if (orientation == 2) {
      displayX = 1.0f - c.values[0];
      displayY = c.values[1];
    } else if (orientation == 3) {
      displayX = 1.0f - c.values[1];
      displayY = 1.0f - c.values[0];
    }
    const int16_t px = x + roundf(displayX * (w - 1));
    const int16_t py = y + roundf(displayY * (h - 1));
    if (c.flags & TocLines) {
      target->drawFastVLine(px, y, h, bright);
      target->drawFastHLine(x, py, w, bright);
    }
    if (c.flags & TocCursor) {
      const int16_t size = max<int16_t>(7, min<int16_t>(24, min(w, h) / 6));
      target->fillRoundRect(px - size / 2, py - size / 2, size, size, radius, bright);
    }
    if (c.flags & TocOutline) drawOutline(target, x, y, w, h, radius, bright, brackets);
    return;
  }

  const int16_t cx = x + w / 2;
  const int16_t cy = y + h / 2;
  const int16_t outer = max<int16_t>(3, min(w, h) / 2 - 1);
  if (c.type == TouchOscControlType::Radar) {
    if (c.flags & TocBackground) target->fillCircle(cx, cy, outer, dim);
    if ((c.flags & TocGridX) && c.gridStepsX > 1) {
      for (uint8_t i = 1; i < c.gridStepsX; ++i) target->drawCircle(cx, cy, i * outer / c.gridStepsX, grid);
    }
    if ((c.flags & TocGridY) && c.gridStepsY > 1) {
      for (uint8_t i = 0; i < c.gridStepsY; ++i) {
        const float angle = i * 2.0f * kPi / c.gridStepsY;
        target->drawLine(cx, cy, cx + roundf(cosf(angle) * outer),
                         cy + roundf(sinf(angle) * outer), grid);
      }
    }
    const float angle = c.values[1] * 2.0f * kPi - kPi / 2.0f +
                        orientationAngle(controlOrientation(c.flags));
    const int16_t amplitudeRadius = roundf(c.values[0] * outer);
    const int16_t px = cx + roundf(cosf(angle) * c.values[0] * outer);
    const int16_t py = cy + roundf(sinf(angle) * c.values[0] * outer);
    if (amplitudeRadius > 1) target->drawCircle(cx, cy, amplitudeRadius, bright);
    if (c.flags & TocLines) target->drawLine(cx, cy, px, py, bright);
    if (c.flags & TocCursor) target->fillCircle(px, py, max<int16_t>(4, outer / 20), bright);
    if (c.flags & TocOutline) target->drawCircle(cx, cy, outer, bright);
    return;
  }

  const int16_t inner = max<int16_t>(2, roundf(outer * 0.50f));
  if (c.type == TouchOscControlType::Encoder) {
    drawRing(target, cx, cy, outer, inner, 0.0f, 2.0f * kPi, dim);
    const float cursorAngle = c.values[0] * 2.0f * kPi - kPi / 2.0f +
                              orientationAngle(controlOrientation(c.flags));
    const float cursorSpan = max(0.12f, 14.0f / outer);
    if (c.flags & TocCursor) drawRing(target, cx, cy, outer, inner,
                                      cursorAngle - cursorSpan, cursorAngle + cursorSpan, bright);
    if (c.flags & TocGridX) {
      const uint8_t steps = max<uint8_t>(2, c.gridStepsX);
      for (uint8_t i = 0; i < steps; ++i) {
        const float a = i * 2.0f * kPi / steps;
        target->drawLine(cx + roundf(cosf(a) * (inner + 2)),
                         cy + roundf(sinf(a) * (inner + 2)),
                         cx + roundf(cosf(a) * (outer - 2)),
                         cy + roundf(sinf(a) * (outer - 2)), grid);
      }
    }
    if (c.flags & TocOutline) target->drawCircle(cx, cy, outer, bright);
    return;
  }

  // TouchOSC radial controls leave a sixty-degree gap centred at the bottom.
  const float start = 2.0f * kPi / 3.0f +
                      orientationAngle(controlOrientation(c.flags));
  const float span = 5.0f * kPi / 3.0f;
  drawRing(target, cx, cy, outer, inner, start, start + span, dim);
  float value = (c.flags & TocInverted) ? 1.0f - c.values[0] : c.values[0];
  if (c.flags & TocCentered) {
    const float middle = start + span * 0.5f;
    drawRing(target, cx, cy, outer, inner, middle,
             middle + (value - 0.5f) * span, bright);
  } else {
    drawRing(target, cx, cy, outer, inner, start, start + value * span, bright);
  }
  if (c.flags & TocGridX) {
    const uint8_t steps = max<uint8_t>(2, c.gridStepsX);
    for (uint8_t i = 0; i <= steps; ++i) {
      const float a = start + span * i / steps;
      target->drawLine(cx + roundf(cosf(a) * (inner + 2)),
                       cy + roundf(sinf(a) * (inner + 2)),
                       cx + roundf(cosf(a) * (outer - 2)),
                       cy + roundf(sinf(a) * (outer - 2)), grid);
    }
  }
  if (c.flags & TocOutline) {
    drawRing(target, cx, cy, outer, max<int16_t>(1, outer - 1),
             start, start + span, bright);
  }
}

void TouchOscLayout::drawTextControl(Arduino_GFX *target, const Control &c,
                                     int16_t x, int16_t y, int16_t width,
                                     int16_t height, float scale) const {
  if (!c.text[0]) return;

  // The panel rasterizer supplies one compact bitmap font. Independent X/Y
  // scaling keeps TouchOSC's requested cap height while retaining a natural
  // text width at the small display resolution.
  const uint16_t requested = max<uint16_t>(8, roundf(c.textSize * scale));
  const uint8_t textScaleX = static_cast<uint8_t>(constrain(roundf(requested / 10.0f), 1.0f, 8.0f));
  const uint8_t textScaleY = static_cast<uint8_t>(constrain(roundf(requested / 8.0f), 1.0f, 8.0f));
  const int16_t characterWidth = 6 * textScaleX;
  const int16_t lineHeight = 8 * textScaleY;
  const uint8_t maximumColumns = max<int16_t>(1, width / characterWidth);
  const uint8_t maximumVisibleLines = max<int16_t>(1, height / lineHeight);
  constexpr uint8_t kMaximumLines = 16;
  String lines[kMaximumLines];
  uint8_t lineCount = 1;
  const bool multiline = c.type == TouchOscControlType::Text;
  const bool wrap = multiline && (c.flags & TocTextWrap);

  for (const char *cursor = c.text; *cursor && lineCount <= kMaximumLines; ++cursor) {
    if (*cursor == '\r') continue;
    if (multiline && *cursor == '\n') {
      if (lineCount < kMaximumLines) ++lineCount;
      continue;
    }
    if (!multiline && *cursor == '\n') continue;
    String &line = lines[lineCount - 1];
    if ((wrap || (c.flags & TocTextClip)) && line.length() >= maximumColumns) {
      if (wrap && lineCount < kMaximumLines) {
        ++lineCount;
      } else {
        continue;
      }
    }
    lines[lineCount - 1] += *cursor;
  }

  if (c.flags & TocTextClip) lineCount = min(lineCount, maximumVisibleLines);
  const int16_t blockHeight = lineCount * lineHeight;
  int16_t textY = y;
  if (c.textAlignV == 2) textY = y + (height - blockHeight) / 2;
  if (c.textAlignV == 3) textY = y + height - blockHeight;

  target->setFont(nullptr);
  target->setTextSize(textScaleX, textScaleY);
  target->setTextWrap(false);
  target->setTextColor(composite(c.textColor));
  for (uint8_t lineIndex = 0; lineIndex < lineCount; ++lineIndex) {
    const int16_t lineWidth = lines[lineIndex].length() * characterWidth;
    int16_t textX = x;
    if (c.textAlignH == 2) textX = x + (width - lineWidth) / 2;
    if (c.textAlignH == 3) textX = x + width - lineWidth;
    target->setCursor(textX, textY + lineIndex * lineHeight);
    target->print(lines[lineIndex]);
  }
  target->setTextSize(1);
  target->setTextWrap(true);
}

void TouchOscLayout::draw(Arduino_GFX *target, int16_t screenWidth,
                          int16_t screenHeight, bool allowPartial,
                          int16_t &updateX, int16_t &updateY,
                          int16_t &updateWidth, int16_t &updateHeight) {
  updateX = updateY = updateWidth = updateHeight = 0;
  if (!active_ || !target || !dirty_) return;
  const uint16_t background = static_cast<uint16_t>(((background_.r & 0xf8) << 8) |
                                                     ((background_.g & 0xfc) << 3) |
                                                     (background_.b >> 3));
  float scale, offsetX, offsetY;
  calculateTransform(screenWidth, screenHeight, scale, offsetX, offsetY);

  bool partial = allowPartial && !fullRedraw_ && dirtyControl_ >= 0 &&
                 dirtyControl_ < controlCount_;
  if (partial) {
    const Control &changed = controls_[dirtyControl_];
    if ((changed.type == TouchOscControlType::Label ||
         changed.type == TouchOscControlType::Text) &&
        !(changed.flags & TocTextClip)) {
      partial = false;
    } else {
      int16_t padding = 2;
      if (changed.type == TouchOscControlType::Xy) padding = 14;
      if (changed.type == TouchOscControlType::Radar) padding = 10;
      const int16_t x1 = floorf(offsetX + changed.x * scale) - padding;
      const int16_t y1 = floorf(offsetY + changed.y * scale) - padding;
      const int16_t x2 = ceilf(offsetX + (changed.x + changed.width) * scale) + padding;
      const int16_t y2 = ceilf(offsetY + (changed.y + changed.height) * scale) + padding;
      updateX = constrain(x1, static_cast<int16_t>(0),
                          static_cast<int16_t>(screenWidth - 1));
      updateY = constrain(y1, static_cast<int16_t>(0),
                          static_cast<int16_t>(screenHeight - 1));
      const int16_t clippedX2 = constrain(x2, static_cast<int16_t>(0),
                                          static_cast<int16_t>(screenWidth - 1));
      const int16_t clippedY2 = constrain(y2, static_cast<int16_t>(0),
                                          static_cast<int16_t>(screenHeight - 1));
      updateWidth = clippedX2 - updateX + 1;
      updateHeight = clippedY2 - updateY + 1;
      partial = updateWidth > 0 && updateHeight > 0;
    }
  }
  if (!partial) {
    updateX = 0;
    updateY = 0;
    updateWidth = screenWidth;
    updateHeight = screenHeight;
    target->fillScreen(background);
  } else {
    target->fillRect(updateX, updateY, updateWidth, updateHeight, background);
  }

  for (uint8_t index = 0; index < controlCount_; ++index) {
    if (controls_[index].page != activePage_) continue;
    if (partial) {
      const Control &c = controls_[index];
      const bool unboundedText = (c.type == TouchOscControlType::Label ||
                                  c.type == TouchOscControlType::Text) &&
                                 !(c.flags & TocTextClip);
      const int16_t controlX = floorf(offsetX + c.x * scale);
      const int16_t controlY = floorf(offsetY + c.y * scale);
      const int16_t controlX2 = ceilf(offsetX + (c.x + c.width) * scale);
      const int16_t controlY2 = ceilf(offsetY + (c.y + c.height) * scale);
      if (!unboundedText &&
          (controlX2 < updateX || controlY2 < updateY ||
           controlX >= updateX + updateWidth ||
           controlY >= updateY + updateHeight)) {
        continue;
      }
    }
    drawControl(target, controls_[index], scale, offsetX, offsetY);
  }
  dirty_ = false;
  fullRedraw_ = false;
  dirtyControl_ = -1;
}

int8_t TouchOscLayout::hitTest(float x, float y) const {
  // Exact geometry always wins, particularly where intentionally overlapping
  // controls are present.
  for (int16_t index = controlCount_ - 1; index >= 0; --index) {
    const Control &c = controls_[index];
    if (c.page != activePage_ || !(c.flags & TocVisible) ||
        !(c.flags & TocInteractive) ||
        c.type == TouchOscControlType::Grid) continue;
    if (x >= c.x && x < c.x + c.width && y >= c.y && y < c.y + c.height) {
      if (c.flags & TocCircle) {
        const float dx = (x - c.x) / c.width * 2.0f - 1.0f;
        const float dy = (y - c.y) / c.height * 2.0f - 1.0f;
        if (dx * dx + dy * dy > 1.0f) continue;
      }
      return index;
    }
  }

  // Short horizontal faders are harder to acquire on the small rounded
  // display. Give faders a modest forgiving halo only after no exact control
  // was hit, so the halo cannot steal a touch from a neighbouring control.
  constexpr float kFaderTouchPadding = 10.0f;
  for (int16_t index = controlCount_ - 1; index >= 0; --index) {
    const Control &c = controls_[index];
    if (c.page != activePage_ || c.type != TouchOscControlType::Fader ||
        !(c.flags & TocVisible) || !(c.flags & TocInteractive)) continue;
    if (x >= c.x - kFaderTouchPadding &&
        x < c.x + c.width + kFaderTouchPadding &&
        y >= c.y - kFaderTouchPadding &&
        y < c.y + c.height + kFaderTouchPadding) {
      return index;
    }
  }
  return -1;
}

void TouchOscLayout::beginTouch(int16_t screenX, int16_t screenY,
                                int16_t screenWidth, int16_t screenHeight) {
  float scale, offsetX, offsetY;
  calculateTransform(screenWidth, screenHeight, scale, offsetX, offsetY);
  const float x = (screenX - offsetX) / scale;
  const float y = (screenY - offsetY) / scale;
  activeControl_ = hitTest(x, y);
  if (activeControl_ < 0) return;
  const Control &c = controls_[activeControl_];
  const float cx = c.x + c.width * 0.5f;
  const float cy = c.y + c.height * 0.5f;
  previousAngle_ = atan2f(y - cy, x - cx) -
                   orientationAngle(controlOrientation(c.flags));
  updateControl(activeControl_, x, y, true);
}

void TouchOscLayout::moveTouch(int16_t screenX, int16_t screenY,
                               int16_t screenWidth, int16_t screenHeight) {
  if (activeControl_ < 0) return;
  float scale, offsetX, offsetY;
  calculateTransform(screenWidth, screenHeight, scale, offsetX, offsetY);
  updateControl(activeControl_, (screenX - offsetX) / scale,
                (screenY - offsetY) / scale, false);
}

void TouchOscLayout::markControlDirty(uint8_t index) {
  if (index >= controlCount_ || controls_[index].page != activePage_) return;
  if (!dirty_) {
    dirty_ = true;
    fullRedraw_ = false;
    dirtyControl_ = index;
  } else if (!fullRedraw_ && dirtyControl_ != static_cast<int8_t>(index)) {
    // Multiple controls changed before the next frame. A full refresh avoids
    // dropping either update and is uncommon during a single-touch gesture.
    fullRedraw_ = true;
    dirtyControl_ = -1;
  }
}

void TouchOscLayout::updateControl(uint8_t index, float x, float y, bool forceSend) {
  Control &c = controls_[index];
  const float nx = constrain((x - c.x) / max<int16_t>(1, c.width), 0.0f, 1.0f);
  const float ny = constrain(1.0f - (y - c.y) / max<int16_t>(1, c.height), 0.0f, 1.0f);
  float nextX = c.values[0];
  float nextY = c.values[1];
  if (c.type == TouchOscControlType::Button) {
    nextX = 1.0f;
  } else if (c.type == TouchOscControlType::Fader) {
    const uint8_t orientation = controlOrientation(c.flags);
    if (orientation == 1) nextX = nx;
    else if (orientation == 2) nextX = 1.0f - ny;
    else if (orientation == 3) nextX = 1.0f - nx;
    else nextX = ny;
    if (c.flags & TocInverted) nextX = 1.0f - nextX;
  } else if (c.type == TouchOscControlType::Xy) {
    const uint8_t orientation = controlOrientation(c.flags);
    float orientedX = nx;
    float orientedY = ny;
    if (orientation == 1) {
      orientedX = 1.0f - ny;
      orientedY = nx;
    } else if (orientation == 2) {
      orientedX = 1.0f - nx;
      orientedY = 1.0f - ny;
    } else if (orientation == 3) {
      orientedX = ny;
      orientedY = 1.0f - nx;
    }
    if (!(c.flags & TocLockX)) nextX = orientedX;
    if (!(c.flags & TocLockY)) nextY = orientedY;
  } else {
    const float cx = c.x + c.width * 0.5f;
    const float cy = c.y + c.height * 0.5f;
    const float dx = (x - cx) / max(1.0f, c.width * 0.5f);
    const float dy = (y - cy) / max(1.0f, c.height * 0.5f);
    const float angle = atan2f(dy, dx);
    const float orientedAngle = angle -
                                orientationAngle(controlOrientation(c.flags));
    if (c.type == TouchOscControlType::Radar) {
      if (!(c.flags & TocLockX)) nextX = constrain(sqrtf(dx * dx + dy * dy), 0.0f, 1.0f);
      if (!(c.flags & TocLockY)) nextY = normalizedAngle(orientedAngle + kPi / 2.0f) / (2.0f * kPi);
    } else if (c.type == TouchOscControlType::Encoder && (c.flags & TocRelative)) {
      float delta = orientedAngle - previousAngle_;
      if (delta > kPi) delta -= 2.0f * kPi;
      if (delta < -kPi) delta += 2.0f * kPi;
      nextX = constrain(c.values[0] + delta / (2.0f * kPi), 0.0f, 1.0f);
      nextY = delta > 0.0001f ? 1.0f : (delta < -0.0001f ? 0.0f : 0.5f);
      previousAngle_ = orientedAngle;
    } else if (c.type == TouchOscControlType::Encoder) {
      nextX = normalizedAngle(orientedAngle + kPi / 2.0f) / (2.0f * kPi);
      nextY = nextX >= c.values[0] ? 1.0f : 0.0f;
    } else {
      const float start = 2.0f * kPi / 3.0f;
      const float span = 5.0f * kPi / 3.0f;
      float relative = normalizedAngle(orientedAngle - start);
      relative = constrain(relative, 0.0f, span);
      nextX = relative / span;
      if (c.flags & TocInverted) nextX = 1.0f - nextX;
    }
  }
  const bool changed = fabsf(nextX - c.values[0]) > 0.0005f ||
                       fabsf(nextY - c.values[1]) > 0.0005f;
  c.values[0] = nextX;
  c.values[1] = nextY;
  if (changed) markControlDirty(index);
  if ((changed || forceSend) && (c.flags & TocSend) && c.valueCount > 0 &&
      (forceSend || millis() - lastSendMs_ >= kSendIntervalMs)) {
    enqueue(c);
    lastSendMs_ = millis();
  }
}

void TouchOscLayout::endTouch() {
  if (activeControl_ < 0) return;
  Control &c = controls_[activeControl_];
  if (c.type == TouchOscControlType::Button) {
    c.values[0] = 0.0f;
    markControlDirty(activeControl_);
  }
  if ((c.flags & TocSend) && c.valueCount > 0) enqueue(c);
  activeControl_ = -1;
}

void TouchOscLayout::enqueue(const Control &control) {
  const uint8_t next = (eventWrite_ + 1) % kEventQueueSize;
  if (next == eventRead_) eventRead_ = (eventRead_ + 1) % kEventQueueSize;
  TouchOscEvent &event = events_[eventWrite_];
  strlcpy(event.address, control.address, sizeof(event.address));
  event.values[0] = control.values[0];
  event.values[1] = control.values[1];
  event.valueCount = control.valueCount;
  eventWrite_ = next;
}

bool TouchOscLayout::popEvent(TouchOscEvent &event) {
  if (eventRead_ == eventWrite_) return false;
  event = events_[eventRead_];
  eventRead_ = (eventRead_ + 1) % kEventQueueSize;
  return true;
}

bool TouchOscLayout::controlActive(const String &address) const {
  return activeControl_ >= 0 && address == controls_[activeControl_].address;
}

bool TouchOscLayout::applyRemote(const String &address, const float *values,
                                 uint8_t valueCount) {
  bool applied = false;
  for (uint8_t index = 0; index < controlCount_; ++index) {
    Control &c = controls_[index];
    if (!(c.flags & TocReceive) || address != c.address ||
        c.valueCount != valueCount || static_cast<int8_t>(index) == activeControl_) continue;
    for (uint8_t value = 0; value < valueCount; ++value) {
      if (!isfinite(values[value])) return false;
      c.values[value] = constrain(values[value], 0.0f, 1.0f);
    }
    applied = true;
    markControlDirty(index);
  }
  return applied;
}

bool TouchOscLayout::applyRemoteText(const String &address, const String &text) {
  bool applied = false;
  for (uint8_t index = 0; index < controlCount_; ++index) {
    Control &c = controls_[index];
    if (!(c.flags & TocReceive) || c.valueCount != 3 || address != c.address ||
        static_cast<int8_t>(index) == activeControl_) continue;
    strlcpy(c.text, text.c_str(), sizeof(c.text));
    applied = true;
    markControlDirty(index);
  }
  return applied;
}
