// CryptoBar Retro V0.99r - VFD Display Implementation
// 16-character PT6302 VFD display with dual-page rotation
#pragma once

#include "display_interface.h"
#include <Arduino.h>

// Forward declaration - PT6302 library will be integrated when hardware arrives
// Placeholder class for now
class PT6302 {
public:
  PT6302(uint8_t clk, uint8_t rst, uint8_t cs, uint8_t din) {}
  void begin() {}
  void clear() {}
  void setCursor(uint8_t pos) {}
  void print(const char* str) {}
  void setBrightness(uint8_t level) {}
};

class DisplayVfd : public DisplayInterface {
public:
  DisplayVfd();
  ~DisplayVfd();

  // ===== DisplayInterface implementation =====
  void init() override;
  DisplayType getType() override { return DISPLAY_VFD; }

  void drawMainScreen(bool forceFullRefresh = false) override;
  void drawMainScreenTimeOnly(bool forceFullRefresh = false) override;

  void drawMenuScreen() override;
  void drawCoinList() override;
  void drawCurrencyList() override;
  void drawTimezoneList() override;

  void drawSettingsScreen(const char* key, const char* value) override;
  void drawWifiSetupScreen(const char* ssid, const char* ip) override;
  void drawOtaScreen(const char* status) override;
  void drawErrorScreen(const char* message) override;

  void clear() override;
  void sleep() override;
  void wake() override;
  void setBrightness(uint8_t level) override;

  bool supportsPartialRefresh() override { return false; }
  bool supportsColor() override { return false; }
  bool supportsBrightness() override { return true; }
  bool supportsAnimation() override { return true; }

  uint16_t getWidth() override { return 16; }   // 16 characters
  uint16_t getHeight() override { return 1; }   // Single line
  const char* getName() override { return "VFD PT6302 16-char"; }

  // ===== VFD-specific functions =====
  void updatePageRotation();  // Handle 3-second page rotation
  void scrollUp();            // Scroll-up animation (0.5 sec)
  void applyTimeBrightness(); // Time-based brightness control
  void runNightMode();        // Night mode (3:00-6:00)

private:
  PT6302* vfd;

  // Page rotation state
  uint8_t currentPage;        // 1=price, 2=change%
  uint32_t lastPageSwitch;    // millis() of last page switch
  time_t priceUpdateTime;     // UTC time of last price update

  // Brightness control
  uint8_t currentBrightness;  // 0-255

  // Helper functions
  void drawPricePage();       // Page 1: "BTC   90651.3437"
  void drawChangePage();      // Page 2: "BTC  24H +0.29%"
  void centerText(const char* text, char* output, uint8_t totalWidth, uint8_t prefixLen);
  void formatPrice(double price, const char* coin, char* output, uint8_t maxLen);
  void formatChange(double change, const char* coin, char* output, uint8_t maxLen);
};
