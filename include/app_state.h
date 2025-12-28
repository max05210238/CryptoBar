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
// V0.99a: CLK=GPIO2, DT=GPIO1 (ESP32-S3 PCNT-compatible)
// V0.98 used GPIO 5/6 (doesn't support PCNT on ESP32-S3)
// V0.99 swapped CLK/DT assignment to fix direction issue in quadrature decoding
#define ENC_CLK_PIN 2
#define ENC_DT_PIN 1
#define ENC_SW_PIN 21

// NTP
extern const char* NTP_SERVER_1;
extern const char* NTP_SERVER_2;

// LED brightness presets
#define BRIGHTNESS_PRESETS_COUNT 3
extern const float BRIGHTNESS_PRESETS[BRIGHTNESS_PRESETS_COUNT];
extern const char* BRIGHTNESS_LABELS[BRIGHTNESS_PRESETS_COUNT];

// Update frequency presets
// V0.99r: Update interval presets: 1min, 3min, 5min, 10min (fixed array bounds bug)
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

// ==================== Display Currency (Multi-currency) =====================
// V0.99f: Extended to support 9 currencies
extern int    g_displayCurrency;
extern double g_usdToRate[CURR_COUNT];  // Exchange rates: USD -> other currencies (double for precision)
extern bool   g_fxValid;
extern time_t g_nextFxUpdateUtc;

// Backward compatibility: g_usdToTwd is a reference to g_usdToRate[CURR_TWD]
extern double& g_usdToTwd;

// ==================== Global Variables =====================

// V0.99m: API source tracking (dynamic display of actual API used)
extern const char* g_currentPriceApi;    // Current real-time price API (e.g., "Paprika", "Kraken")
extern const char* g_currentHistoryApi;  // Current historical data API (e.g., "CoinGecko", "Binance")

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
  UI_MODE_NORMAL      = 0,
  UI_MODE_MENU        = 1,
  UI_MODE_TZ_SUB      = 2,
  UI_MODE_WIFI_INFO   = 3,
  UI_MODE_COIN_SUB    = 4,
  UI_MODE_FW_CONFIRM  = 5,
  UI_MODE_MAINT       = 6,
  UI_MODE_CURRENCY_SUB = 7,  // V0.99f: Currency submenu
  UI_MODE_UPDATE_SUB   = 8   // V0.99r: Update interval submenu
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

// Currency submenu state (V0.99f)
extern int g_currencyMenuIndex;
extern int g_currencyMenuTopIndex;

// Update interval submenu state (V0.99r)
extern int g_updateMenuIndex;
extern int g_updateMenuTopIndex;

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
extern bool g_currencyDirty;  // V0.99f
extern bool g_updateDirty;    // V0.99r
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
extern double g_prefetchPrice;
extern double g_prefetchChange;

extern double g_lastPriceUsd;
extern double g_lastChange24h;
extern bool   g_lastPriceOk;

// Previous day average reference price
extern double g_prevDayRefPrice;
extern bool   g_prevDayRefValid;

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

// ==================== Independent Time Refresh (V0.99q) =====================
// Refresh time display every minute (independent of price update interval)
extern time_t g_nextTimeRefreshUtc;   // Next scheduled time-only refresh (UTC timestamp)
extern bool   g_timeRefreshEnabled;    // Enable/disable independent time refresh (default: true)
