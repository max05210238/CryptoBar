// CryptoBar V0.99s - Display Abstraction Interface
// Supports both E-ink and VFD displays with automatic detection
#pragma once

#include <Arduino.h>

// Display type enumeration
enum DisplayType {
  DISPLAY_UNKNOWN = 0,
  DISPLAY_EINK = 1,    // E-ink 2.9" display (original CryptoBar)
  DISPLAY_VFD = 2      // VFD 16-char display (CryptoBar Retro)
};

// Abstract display interface
// All display implementations must inherit from this class
class DisplayInterface {
public:
  virtual ~DisplayInterface() {}

  // ===== Initialization =====
  virtual void init() = 0;
  virtual DisplayType getType() = 0;

  // ===== Main screen rendering =====
  // Full screen update with price, change, and optional chart
  virtual void drawMainScreen(bool forceFullRefresh = false) = 0;

  // Time-only update (e-ink only, VFD ignores)
  virtual void drawMainScreenTimeOnly(bool forceFullRefresh = false) = 0;

  // ===== Menu rendering =====
  virtual void drawMenuScreen() = 0;
  virtual void drawCoinList() = 0;
  virtual void drawCurrencyList() = 0;
  virtual void drawTimezoneList() = 0;

  // ===== Settings screens =====
  virtual void drawSettingsScreen(const char* key, const char* value) = 0;

  // ===== Special screens =====
  virtual void drawWifiSetupScreen(const char* ssid, const char* ip) = 0;
  virtual void drawOtaScreen(const char* status) = 0;
  virtual void drawErrorScreen(const char* message) = 0;

  // ===== Display control =====
  virtual void clear() = 0;
  virtual void sleep() = 0;
  virtual void wake() = 0;

  // Brightness control (VFD only, e-ink ignores)
  virtual void setBrightness(uint8_t level) = 0;

  // ===== Display capabilities =====
  virtual bool supportsPartialRefresh() = 0;
  virtual bool supportsColor() = 0;
  virtual bool supportsBrightness() = 0;
  virtual bool supportsAnimation() = 0;

  // ===== Display info =====
  virtual uint16_t getWidth() = 0;
  virtual uint16_t getHeight() = 0;
  virtual const char* getName() = 0;
};
