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
void drawWifiPreparingApScreen(const char* version);
void drawWifiPortalScreen(const char* version, const char* apSsid, const char* apIp);
void drawWifiConnectingScreen(const char* version, const char* ssid);
void drawWifiConnectFailedScreen(const char* version);
void drawWifiInfoScreen(const char* version, const char* mac, const char* staIp, int signalBars, int channel, bool connected);

// Firmware update / maintenance mode screens
void drawFirmwareUpdateConfirmScreen(const char* version);
void drawFirmwareUpdateApScreen(const char* version, const char* apSsid, const char* apIp);

// Main price display screen: coin symbol from currentCoin()
void drawMainScreen(float priceUsd, float change24h, bool fullRefresh);

// Main settings menu
void drawMenuScreen(bool fullRefresh);
void ensureMainMenuVisible();

// Timezone submenu
void drawTimezoneMenu(bool fullRefresh);
void ensureTzMenuVisible();

// Coin selection submenu
void drawCoinMenu(bool fullRefresh);
void ensureCoinMenuVisible();
