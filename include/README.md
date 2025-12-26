# CryptoBar Header Files (`include/`) API Reference

This directory contains all header files (`.h`) for the CryptoBar firmware.

**Quick Navigation:**
- [Configuration](#configuration) - Global constants and settings
- [Core System](#core-system) - Application state and version
- [Network & Data](#network--data) - API interfaces
- [User Interface](#user-interface) - Display and rendering APIs
- [User Input](#user-input) - Encoder and input handling
- [LED Indicator](#led-indicator) - WS2812 control
- [Data Structures](#data-structures) - Charts, coins, averages
- [Settings & Storage](#settings--storage) - NVS persistence
- [Scheduler](#scheduler) - Update timing
- [WiFi Provisioning](#wifi-provisioning) - Setup portal
- [Maintenance & OTA](#maintenance--ota) - Firmware updates
- [Assets](#assets) - Fonts and graphics

---

## Configuration

### `config.h`
**Global constants and configuration arrays.**

**Purpose:** Central configuration for timezones, currencies, and update intervals.

**Key definitions:**

```cpp
// Update interval (default: 30 seconds)
static const uint32_t UPDATE_INTERVAL_MS = 30UL * 1000UL;

// Timezone configuration (27 timezones: UTC-12 to UTC+14)
struct TimezoneInfo {
  const char* label;           // Display string (e.g., "UTC+08 Taipei")
  int8_t      utcOffsetHours;  // UTC offset in hours (integer only)
};
extern const TimezoneInfo TIMEZONES[];
extern const uint8_t TIMEZONE_COUNT;            // 27
extern const uint8_t DEFAULT_TIMEZONE_INDEX;    // 6 = UTC-08 Seattle

// Display currency (9 currencies: USD, TWD, EUR, GBP, CAD, JPY, KRW, SGD, AUD)
enum DisplayCurrency : uint8_t {
  CURR_USD = 0,  // US Dollar ($)
  CURR_TWD = 1,  // Taiwan Dollar (NT)
  CURR_EUR = 2,  // Euro (€)
  CURR_GBP = 3,  // British Pound (£)
  CURR_CAD = 4,  // Canadian Dollar (C$)
  CURR_JPY = 5,  // Japanese Yen (¥) - no decimals
  CURR_KRW = 6,  // Korean Won (₩) - no decimals
  CURR_SGD = 7,  // Singapore Dollar (S$)
  CURR_AUD = 8,  // Australian Dollar (A$)
  CURR_COUNT
};

// Currency metadata
struct CurrencyInfo {
  const char* code;        // 3-letter ISO code (e.g., "USD")
  const char* symbol;      // Display symbol (e.g., "$")
  bool        noDecimals;  // true for JPY, KRW (no fractional units)
  bool        twoCharSym;  // true for NT, C$, S$, A$ (use smaller font)
};
extern const CurrencyInfo CURRENCY_INFO[CURR_COUNT];
```

**Helper functions:**
- `tzIndexFromDisplayPos(pos)` - Convert display position to timezone index
- `tzDisplayPosFromTzIndex(tzIdx)` - Convert timezone index to display position

**When to modify:** Adding new timezones/currencies or changing defaults.

**Notes:**
- Timezones are **integer offsets only** (no DST, no half-hour zones)
- Taiwan is represented as **"UTC+08 Taipei"** (index 4)
- Default timezone: **UTC-08 Seattle** (index 6)
- Timezone list is displayed in **UTC-sorted order** via `TIMEZONE_DISPLAY_ORDER[]`

---

## Core System

### `app_state.h`
**Global application state and hardware pin definitions.**

**Purpose:** Centralized state management for all subsystems.

**Version:**
```cpp
extern const char* CRYPTOBAR_VERSION;  // e.g., "V0.99q"
```

**Hardware pins:**
```cpp
// E-paper display (SPI)
#define EPD_BUSY 4
#define EPD_RST  16
#define EPD_DC   17
#define EPD_CS   10
#define EPD_SCK  12
#define EPD_MOSI 11

// WS2812 RGB LED
#define NEOPIXEL_PIN     15   // External LED (primary indicator)
#define NEOPIXEL_COUNT   1
#define BOARD_RGB_PIN    48   // Onboard LED (forced off)
#define BOARD_RGB_COUNT  1

// Rotary encoder (PCNT-compatible pins)
#define ENC_CLK_PIN 2    // Encoder A/CLK
#define ENC_DT_PIN  1    // Encoder B/DT
#define ENC_SW_PIN  21   // Encoder button
```

**UI modes:**
```cpp
enum UiMode {
  UI_MODE_NORMAL      = 0,  // Main price display
  UI_MODE_MENU        = 1,  // Settings menu
  UI_MODE_TZ_SUB      = 2,  // Timezone submenu
  UI_MODE_WIFI_INFO   = 3,  // WiFi provisioning info screen
  UI_MODE_COIN_SUB    = 4,  // Coin selection submenu
  UI_MODE_FW_CONFIRM  = 5,  // Firmware update confirmation
  UI_MODE_MAINT       = 6,  // Maintenance mode (OTA)
  UI_MODE_CURRENCY_SUB = 7  // Currency submenu
};
```

**Application states:**
```cpp
enum AppState : uint8_t {
  APP_STATE_NEED_WIFI  = 0,  // No WiFi credentials, need provisioning
  APP_STATE_CONNECTING = 1,  // Connecting to WiFi
  APP_STATE_RUNNING    = 2,  // Normal operation
  APP_STATE_MAINT      = 3   // Maintenance mode (OTA)
};
```

**Key global variables:**
```cpp
// Current price data
extern double g_lastPriceUsd;
extern double g_lastChange24h;
extern bool   g_lastPriceOk;

// Multi-currency exchange rates
extern int    g_displayCurrency;
extern double g_usdToRate[CURR_COUNT];
extern bool   g_fxValid;

// API source tracking (V0.99m)
extern const char* g_currentPriceApi;     // "Paprika", "Kraken", etc.
extern const char* g_currentHistoryApi;   // "CoinGecko", "Binance", etc.

// User settings
extern int    g_currentCoinIndex;
extern int    g_timezoneIndex;
extern int    g_brightnessPresetIndex;
extern float  g_ledBrightness;
extern int    g_updatePresetIndex;
extern uint32_t g_updateIntervalMs;
extern int    g_dateFormatIndex;
extern int    g_timeFormat;
extern int    g_dtSize;
extern uint8_t g_dayAvgMode;
extern int    g_refreshMode;

// UI state
extern UiMode g_uiMode;
extern int    g_menuIndex;
extern int    g_menuTopIndex;

// WiFi credentials
extern String g_wifiSsid;
extern String g_wifiPass;
extern bool   g_hasWifiCreds;

// Display instance
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;
```

**Constants:**
```cpp
// LED brightness presets (3 levels)
#define BRIGHTNESS_PRESETS_COUNT 3
extern const float BRIGHTNESS_PRESETS[];      // { 0.1, 0.3, 1.0 }
extern const char* BRIGHTNESS_LABELS[];       // { "Low", "Medium", "High" }

// Update interval presets (5 levels: 30s, 1m, 3m, 5m, 10m)
#define UPDATE_PRESETS_COUNT 5
extern const uint32_t UPDATE_PRESETS_MS[];
extern const char* UPDATE_PRESET_LABELS[];

// Date format labels
extern const char* DATE_FORMAT_LABELS[];      // MM/DD, DD/MM, YYYY-MM-DD

// Refresh mode labels
extern const char* REFRESH_MODE_LABELS[];     // Partial, Full

// NTP servers
extern const char* NTP_SERVER_1;              // "pool.ntp.org"
extern const char* NTP_SERVER_2;              // "time.nist.gov"

// Partial refresh limit (reset threshold for full refresh)
extern const uint16_t PARTIAL_REFRESH_LIMIT;  // 100

// NTP resync interval (6 hours)
extern const uint32_t NTP_RESYNC_INTERVAL_SEC;
```

**When to modify:** Adding new global state or hardware pins.

---

## Network & Data

### `network.h`
**Unified API for price fetching, historical data, and exchange rates.**

**Purpose:** Abstract all external API calls with multi-layer fallback.

**API functions:**

```cpp
// Fetch latest price (4-layer fallback: Binance → Kraken → Paprika → CoinGecko)
// Returns: true if successful, priceUsd/change24h populated
bool fetchPrice(double& priceUsd, double& change24h);

// Historical chart bootstrap (3-layer fallback)
// - Kraken OHLC (5min) → Binance klines → CoinGecko market_chart
void bootstrapHistoryFromKrakenOHLC();

// Fetch exchange rates for all 9 currencies
// Updates g_usdToRate[] array
// Returns: true if at least 50% of rates fetched successfully
bool fetchExchangeRates();

// Legacy: USD → TWD rate only (backward compatibility)
bool fetchUsdToTwdRate(float& outRate);
```

**Fallback strategy:**
- Each API has timeout and error handling
- Automatically tries next source on failure
- Updates `g_currentPriceApi` / `g_currentHistoryApi` for UI display

**When to modify:** Adding new API sources or changing fallback order.

---

### `app_wifi.h`
**WiFi connection management.**

```cpp
// Connect to WiFi using saved credentials
// Returns: true if connected within timeout
bool wifiConnect(const String& ssid, const String& pass, uint32_t timeoutMs);

// Runtime reconnection management (called in loop)
void wifiRuntimeReconnectCheck();
```

---

### `app_time.h`
**NTP time synchronization.**

```cpp
// Initialize NTP client
void appTimeBegin();

// Periodic NTP resync (call in loop)
void appTimeLoop();

// Auto-detect timezone (IP-based, one-time on fresh device)
bool appTimeAutoDetectTimezone(int& outTzIndex);
```

---

## User Interface

### `ui.h`
**Main UI rendering API.**

**Display modes:**
```cpp
// Date format options
enum DateFormat {
  DATE_FORMAT_MDY = 0,  // MM/DD
  DATE_FORMAT_DMY = 1,  // DD/MM
  DATE_FORMAT_YMD = 2   // YYYY-MM-DD
};
#define DATE_FORMAT_COUNT 3

// Date/Time size options
enum DtSize {
  DTSIZE_SMALL = 0,
  DTSIZE_LARGE = 1
};
#define DTSIZE_COUNT 2

// Refresh mode
enum RefreshMode {
  REFRESH_MODE_PARTIAL = 0,  // Partial refresh (fast, may ghosting)
  REFRESH_MODE_FULL    = 1   // Full refresh (slow, clean)
};
#define REFRESH_MODE_COUNT 2
```

**Rendering functions:**
```cpp
// Main price display
void uiDrawNormal();

// Settings menu
void uiDrawMenu();

// WiFi provisioning instructions
void uiDrawWifiInfo(const String& apSsid);

// Maintenance mode display
void uiDrawMaintMode(const String& apSsid, const String& apIp);

// Firmware update confirmation
void uiDrawFwConfirm();
```

**When to modify:** Changing UI layout or adding new screens.

---

### `ui_list.h`
**Generic scrollable list component.**

```cpp
// Render scrollable list
// - items: array of strings
// - itemCount: number of items
// - selectedIndex: highlighted item
// - topIndex: scroll position
void uiDrawList(const char** items, int itemCount,
                int selectedIndex, int topIndex,
                const char* title = nullptr);
```

**Used by:** Coin, currency, and timezone selection menus.

---

## User Input

### `encoder_pcnt.h`
**PCNT-based rotary encoder driver (hardware-accelerated).**

**Purpose:** Read Bourns PEC11R-S0024 rotary encoder using ESP32-S3 PCNT peripheral.

**Hardware configuration:**
- **Encoder:** Bourns PEC11R-S0024 (24 PPR, smooth/no detents)
- **Pins:** CLK=GPIO2, DT=GPIO1 (ESP32-S3 PCNT-compatible)
- **Mode:** X2 quadrature decoding (48 counts/rev)
- **Step size:** 6 counts = 1 UI step (1/8 revolution)

**API functions:**
```cpp
// Initialize PCNT unit
void encoderPcntBegin(int clkPin, int dtPin);

// Poll PCNT counter and accumulate steps
// - appRunning: true when UI is in normal state (discard input if false)
// - stepAccum: destination accumulator (ISR-safe with mux)
void encoderPcntPoll(bool appRunning, volatile int* stepAccum, portMUX_TYPE* mux);
```

**Tuning parameters** (edit in `encoder_pcnt.cpp`):
```cpp
// Noise filter (APB cycles, ~2.5μs @ 80MHz)
// Range: 100–300 (lower = more responsive, higher = more noise rejection)
#define ENC_PCNT_FILTER_VAL 150

// Counts per UI step
// 3 = 1/16 rev, 6 = 1/8 rev (default), 12 = 1/4 rev
#define ENC_COUNTS_PER_DETENT 6

// Direction inversion
// 0 = CW is up, 1 = reversed
#define ENC_DIR_INVERT 0

// Direction lock time (ms)
// Filters accidental direction reversals (0–50ms)
#define ENC_DIR_LOCK_MS 10

// Debug verbosity
// 0 = off, 1 = basic, 2 = verbose (every 100ms)
#define ENC_DEBUG 2
```

**When to modify:** Changing encoder hardware, adjusting sensitivity, or debugging.

---

### `app_input.h`
**High-level input handling (encoder + button).**

```cpp
// Poll encoder and button, update UI state
void appInputLoop();
```

**Button behavior:**
- **Short press:** Select menu item / confirm action
- **Long press (2s):** Enter menu from normal mode

---

### `app_menu.h`
**Menu navigation logic.**

```cpp
// Handle menu selection (what happens when user picks a menu item)
void appMenuHandleSelection(int menuIndex);
```

---

## LED Indicator

### `led_status.h`
**WS2812 RGB LED control with trend-based color and animations.**

**API functions:**

```cpp
// Initialize LED hardware
void ledStatusBegin(uint8_t neopixelPin, uint16_t neopixelCount,
                    uint8_t boardPin, uint16_t boardCount);

// Set master brightness (0.0–1.0)
void ledStatusSetMasterBrightness(float master01);
float ledStatusGetMasterBrightness();

// Get current logical RGB (unscaled by brightness)
void ledStatusGetLogicalRgb(uint8_t* r, uint8_t* g, uint8_t* b);

// Basic color control (logical RGB, brightness applied internally)
void setLed(uint8_t r, uint8_t g, uint8_t b);
void fadeLedTo(uint8_t r, uint8_t g, uint8_t b, int steps = 20, int delayMs = 15);

// Preset colors
void setLedOff();
void setLedGreen();
void setLedRed();
void setLedBlue();
void setLedPurple();
void setLedYellow();
void setLedCyan();
void setLedWhiteLow();

// Update LED based on 24h price change
// - change24h: percentage change (-100.0 to +inf)
// - priceOk: data validity (false = yellow "no data")
void updateLedForPrice(double change24h, bool priceOk);

// Non-blocking animation tick (call in loop)
void ledAnimLoop(bool appRunning, bool priceOk);

// Optional: dedicated LED task (20–30 Hz, runs on separate core)
void ledAnimStartTask(uint16_t hz = 30, uint8_t core = 0);

// Periodic service tick (re-assert LED state to prevent "fall dark")
void ledStatusService();
```

**Color scheme:**
- **Red:** Price down (-5% or more → breathing)
- **Green:** Price up (+5% or more → breathing)
- **Purple:** Small change (< ±5%)
- **Yellow:** No data / API error
- **Rainbow party mode:** +20% or more (celebration animation)

**When to modify:** Changing color thresholds or adding animations.

---

## Data Structures

### `chart.h`
**Chart data structure and 7pm ET cycle management.**

**Data structures:**
```cpp
// Single chart sample
struct ChartSample {
  float  pos;    // Relative position within day (0.0–1.0)
  double price;  // Price in USD
};

// Chart buffer (up to 300 samples)
constexpr int MAX_CHART_SAMPLES = 300;
extern ChartSample g_chartSamples[MAX_CHART_SAMPLES];
extern int         g_chartSampleCount;

// 7pm ET cycle state
extern bool   g_cycleInit;
extern time_t g_cycleStartUtc;
extern time_t g_cycleEndUtc;
```

**API functions:**
```cpp
// Recalculate today's 7pm ET cycle boundaries
void updateEtCycle();

// Add sample at current time
void addChartSampleForNow(double price);
```

**Chart cycle:**
- Resets at 7pm US Eastern Time daily
- Samples normalized to 0.0–1.0 position within 24h window

---

### `coins.h`
**Cryptocurrency registry.**

**Data structure:**
```cpp
struct CoinInfo {
  const char* ticker;         // Symbol (e.g., "BTC")
  const char* name;           // Full name (e.g., "Bitcoin")
  const char* coinPaprikaId;  // CoinPaprika API ID
  const char* coinGeckoId;    // CoinGecko API ID
  const char* krakenPair;     // Kraken trading pair (optional)
};
```

**API functions:**
```cpp
// Number of registered coins
int coinCount();

// Get coin info by index
const CoinInfo& coinAt(int index);

// Find coin by ticker string (e.g., "XRP")
// Returns: index, or -1 if not found
int coinIndexByTicker(const String& ticker);
int coinIndexByTicker(const char* ticker);
```

**Supported coins:** 28 cryptocurrencies (BTC, ETH, XRP, LTC, BCH, ADA, DOT, LINK, XLM, DOGE, MATIC, SOL, AVAX, UNI, ATOM, FIL, NEAR, ALGO, VET, HBAR, TRX, EOS, XTZ, XMR, MKR, AAVE, SNX, COMP)

**When to modify:** Adding/removing cryptocurrencies.

---

### `day_avg.h`
**Day-average reference line calculation.**

**Modes:**
```cpp
enum DayAvgMode : uint8_t {
  DAYAVG_OFF     = 0,  // No average line
  DAYAVG_ROLLING = 1,  // Rolling 24h mean (default)
  DAYAVG_CYCLE   = 2,  // ET 7pm cycle mean
};
```

**API functions:**
```cpp
// Rolling 24h mean (5-min buckets)
void dayAvgRollingReset();
void dayAvgRollingAdd(time_t sampleUtc, double price);
bool dayAvgRollingGet(time_t nowUtc, double& outMean);
int  dayAvgRollingCount();

// Cycle mean (computed from chart buffer)
bool dayAvgCycleMean(double& outMean);
```

---

## Settings & Storage

### `settings_store.h`
**NVS (Non-Volatile Storage) persistence.**

**Data structure:**
```cpp
struct StoredSettings {
  int    updPreset  = 0;     // Update interval preset (0–4)
  int    briPreset  = 1;     // LED brightness preset (0–2)
  String coinTicker = "";    // Selected coin (e.g., "XRP")
  int    timeFmt    = 1;     // Time format (0=24h, 1=12h)
  int    dateFmt    = 0;     // Date format (0=MM/DD, 1=DD/MM, 2=YYYY-MM-DD)
  int    dtSize     = 1;     // Date/Time size (0=Small, 1=Large)
  int    tzIndex    = 6;     // Timezone index (default: Seattle)
  int    dayAvg     = 1;     // Day average mode (0=Off, 1=Rolling, 2=Cycle)
  int    rfMode     = 1;     // Refresh mode (0=Partial, 1=Full)
  int    dispCur    = 0;     // Display currency (0=USD, 1=TWD, etc.)
};
```

**API functions:**
```cpp
// Load all settings from NVS (returns true even if keys missing, uses defaults)
bool settingsStoreLoad(StoredSettings& out);

// Save all settings to NVS
bool settingsStoreSave(const StoredSettings& in);

// Check if timezone was ever set (for auto-detect decision)
bool settingsStoreHasTzIndex();

// WiFi credentials
bool settingsStoreLoadWifi(String& outSsid, String& outPass);
bool settingsStoreSaveWifi(const String& ssid, const String& pass);
bool settingsStoreClearWifi();
```

**When to modify:** Adding new settings or changing storage schema.

---

## Scheduler

### `app_scheduler.h`
**Tick-aligned update scheduler with prefetch optimization.**

**API functions:**
```cpp
// Calculate next update time (tick-aligned to interval)
void appSchedulerCalcNext(time_t nowUtc);

// Check if update is due, trigger if needed
void appSchedulerLoop(time_t nowUtc);
```

**Features:**
- **Tick-aligned:** Updates at :00, :30 seconds (for 30s interval)
- **Prefetch window:** Fetch 5–10s before display update
- **Time-only refresh:** Independent minute-based time display update

---

## WiFi Provisioning

### `wifi_portal.h`
**AP-based WiFi credential provisioning portal.**

**Data structure:**
```cpp
struct WifiPortalSubmission {
  // WiFi credentials
  String ssid;
  String pass;

  // Core settings (optional, -1 = not provided)
  String coinTicker;
  int    updPreset   = -1;  // Update interval preset
  int    rfMode      = -1;  // Refresh mode
  int    briPreset   = -1;  // Brightness preset
  int    timeFmt     = -1;  // Time format
  int    dateFmt     = -1;  // Date format
  int    dtSize      = -1;  // Date/Time size
  int    dispCur     = -1;  // Display currency
  int    tzIndex     = -1;  // Timezone
  int    dayAvg      = -1;  // Day average mode
};
```

**API functions:**
```cpp
// Start portal with default SSID "CryptoBar_XXXX" (MAC last 4)
void wifiPortalStart();
void wifiPortalStart(const char* apSsid);  // Custom SSID

// Stop portal
void wifiPortalStop();

// Handle HTTP requests (call in loop)
void wifiPortalLoop();

// Status queries
bool wifiPortalIsRunning();
const String& wifiPortalApSsid();

// Check if user submitted credentials
bool wifiPortalHasSubmission();

// Retrieve submission data
bool wifiPortalTakeSubmission(WifiPortalSubmission& out);

// Legacy: WiFi + coin only
bool wifiPortalTakeSubmission(String& outSsid, String& outPass, String& outCoinTicker);

// Pre-populate form defaults
void wifiPortalSetDefaultCoinTicker(const String& ticker);
void wifiPortalSetDefaultAdvanced(int rfMode, int updPreset, int briPreset,
                                  int timeFmt, int dateFmt, int dtSize,
                                  int dispCur, int tzIndex, int dayAvg);
```

**Portal access:**
- **SSID:** `CryptoBar_XXXX` (last 4 MAC digits)
- **IP:** `http://192.168.4.1`

---

## Maintenance & OTA

### `maint_mode.h`
**Maintenance mode web interface for firmware updates.**

**API functions:**
```cpp
// Enter maintenance mode
void maintModeEnter(const char* version);

// Handle HTTP requests and OTA upload (call in loop)
void maintModeLoop();

// Exit maintenance mode
void maintModeExit(bool reboot);

// Status queries
bool maintModeIsActive();
const String& maintModeApSsid();
const String& maintModeApIp();

// Check if user requested exit (without reboot)
bool maintModeConsumeExitRequest();
```

**Access:**
- **SSID:** `CryptoBar_MAINT_XXXX` (last 4 MAC digits)
- **IP:** `http://192.168.4.1`

---

### `maint_boot.h`
**Maintenance mode boot trigger detection.**

```cpp
// Check if encoder button held during power-on
bool maintBootShouldEnter();
```

---

### `ota_guard.h`
**OTA rollback safety mechanism (anti-brick protection).**

**API functions:**
```cpp
// Call very early in setup() - may trigger rollback + reboot
void otaGuardBootBegin();

// Call frequently in loop() - marks firmware valid after 25s uptime
void otaGuardLoop();

// Called by OTA update before reboot
void otaGuardSetPending(const String& prevLabel, const String& newLabel);

// Status queries
bool otaGuardIsPending();
String otaGuardPendingSummary();
```

**Rollback policy:**
- **2 failed boots** → automatic rollback to previous partition
- **25 seconds stable uptime** → mark new firmware as valid

---

## Assets

### `SpaceGroteskBold24pt7b.h`
**Custom font for large price display.**

- **Purpose:** Bold font for main price readout
- **Size:** 24pt (optimized for e-paper)
- **Format:** Adafruit GFX font array

---

## Include File Summary

| Header | Purpose | Key API |
|--------|---------|---------|
| `config.h` | Global constants (timezones, currencies) | `TIMEZONES[]`, `CURRENCY_INFO[]` |
| `app_state.h` | Global state, pins, version | `g_lastPriceUsd`, `g_uiMode`, `g_displayCurrency` |
| `network.h` | API calls (price, FX, history) | `fetchPrice()`, `fetchExchangeRates()` |
| `app_wifi.h` | WiFi connection | `wifiConnect()` |
| `app_time.h` | NTP sync | `appTimeBegin()`, `appTimeLoop()` |
| `ui.h` | E-paper rendering | `uiDrawNormal()`, `uiDrawMenu()` |
| `ui_list.h` | Scrollable list component | `uiDrawList()` |
| `encoder_pcnt.h` | Rotary encoder driver | `encoderPcntBegin()`, `encoderPcntPoll()` |
| `app_input.h` | Input handling | `appInputLoop()` |
| `app_menu.h` | Menu logic | `appMenuHandleSelection()` |
| `led_status.h` | WS2812 LED control | `updateLedForPrice()`, `ledAnimLoop()` |
| `chart.h` | Chart data structure | `ChartSample`, `addChartSampleForNow()` |
| `coins.h` | Coin registry | `coinCount()`, `coinAt()`, `coinIndexByTicker()` |
| `day_avg.h` | Day-average calculation | `dayAvgRollingAdd()`, `dayAvgCycleMean()` |
| `settings_store.h` | NVS persistence | `settingsStoreLoad()`, `settingsStoreSave()` |
| `app_scheduler.h` | Update scheduler | `appSchedulerLoop()` |
| `wifi_portal.h` | WiFi provisioning portal | `wifiPortalStart()`, `wifiPortalTakeSubmission()` |
| `maint_mode.h` | Maintenance mode | `maintModeEnter()`, `maintModeLoop()` |
| `maint_boot.h` | Maintenance trigger | `maintBootShouldEnter()` |
| `ota_guard.h` | OTA rollback safety | `otaGuardBootBegin()`, `otaGuardLoop()` |
| `SpaceGroteskBold24pt7b.h` | Custom font | Font bitmap array |

---

## Development Patterns

### Adding a New Global Variable

1. Declare in `app_state.h`: `extern int g_myNewSetting;`
2. Define in `app_state.cpp`: `int g_myNewSetting = 0;`
3. Add to `StoredSettings` if it should persist across reboots

### Creating a New UI Screen

1. Add enum to `UiMode` in `app_state.h`
2. Create rendering function in `ui.cpp` or new `ui_*.cpp`
3. Add to main UI dispatch in `ui.cpp`
4. Handle input in `app_input.cpp`

### Adding a New API Source

1. Add API call in `network.cpp`
2. Add to fallback chain in `fetchPrice()` or `bootstrapHistoryFromKrakenOHLC()`
3. Update `g_currentPriceApi` / `g_currentHistoryApi` for UI display

---

## See Also

- **Source code documentation:** `src/README.md` for detailed module descriptions
- **User guides:** `docs/guides/` (DISPLAY_GUIDE.md, OTA_UPDATE_GUIDE.md, SECURITY_MECHANISMS.md)
- **Hardware documentation:** `hardware/` directory
