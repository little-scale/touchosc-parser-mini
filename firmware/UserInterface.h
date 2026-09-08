#pragma once

#include <Adafruit_XCA9554.h>
#include <Arduino_DriveBus_Library.h>
#include <Arduino_GFX_Library.h>
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

#include "AppTypes.h"
#include "TouchOscLayout.h"

enum class UiEventType : uint8_t {
  Control,
  WallCollision,
  BallCollision,
  ParticleWall,
  PendulumState,
  PendulumPing,
  PendulumActive,
  WifiCredentials,
  OscSettings,
  DeviceName,
  PhysicsSettings,
  ToggleBle,
  ToggleImuOutput,
  ClearBleBonds,
};

struct UiEvent {
  UiEventType type = UiEventType::Control;
  MessageType control = MessageType::Invalid;
  uint8_t index = 0;
  float values[32] = {};
  bool state = false;
  String text[2];
  uint16_t numbers[2] = {};
};

class UserInterface {
 public:
  bool begin(const DeviceSettings &settings);
  void loop(ControlState &state, const ImuFrame &imu, float micEnergy,
            const float spectrum[32], bool spectrumReady,
            bool wifiConnected, bool bleEnabled, bool bleConnected,
            bool imuOutputEnabled);
  bool popEvent(UiEvent &event);
  bool popTouchOscEvent(TouchOscEvent &event) { return touchOscLayout_.popEvent(event); }
  bool applyTouchOscRemote(const String &address, const float *values,
                           uint8_t valueCount) {
    return touchOscLayout_.applyRemote(address, values, valueCount);
  }
  bool applyTouchOscRemoteText(const String &address, const String &text) {
    return touchOscLayout_.applyRemoteText(address, text);
  }
  TouchOscLayout &touchOscLayout() { return touchOscLayout_; }
  bool touchOscLayoutActive() const { return touchOscLayout_.active(); }
  void openWifiSetup(bool openNetworkPicker = false);
  bool wifiSetupActive() const { return wifiSetupActive_; }
  bool fftCaptureActive() const { return fftCaptureActive_; }
  bool spectrumPageActive() const { return controlPage_ == 7 && !wifiSetupActive_; }
  void wake();
  void noteMotion();
  void forceRedraw();
  bool isControlActive(MessageType type, uint8_t index) const;

 private:
  enum class TouchTarget : uint8_t {
    None,
    Xy,
    Fader0,
    Fader1,
    Fader2,
    Fader3,
    Button0,
    Button1,
    Button2,
    Button3,
    BallArena,
    Pendulum,
    Keyboard,
    OctaveDown,
    OctaveUp,
    Spectrum,
    FftCapture,
    Page,
    Wifi,
    Ble,
    ImuOutput,
  };

  void readTouch(ControlState &state);
  bool initializeTouch(uint8_t attempts);
  void serviceTouchRecovery();
  bool readTouchPoint(bool &pressed, int16_t &x, int16_t &y);
  void readWifiSetupTouch();
  void handleWifiSetupTap(int16_t x, int16_t y);
  void startWifiScan();
  void serviceWifiSetup();
  void closeWifiSetup();
  void drawWifiSetup();
  void drawSettingsMenu(Arduino_GFX *target);
  void drawWifiNetworks(Arduino_GFX *target);
  void drawWifiKeyboard(Arduino_GFX *target);
  void drawOscSettings(Arduino_GFX *target);
  void drawDeviceSettings(Arduino_GFX *target);
  void drawPhysicsSettings(Arduino_GFX *target);
  void drawWifiMessage(Arduino_GFX *target, const char *message);
  bool oscSettingsValid() const;
  bool deviceNameValid() const;
  const char *keyboardRow(uint8_t row) const;
  TouchTarget hitTest(int16_t x, int16_t y) const;
  void beginTouch(TouchTarget target, int16_t x, int16_t y, ControlState &state);
  void moveTouch(int16_t x, int16_t y, ControlState &state);
  void endTouch(ControlState &state);
  void updateXy(int16_t x, int16_t y, ControlState &state, bool finalUpdate = false);
  void updateFader(uint8_t index, int16_t y, ControlState &state,
                   bool finalUpdate = false);
  int8_t keyboardKeyAt(int16_t x, int16_t y) const;
  uint8_t keyboardVelocityAt(int8_t key, int16_t y) const;
  void updateKeyboardKey(int8_t key, int16_t y, ControlState &state);
  void shiftKeyboardOctave(int8_t direction, ControlState &state);
  void updateSpectrumSlider(int16_t x, int16_t y, ControlState &state,
                            bool forceSend = false);
  void startFftCapture(ControlState &state);
  void finishFftCapture(ControlState &state);
  void applyFftSpectrum(const float spectrum[32], ControlState &state);
  void handleBallTap(int16_t x, int16_t y);
  void resetBalls();
  void updateBall(const ImuFrame &imu);
  void beginPendulumDraw(int16_t x, int16_t y);
  void movePendulumDraw(int16_t x, int16_t y, bool forceSend = false);
  void finishPendulumDraw();
  void resetPendulumsToCentre();
  void updatePendulum(const ImuFrame &imu);
  void enqueuePendulumState(uint8_t index, bool force = false);
  void enqueuePendulumPing(uint8_t index, uint8_t ping);
  void enqueuePendulumActive(uint8_t index, bool active);
  void updateParticles(float micEnergy, const ImuFrame &imu);
  float nextParticleRandom();
  void enqueueParticleWall(uint8_t wall, float normalizedSize);
  void enqueue(const UiEvent &event);

