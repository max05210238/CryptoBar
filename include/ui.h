#pragma once

#include <Arduino.h>
#include "config.h"

// ===== 日期／時間格式 =====

enum DateFormat : int {
  DATE_MM_DD_YYYY = 0,
  DATE_DD_MM_YYYY = 1,
  DATE_YYYY_MM_DD = 2,
  DATE_FORMAT_COUNT
};

// 24h / 12h
enum TimeFormat : int {
  TIME_24H = 0,
  TIME_12H = 1
};

// ===== Settings 主選單項目（main.cpp & ui.cpp 共用）=====

enum MenuItem {
  MENU_COIN = 0,
  MENU_UPDATE_INTERVAL,
  MENU_REFRESH_MODE,      // ✅ NEW
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

// ===== 對外 UI API（實作在 ui.cpp）=====

// 開機歡迎畫面：中間顯示 "CryptoBar"，右下顯示版本字串
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

// 主畫面：幣別 symbol 從 currentCoin() 取得
void drawMainScreen(float priceUsd, float change24h, bool fullRefresh);

// 主設定選單
void drawMenuScreen(bool fullRefresh);
void ensureMainMenuVisible();

// 時區子選單
void drawTimezoneMenu(bool fullRefresh);
void ensureTzMenuVisible();

// 幣別子選單
void drawCoinMenu(bool fullRefresh);
void ensureCoinMenuVisible();
