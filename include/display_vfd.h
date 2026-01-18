// CryptoBar Retro V0.99s - VFD Display Implementation
// 16-character IGL VFD display with dual-page rotation
#pragma once

#include "display_interface.h"
#include <Arduino.h>

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
  const char* getName() override { return "VFD IGL 16-char"; }

  // ===== VFD-specific functions =====
  void updatePageRotation();  // Handle 3-second page rotation
  void scrollUp();            // Scroll-up animation (configurable duration)
  void applyTimeBrightness(); // Time-based brightness control
  void runNightMode();        // Night mode (3:00-6:00)

  // Configuration
  static const uint16_t SCROLL_DURATION_MS = 500;  // Scroll animation time (adjustable)

private:
  // Low-level VFD communication
  void vfdWriteByte(uint8_t data);
  void vfdCommand(uint8_t cmd);
  void vfdShow();
  void vfdInit();
  void vfdClear();
  void vfdWriteStr(uint8_t pos, const char* str);
  void vfdSetBrightness(uint8_t level);  // 0-255

  // Pixel-level scrolling functions
  void scrollUpPixelLevel(const char* oldText, const char* newText);
  void scrollLeftCharLevel(const char* oldText, const char* newText);  // NEW: Horizontal scroll
  void writeCustomChar(uint8_t cgramSlot, const uint8_t* pixelData);
  void mixCharPixels(uint8_t* output, const uint8_t* oldChar, const uint8_t* newChar, uint8_t offset);

  // Page rotation state
  uint8_t currentPage;        // 1=price, 2=change%
  uint32_t lastPageSwitch;    // millis() of last page switch
  time_t priceUpdateTime;     // UTC time of last price update
  char lastPageContent[17];   // Cache of last displayed page (for pixel scrolling)

  // Brightness control
  uint8_t currentBrightness;  // 0-255

  // Helper functions
  void drawPricePage();       // Page 1: "BTC   90651.3437"
  void drawChangePage();      // Page 2: "BTC  24H +0.29%"
  void centerText(const char* text, char* output, uint8_t totalWidth, uint8_t prefixLen);
  void formatPrice(double price, const char* coin, char* output, uint8_t maxLen);
  void formatChange(double change, const char* coin, char* output, uint8_t maxLen);
};
