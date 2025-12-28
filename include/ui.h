#pragma once

#include <Arduino.h>
#include "config.h"

// ===== Date/Time Formats =====

enum DateFormat : int {
  DATE_MM_DD_YYYY = 0,
  DATE_DD_MM_YYYY = 1,
  DATE_YYYY_MM_DD = 2,
  DATE_FORMAT_COUNT
};

enum TimeFormat : int {
  TIME_24H = 0,
  TIME_12H = 1
};

// ===== Settings Menu Items =====

enum MenuItem {
  MENU_COIN = 0,
  MENU_UPDATE_INTERVAL,
  MENU_REFRESH_MODE,
  MENU_LED_BRIGHTNESS,
  MENU_TIME_FORMAT,
  MENU_DATE_FORMAT,
  MENU_DATETIME_SIZE,
  MENU_CURRENCY,
  MENU_TIMEZONE,
  MENU_DAYAVG_LINE,
  MENU_FIRMWARE_UPDATE,
  MENU_WIFI_INFO,
  MENU_EXIT,
  MENU_COUNT
};

// ===== UI API (implemented in ui.cpp) =====

// Boot splash screen: displays "CryptoBar" with version in corner
void drawSplashScreen(const char* version);

// WiFi provisioning / status screens
void drawWifiPreparingApScreen(const char* version, bool fullRefresh = false);
void drawWifiPortalScreen(const char* version, const char* apSsid, const char* apIp, bool fullRefresh = false);
void drawWifiConnectingScreen(const char* version, const char* ssid, bool fullRefresh = false);
void drawWifiConnectFailedScreen(const char* version, bool fullRefresh = false);
void drawWifiInfoScreen(const char* version, const char* mac, const char* staIp, int signalBars, int channel, bool connected);

// Firmware update / maintenance mode screens
void drawFirmwareUpdateConfirmScreen(const char* version);
void drawFirmwareUpdateApScreen(const char* version, const char* apSsid, const char* apIp, bool fullRefresh = true);

// Main price display screen: coin symbol from currentCoin()
void drawMainScreen(double priceUsd, double change24h, bool fullRefresh);

// V0.99q: Time-only refresh (updates only date/time area, not price/chart)
// This allows the clock to stay current even with long price update intervals
void drawMainScreenTimeOnly(bool fullRefresh = false);

// Main settings menu
void drawMenuScreen(bool fullRefresh);
void ensureMainMenuVisible();

// Timezone submenu
void drawTimezoneMenu(bool fullRefresh);
void ensureTzMenuVisible();

// Coin selection submenu
void drawCoinMenu(bool fullRefresh);
void ensureCoinMenuVisible();

// Currency selection submenu (V0.99f)
void drawCurrencyMenu(bool fullRefresh);
void ensureCurrencyMenuVisible();

// Update interval selection submenu (V0.99r)
void drawUpdateMenu(bool fullRefresh);
void ensureUpdateMenuVisible();
