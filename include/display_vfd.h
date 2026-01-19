// CryptoBar Retro V0.99s - VFD Display Implementation
// 16-character IGL VFD display with dual-page rotation
#pragma once

#include "display_interface.h"
#include <Arduino.h>

// VFD-specific menu items (9 items, simplified from e-ink's 13)
enum VfdMenuItem {
  VFD_MENU_COIN = 0,
  VFD_MENU_UPDATE,
  VFD_MENU_LED_BRIGHTNESS,
  VFD_MENU_VFD_BRIGHTNESS,
  VFD_MENU_CURRENCY,
  VFD_MENU_TIMEZONE,
  VFD_MENU_FIRMWARE,
  VFD_MENU_WIFI_INFO,
  VFD_MENU_EXIT,
  VFD_MENU_COUNT
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
  void drawUpdateIntervalList();  // VFD-specific update interval submenu
  void drawFirmwareUpdateConfirmScreen(const char* version);  // VFD-specific firmware update confirmation
  void drawWifiInfoScreen(const char* version, const char* mac, const char* staIp, int signalBars, int channel, bool connected);  // VFD-specific WiFi info

  void drawSettingsScreen(const char* key, const char* value) override;
  void drawWifiSetupScreen(const char* ssid, const char* ip) override;
  void drawWifiCooldownProgress(uint32_t elapsedMs, uint32_t totalMs);  // WiFi retry cooldown progress bar
  void drawOtaScreen(const char* status) override;
  void drawErrorScreen(const char* message) override;

  // VFD-specific helper: show arbitrary text (for boot sequence, debug, etc.)
  void showText(const char* text);

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
  void runNightMode();        // Night mode (04:00-06:00, anti-burn-in at 04:00)
  void handleEncoderActivity(); // Handle encoder activity (temporary brightness boost)

  // Configuration
  static const uint16_t SCROLL_DURATION_MS = 500;  // Scroll animation time (adjustable)
  static const uint8_t MENU_TEXT_MAX_LEN = 14;     // Max text length before auto-scroll (16 - 2 for padding)
  static const uint32_t TEMP_BRIGHT_BOOST_DURATION_MS = 5UL * 60UL * 1000UL;  // 5 minutes

private:
  // Menu display helpers
  void drawMenuItemText(VfdMenuItem item);  // Draw menu item with auto-scroll if needed
  void getMenuItemText(VfdMenuItem item, char* output, uint8_t maxLen);  // Get menu item text
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

  // Page rotation state (NTP-synced)
  uint8_t currentPage;        // 1=price, 2=change%
  time_t priceUpdateTime;     // UTC time of last price update
  char lastPageContent[17];   // Cache of last displayed page (for scroll animation)

  // Brightness control
  uint8_t currentBrightness;  // 0-255
  unsigned long tempBrightBoostEndTime;  // Temporary brightness boost end time (millis)
  uint8_t savedBrightness;    // Saved brightness before temp boost

  // Helper functions
  void drawPricePage();       // Page 1: "BTC   90651.3437"
  void drawChangePage();      // Page 2: "BTC  24H +0.29%"
  void centerText(const char* text, char* output, uint8_t totalWidth, uint8_t prefixLen);
  void formatPrice(double price, const char* coin, char* output, uint8_t maxLen);
  void formatChange(double change, const char* coin, char* output, uint8_t maxLen);
};