  void drawAll(const ControlState &state, bool wifiConnected, bool bleEnabled,
               bool bleConnected, bool imuOutputEnabled);
  void drawTop(bool wifiConnected, bool bleEnabled, bool bleConnected,
               bool imuOutputEnabled);
  void drawCurrentPage(const ControlState &state);
  void drawXy(const ControlState &state);
  void drawFaders(const ControlState &state);
  void drawButtons(const ControlState &state);
  void drawBall();
  void drawPendulum();
  void drawParticles();
  void drawKeyboard(const ControlState &state);
  void drawSpectrum(const ControlState &state);
  void flushKeyboardKey(uint8_t key);
  void drawPageButton();
  void flushXyRegion(int16_t x, int16_t y, int16_t width, int16_t height);
  void flushLayoutRegion(int16_t x, int16_t y, int16_t width, int16_t height);
  void drawLayoutStatusOverlay(Arduino_GFX *target, bool wifiConnected);
  void drawLandingScreen(Arduino_GFX *target, bool wifiConnected);
  void updatePalette(const ControlState &state);
  void updatePowerStatus();
  uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) const;
  uint16_t mixColor(const uint8_t background[3], uint8_t foreground, uint8_t amount) const;

  Arduino_DataBus *displayBus_ = nullptr;
  Arduino_CO5300 *display_ = nullptr;
  Arduino_Canvas *topCanvas_ = nullptr;
  Arduino_Canvas *xyCanvas_ = nullptr;
  Arduino_Canvas *setupCanvas_ = nullptr;
  uint16_t *layoutTransferBuffer_ = nullptr;
  std::shared_ptr<Arduino_IIC_DriveBus> i2cBus_;
  std::unique_ptr<Arduino_IIC> touch_;
  Adafruit_XCA9554 expander_;
  XPowersPMU power_;
  bool touchReady_ = false;
  uint32_t lastTouchRecoveryMs_ = 0;
  bool powerReady_ = false;
  TouchOscLayout touchOscLayout_;
  bool layoutWifiTouch_ = false;
  bool layoutPageTouch_ = false;
  bool ignoreTouchUntilRelease_ = false;
  uint32_t ignoreTouchStartedMs_ = 0;

  TouchTarget activeTouch_ = TouchTarget::None;
  bool touching_ = false;
  bool longActionSent_ = false;
  bool touchValueChanged_ = false;
  uint32_t touchStartedMs_ = 0;
  uint32_t lastTouchPollMs_ = 0;
  uint32_t lastXyStreamSendMs_ = 0;
  uint32_t lastFaderStreamSendMs_[4] = {};
  uint32_t lastControlDrawUs_ = 0;
  TouchTarget drawnActiveTouch_ = TouchTarget::None;
  uint8_t controlPage_ = 0;
  bool pageButtonPressed_ = false;
  static constexpr uint8_t kMaxBalls = 8;
  struct BallState {
    bool active = false;
    float x = 0.5f;
    float y = 0.5f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
  };
  BallState balls_[kMaxBalls];
  uint32_t ballPairContacts_ = 0;
  float ballGravity_ = 2.2f;
  float ballBounciness_ = 0.82f;
  uint32_t lastBallUpdateUs_ = 0;
  bool ballDirty_ = true;
  static constexpr uint8_t kMaxPendulums = 4;
  struct PendulumState {
    bool active = false;
    float ax = 164.0f;
    float ay = 45.0f;
    float length = 120.0f;
    float angle = 0.0f;
    float angularVelocity = 0.0f;
    float previousRelativeAngle = 0.0f;
    bool historyInitialized = false;
    int8_t motionDirection = 0;
    uint32_t lastUpdateUs = 0;
    uint32_t lastStreamMs = 0;
    uint32_t lastCentrePingMs = 0;
    uint32_t lastTurnPingMs = 0;
  };
  PendulumState pendulums_[kMaxPendulums];
  bool pendulumDrawing_ = false;
  int8_t activePendulumIndex_ = -1;
  bool pendulumResetHoldEligible_ = false;
  int8_t pendulumCreatedOnTouch_ = -1;
  float pendulumGravityAngle_ = 0.0f;
  bool pendulumDirty_ = true;
  static constexpr uint8_t kMaxParticles = 72;
  struct ParticleState {
    bool active = false;
    float x = 0.0f;
    float y = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float age = 0.0f;
    float lifetime = 1.0f;
    uint8_t size = 4;
  };
  ParticleState particles_[kMaxParticles];
  float particleSpawnAccumulator_ = 0.0f;
  uint32_t particleRandomState_ = 0x8364a72du;
  uint32_t lastParticleUpdateUs_ = 0;
  bool particleDirty_ = true;
  float xyFilteredPixelX_ = 0.0f;
  float xyFilteredPixelY_ = 0.0f;
  bool xyFilterInitialized_ = false;
  bool xyPartialValid_ = false;
  bool keyboardPartialValid_ = false;
  bool spectrumPartialValid_ = false;
  int8_t activeKeyboardKey_ = -1;
  uint8_t keyboardBaseMidiNote_ = 60;
  int8_t octaveButtonPressed_ = -1;
  bool fftCaptureActive_ = false;
  int8_t lastSpectrumTouchIndex_ = -1;
  float lastSpectrumTouchValue_ = 0.0f;
  uint32_t lastSpectrumStreamSendMs_ = 0;
  static constexpr int16_t kXyStripBufferWidth = 54;
  static constexpr int16_t kXyStripBufferHeight = 312;
  uint16_t xyStripBuffer_[kXyStripBufferWidth * kXyStripBufferHeight] = {};
  uint32_t lastActivityMs_ = 0;
  uint32_t lastBatteryReadMs_ = 0;
  bool dimmed_ = false;
  uint32_t dimAfterMs_ = 60000;
  int batteryPercent_ = -1;
  bool charging_ = false;
  bool topStateValid_ = false;
  bool drawnWifiConnected_ = false;
  bool drawnBleEnabled_ = false;
  bool drawnBleConnected_ = false;
  bool drawnImuOutputEnabled_ = false;
  int drawnBatteryPercent_ = -2;
  bool drawnCharging_ = false;
  bool layoutOverlayValid_ = false;
  bool drawnLayoutWifiConnected_ = false;
  int drawnLayoutBatteryPercent_ = -2;
  bool drawnLayoutCharging_ = false;

  enum class WifiSetupPage : uint8_t {
    Menu,
    Scanning,
    Networks,
    Password,
    Connecting,
    Connected,
    Osc,
    Device,
    Physics,
    Saved,
    Restarting,
  };
  static constexpr uint8_t kMaxWifiNetworks = 16;
  bool wifiSetupActive_ = false;
  bool wifiSetupTouching_ = false;
  bool wifiScanPending_ = false;
  bool wifiShift_ = false;
  bool wifiSymbols_ = false;
  bool wifiPasswordVisible_ = false;
  bool wifiPasswordError_ = false;
  WifiSetupPage wifiSetupPage_ = WifiSetupPage::Scanning;
  String wifiNetworks_[kMaxWifiNetworks];
  int32_t wifiRssi_[kMaxWifiNetworks] = {};
  bool wifiSecure_[kMaxWifiNetworks] = {};
  uint8_t wifiNetworkCount_ = 0;
  uint8_t wifiNetworkPage_ = 0;
  String wifiSelectedSsid_;
  String wifiPassword_;
  String oscTargetText_;
  String oscSendPortText_;
  String oscReceivePortText_;
  String deviceNameText_;
  String savedDeviceName_;
  String defaultDeviceName_;
  uint8_t oscActiveField_ = 0;
  bool oscInputError_ = false;
  bool deviceNameInputError_ = false;
  float physicsGravityEdit_ = 2.2f;
  float physicsBouncinessEdit_ = 0.82f;
  uint32_t wifiSetupLastActivityMs_ = 0;
  uint32_t wifiScanStartedMs_ = 0;
  uint32_t wifiScanRetryAtMs_ = 0;
  uint32_t wifiConnectStartedMs_ = 0;
  uint32_t wifiConnectedShownMs_ = 0;

  uint16_t backgroundColor_ = 0;
  uint16_t foregroundColor_ = 0xffff;
  uint16_t mutedColor_ = 0x7bef;
  uint16_t panelColor_ = 0x1082;
  uint8_t previousBackground_[3] = {255, 255, 255};
  ControlState drawnState_;
  bool hasDrawn_ = false;

  static constexpr uint8_t kEventQueueSize = 32;
  UiEvent events_[kEventQueueSize];
  uint8_t eventRead_ = 0;
  uint8_t eventWrite_ = 0;
};
