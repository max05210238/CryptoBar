// CryptoBar V0.98-rc3 (Refactored: Step 3)
// app_state.cpp - Global application state definitions
#include "app_state.h"
#include "day_avg.h"  // for DAYAVG_ROLLING constant

// ==================== Version =====================
const char* CRYPTOBAR_VERSION = "V0.98-rc3";

// ==================== Constants =====================

// Screen layout
const int SYMBOL_PANEL_WIDTH = 90;

// NTP
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.nist.gov";

// LED brightness presets
const float BRIGHTNESS_PRESETS[] = { 0.2f, 0.5f, 1.0f };
const char* BRIGHTNESS_LABELS[]  = { "Low", "Med", "High" };

// Update frequency presets
const uint32_t UPDATE_PRESETS_MS[] = {
  30UL * 1000UL,
  60UL * 1000UL,
  180UL * 1000UL,
  300UL * 1000UL
};
const char* UPDATE_PRESET_LABELS[] = { "30s", "60s", "3m", "5m" };

// Date format labels
const char* DATE_FORMAT_LABELS[DATE_FORMAT_COUNT] = {
  "MM/DD/YYYY",
  "DD/MM/YYYY",
  "YYYY-MM-DD"
};

// Date/Time size labels
const char* DTSIZE_LABELS[] = { "Small", "Large" };

// ==================== Refresh Mode =====================
int g_refreshMode = 1;
const char* REFRESH_MODE_LABELS[] = { "Partial", "Full" };

// ==================== Display Currency (USD/NTD) =====================
int   g_displayCurrency = (int)CURR_USD;
float g_usdToTwd        = 32.0f;
bool  g_fxValid         = false;
time_t g_nextFxUpdateUtc = 0;

// ==================== Global Variables =====================

uint32_t g_updateIntervalMs = UPDATE_INTERVAL_MS;

// Refresh statistics
uint16_t g_partialRefreshCount = 0;

// LED / update / coin settings index
int   g_brightnessPresetIndex = 1;
float g_ledBrightness         = BRIGHTNESS_PRESETS[1];
int g_updatePresetIndex  = 0;
int g_currentCoinIndex   = 0;
int g_dateFormatIndex    = DATE_MM_DD_YYYY;
int g_timeFormat         = TIME_12H;
int g_dtSize             = 1;
int g_timezoneIndex      = DEFAULT_TIMEZONE_INDEX;
uint8_t g_dayAvgMode = DAYAVG_ROLLING;

// Timezone key presence flags
bool g_tzIndexKeyPresent = false;
bool g_tzIndexKeyPresentAtBoot = false;
bool g_bootFlagsInit = false;
bool g_tzAutoAttempted   = false;

// UI state
UiMode g_uiMode = UI_MODE_NORMAL;

// Main menu & timezone submenu index
int g_menuIndex    = 0;
int g_menuTopIndex = 0;
int g_tzMenuIndex    = DEFAULT_TIMEZONE_INDEX;
int g_tzMenuTopIndex = 0;

// Coin submenu state
int g_coinMenuIndex    = 0;
int g_coinMenuTopIndex = 0;

// Encoder button state
bool          g_lastEncSw       = HIGH;
unsigned long g_encPressStart   = 0;

// Encoder ISR capture
volatile int g_encStepAccum = 0;
portMUX_TYPE g_encMux = portMUX_INITIALIZER_UNLOCKED;

// UI redraw throttling
bool g_menuDirty = false;
bool g_tzDirty   = false;
bool g_coinDirty = false;
uint32_t g_lastUiDrawMs = 0;
const uint32_t UI_DRAW_MIN_MS = 120;

// Time / price state
unsigned long lastUpdate = 0;

// ==================== Tick-aligned update scheduler =====================
const time_t TIME_VALID_MIN_UTC = 1600000000;
bool   g_timeValid      = false;
time_t g_nextUpdateUtc  = 0;

// Prefetch-to-tick
const uint32_t PREFETCH_WINDOW_SEC   = 6;
const uint32_t PREFETCH_MIN_LEAD_SEC = 2;
const uint32_t PREFETCH_FIXED_LEAD_SEC = 4;
uint32_t g_fetchJitterSec = 0;
bool   g_prefetchValid  = false;
time_t g_prefetchForUtc = 0;
float  g_prefetchPrice  = 0.0f;
float  g_prefetchChange = 0.0f;

float g_lastPriceUsd   = 0.0f;
float g_lastChange24h  = 0.0f;
bool  g_lastPriceOk    = false;

// Previous day average reference price
float g_prevDayRefPrice   = 0.0f;
bool  g_prevDayRefValid   = false;

// 7pm ET cycle state
bool   g_cycleInit     = false;
time_t g_cycleStartUtc = 0;
time_t g_cycleEndUtc   = 0;

// Local timezone offset (seconds)
int32_t g_localUtcOffsetSec = 0;

// Chart samples
ChartSample g_chartSamples[MAX_CHART_SAMPLES];
int         g_chartSampleCount = 0;

// e-paper display (defined in main.cpp)

// ==================== WiFi credentials (stored in NVS) =====================
String g_wifiSsid;
String g_wifiPass;
bool   g_hasWifiCreds = false;

// ==================== WiFi runtime reconnect guards =====================
bool     g_wifiEverConnected = false;
uint32_t g_nextRuntimeReconnectMs = 0;
uint8_t  g_runtimeReconnectBatch = 0;
const uint8_t  RUNTIME_RECONNECT_ATTEMPTS = 3;
const uint32_t RUNTIME_RECONNECT_TIMEOUT_MS = 10000;
const uint32_t RUNTIME_RECONNECT_BACKOFF_MS = 30000;

AppState g_appState = APP_STATE_RUNNING;

uint32_t g_staConnectStartMs = 0;
const uint32_t STA_CONNECT_TIMEOUT_MS = 30000;

// Partial refresh limit
const uint16_t PARTIAL_REFRESH_LIMIT = 20;

// ==================== Periodic NTP resync =====================
const uint32_t NTP_RESYNC_INTERVAL_SEC = 10UL * 60UL;
volatile bool   g_ntpEventPending = false;
volatile int64_t g_ntpEventUtc = 0;
portMUX_TYPE    g_ntpMux = portMUX_INITIALIZER_UNLOCKED;
time_t g_nextNtpResyncUtc = 0;
