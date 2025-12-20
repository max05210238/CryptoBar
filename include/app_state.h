// CryptoBar V0.98 (Refactored: Step 2)
// app_state.h - Global application state (extracted from main.cpp)
#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "config.h"
#include "ui.h"
#include "chart.h"

// ==================== Version =====================
extern const char* CRYPTOBAR_VERSION;

// ==================== Constants =====================

// Screen layout
extern const int SYMBOL_PANEL_WIDTH;

// e-paper pins (ESP32-S3-DevKitC-1)
#define EPD_BUSY 4
#define EPD_RST 16
#define EPD_DC 17
#define EPD_CS 10
#define EPD_SCK 12
#define EPD_MOSI 11

// External WS2812
#define NEOPIXEL_PIN 15
#define NEOPIXEL_COUNT 1

// Onboard WS2812
#define BOARD_RGB_PIN 48
#define BOARD_RGB_COUNT 1

// Rotary encoder pins
// V0.99: Changed from GPIO 5/6 to 9/10 for ESP32-S3 PCNT compatibility
// GPIO 5/6 may be reserved for SPI Flash on some ESP32-S3 boards
#define ENC_CLK_PIN 9
#define ENC_DT_PIN 10
#define ENC_SW_PIN 21

// NTP
extern const char* NTP_SERVER_1;
extern const char* NTP_SERVER_2;

// LED brightness presets
#define BRIGHTNESS_PRESETS_COUNT 3
extern const float BRIGHTNESS_PRESETS[BRIGHTNESS_PRESETS_COUNT];
extern const char* BRIGHTNESS_LABELS[BRIGHTNESS_PRESETS_COUNT];

// Update frequency presets
#define UPDATE_PRESETS_COUNT 4
extern const uint32_t UPDATE_PRESETS_MS[UPDATE_PRESETS_COUNT];
extern const char* UPDATE_PRESET_LABELS[UPDATE_PRESETS_COUNT];

// Date format labels
extern const char* DATE_FORMAT_LABELS[DATE_FORMAT_COUNT];

// Date/Time size labels
extern const char* DTSIZE_LABELS[];

// ==================== Refresh Mode =====================
extern int g_refreshMode;
extern const char* REFRESH_MODE_LABELS[];

// ==================== Display Currency (USD/NTD) =====================
extern int   g_displayCurrency;
extern float g_usdToTwd;
extern bool  g_fxValid;
extern time_t g_nextFxUpdateUtc;

// ==================== Global Variables =====================

extern uint32_t g_updateIntervalMs;

// Refresh statistics
extern uint16_t g_partialRefreshCount;

// LED / update / coin settings index
extern int   g_brightnessPresetIndex;
extern float g_ledBrightness;
extern int g_updatePresetIndex;
extern int g_currentCoinIndex;
extern int g_dateFormatIndex;
extern int g_timeFormat;
extern int g_dtSize;
extern int g_timezoneIndex;
extern uint8_t g_dayAvgMode;

// Timezone key presence flags
extern bool g_tzIndexKeyPresent;
extern bool g_tzIndexKeyPresentAtBoot;
extern bool g_bootFlagsInit;
extern bool g_tzAutoAttempted;

// UI state
enum UiMode {
  UI_MODE_NORMAL    = 0,
  UI_MODE_MENU      = 1,
  UI_MODE_TZ_SUB    = 2,
  UI_MODE_WIFI_INFO = 3,
  UI_MODE_COIN_SUB  = 4,
  UI_MODE_FW_CONFIRM = 5,
  UI_MODE_MAINT      = 6
};
extern UiMode g_uiMode;

// Main menu & timezone submenu index
extern int g_menuIndex;
extern int g_menuTopIndex;
extern int g_tzMenuIndex;
extern int g_tzMenuTopIndex;

// Coin submenu state
extern int g_coinMenuIndex;
extern int g_coinMenuTopIndex;

// Encoder button state
extern bool          g_lastEncSw;
extern unsigned long g_encPressStart;

// Encoder ISR capture
extern volatile int g_encStepAccum;
extern portMUX_TYPE g_encMux;

// UI redraw throttling
extern bool g_menuDirty;
extern bool g_tzDirty;
extern bool g_coinDirty;
extern uint32_t g_lastUiDrawMs;
extern const uint32_t UI_DRAW_MIN_MS;

// Time / price state
extern unsigned long lastUpdate;

// ==================== Tick-aligned update scheduler =====================
extern const time_t TIME_VALID_MIN_UTC;
extern bool   g_timeValid;
extern time_t g_nextUpdateUtc;

// Prefetch-to-tick
extern const uint32_t PREFETCH_WINDOW_SEC;
extern const uint32_t PREFETCH_MIN_LEAD_SEC;
extern const uint32_t PREFETCH_FIXED_LEAD_SEC;
extern uint32_t g_fetchJitterSec;
extern bool   g_prefetchValid;
extern time_t g_prefetchForUtc;
extern float  g_prefetchPrice;
extern float  g_prefetchChange;

extern float g_lastPriceUsd;
extern float g_lastChange24h;
extern bool  g_lastPriceOk;

// Previous day average reference price
extern float g_prevDayRefPrice;
extern bool  g_prevDayRefValid;

// 7pm ET cycle state
extern bool   g_cycleInit;
extern time_t g_cycleStartUtc;
extern time_t g_cycleEndUtc;

// Local timezone offset (seconds)
extern int32_t g_localUtcOffsetSec;

// Chart samples
extern ChartSample g_chartSamples[MAX_CHART_SAMPLES];
extern int         g_chartSampleCount;

// e-paper display
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// ==================== WiFi credentials (stored in NVS) =====================
extern String g_wifiSsid;
extern String g_wifiPass;
extern bool   g_hasWifiCreds;

// ==================== WiFi runtime reconnect guards =====================
extern bool     g_wifiEverConnected;
extern uint32_t g_nextRuntimeReconnectMs;
extern uint8_t  g_runtimeReconnectBatch;
extern const uint8_t  RUNTIME_RECONNECT_ATTEMPTS;
extern const uint32_t RUNTIME_RECONNECT_TIMEOUT_MS;
extern const uint32_t RUNTIME_RECONNECT_BACKOFF_MS;

enum AppState : uint8_t {
  APP_STATE_NEED_WIFI  = 0,
  APP_STATE_CONNECTING = 1,
  APP_STATE_RUNNING    = 2,
  APP_STATE_MAINT      = 3
};
extern AppState g_appState;

extern uint32_t g_staConnectStartMs;
extern const uint32_t STA_CONNECT_TIMEOUT_MS;

// Partial refresh limit
extern const uint16_t PARTIAL_REFRESH_LIMIT;

// ==================== Periodic NTP resync =====================
extern const uint32_t NTP_RESYNC_INTERVAL_SEC;
extern volatile bool   g_ntpEventPending;
extern volatile int64_t g_ntpEventUtc;
extern portMUX_TYPE    g_ntpMux;
extern time_t g_nextNtpResyncUtc;
