# CryptoBar Source Code (`src/`) Module Reference

This directory contains all source code (`.cpp` files) for the CryptoBar firmware.

**Quick Navigation:**
- [Core System](#core-system) - Application entry and state management
- [Network & Data](#network--data) - Price fetching, WiFi, time sync
- [User Interface](#user-interface) - E-paper display rendering
- [User Input](#user-input) - Rotary encoder and menu navigation
- [LED Indicator](#led-indicator) - WS2812 RGB status LED
- [Data Processing](#data-processing) - Charts, averages, coin registry
- [Settings & Storage](#settings--storage) - NVS persistence
- [Scheduler](#scheduler) - Tick-aligned update timing
- [WiFi Provisioning](#wifi-provisioning) - Initial setup portal
- [Maintenance & OTA](#maintenance--ota) - Firmware updates and safety

---

## Core System

### `main.cpp`
**Main application entry point and orchestration.**

- **Purpose:** Arduino `setup()` and `loop()`, coordinates all subsystems
- **Key responsibilities:**
  - Initialize hardware (e-paper, encoder, LED, NTP)
  - Bootstrap WiFi credentials (portal if missing)
  - Main event loop (price updates, UI refresh, input handling)
  - Tick-aligned scheduling integration
- **Dependencies:** Includes all other modules
- **Entry points:** `setup()`, `loop()`

**When to modify:** Adding new hardware, changing boot sequence, or modifying main loop logic.

---

### `app_state.cpp`
**Global application state and constants.**

- **Purpose:** Centralized state management (extracted from `main.cpp` for clarity)
- **What it stores:**
  - Current prices, exchange rates, chart data
  - UI state (current mode, menu indices, dirty flags)
  - WiFi connection state and credentials
  - User settings (coin, timezone, brightness, etc.)
  - Scheduler state (next update times, prefetch cache)
- **Key variables:**
  - `g_currentCoinIndex`, `g_displayCurrency`, `g_timezoneIndex`
  - `g_lastPriceUsd`, `g_lastChange24h`, `g_usdToRate[]`
  - `g_chartSamples[]`, `g_chartSampleCount`
  - `g_uiMode`, `g_menuIndex`, `g_appState`
- **Constants:**
  - Hardware pin definitions (e-paper, encoder, LED)
  - Version string (`CRYPTOBAR_VERSION`)
  - NTP servers, brightness presets, update interval presets

**When to modify:** Adding new global state variables or hardware pin configurations.

---

## Network & Data

### `network.cpp`
**Unified API for price fetching, historical data, and exchange rates.**

- **Purpose:** Abstract all external API calls with multi-layer fallback
- **Key functions:**
  - `fetchPrice()` - Get latest price (4-layer fallback: Binance → Kraken → Paprika → CoinGecko)
  - `bootstrapHistoryFromKrakenOHLC()` - Historical chart bootstrap (Kraken → Binance → CoinGecko)
  - `fetchExchangeRates()` - Multi-currency exchange rates (updates `g_usdToRate[]`)
  - `fetchUsdToTwdRate()` - Legacy single-currency wrapper
- **Fallback strategy:**
  - Each API has timeout and error handling
  - Automatically tries next source on failure
  - Updates `g_currentPriceApi` / `g_currentHistoryApi` for UI display
- **API sources:**
  - **Binance:** Real-time prices, klines (5min OHLC)
  - **Kraken:** Real-time prices, OHLC (for configured pairs)
  - **CoinPaprika:** Aggregated market data (30s refresh)
  - **CoinGecko:** Backup price + historical charts
  - **ExchangeRate-API:** Multi-currency rates

**When to modify:** Adding new cryptocurrency APIs, changing fallback order, or implementing new data sources.

---

### `app_wifi.cpp`
**WiFi connection management and reconnect logic.**

- **Purpose:** Handle WiFi STA connection with runtime reconnection
- **Key functions:**
  - `wifiConnect()` - Connect to saved WiFi credentials
  - Runtime reconnect handling (backoff strategy)
- **Features:**
  - Timeout-based connection (30s default)
  - Automatic reconnection attempts (3 per batch, exponential backoff)
  - Tracks `g_wifiEverConnected` to prevent aggressive retries
- **State tracking:**
  - `g_nextRuntimeReconnectMs` - Next reconnect attempt time
  - `g_runtimeReconnectBatch` - Current reconnect batch counter

**When to modify:** Changing WiFi reconnection policy or adding connection callbacks.

---

### `app_time.cpp`
**NTP time synchronization and timezone handling.**

- **Purpose:** Keep system time accurate via NTP
- **Key functions:**
  - `appTimeBegin()` - Initialize NTP client
  - `appTimeLoop()` - Periodic NTP resync (every 6 hours)
  - Timezone auto-detection (worldtimeapi.org) on first boot
- **NTP servers:**
  - Primary: `pool.ntp.org`
  - Secondary: `time.nist.gov`
- **Features:**
  - Event-driven NTP callback (ISR-safe with mutex)
  - Automatic timezone detection (IP-based, one-time on fresh device)
  - Periodic resync (6 hour interval)

**When to modify:** Changing NTP servers, resync interval, or timezone detection logic.

---

## User Interface

### `ui.cpp`
**Main e-paper UI rendering engine.**

- **Purpose:** Render all UI screens to 2.9" e-ink display (296×128 pixels)
- **Key functions:**
  - `uiDrawNormal()` - Main price display (coin, price, chart, time, date)
  - `uiDrawMenu()` - Settings menu
  - `uiDrawWifiInfo()` - WiFi provisioning instructions
  - `uiDrawMaintMode()` - Firmware update mode display
  - `uiDrawFwConfirm()` - Firmware update confirmation prompt
- **Rendering features:**
  - Partial vs. Full refresh logic (with counter-based full refresh)
  - Chart rendering (7pm ET cycle with day-average line)
  - Multi-currency symbol rendering (USD, TWD, EUR, GBP, CAD, JPY, KRW, SGD, AUD)
  - Dynamic API source display (`g_currentPriceApi` / `g_currentHistoryApi`)
  - Responsive layout (small/large time, different date formats)
- **Display library:** GxEPD2 (e-paper driver)

**When to modify:** Changing UI layout, adding new screens, or modifying chart rendering.

---

### `ui_coin.cpp`
**Coin selection submenu UI.**

- **Purpose:** Render cryptocurrency selection list
- **Key function:** `uiDrawCoinSubmenu()`
- **Features:**
  - Scrollable list of available coins (from `coins.cpp` registry)
  - Highlights current selection
  - Shows coin ticker and name

**When to modify:** Changing coin list display format.

---

### `ui_currency.cpp`
**Display currency selection submenu UI.**

- **Purpose:** Render currency selection list (USD, TWD, EUR, GBP, etc.)
- **Key function:** `uiDrawCurrencySubmenu()`
- **Features:**
  - Scrollable list of 9 supported currencies
  - Highlights current selection
  - Shows currency code and symbol

**When to modify:** Adding new currencies or changing display format.

---

### `ui_list.cpp`
**Generic scrollable list UI component.**

- **Purpose:** Reusable list rendering with scroll indicators
- **Key function:** `uiDrawList()`
- **Features:**
  - Handles pagination and scroll arrows
  - Configurable item height and selection highlight
  - Used by coin, currency, and timezone menus

**When to modify:** Changing list UI behavior across all submenus.

---

### `ui_menu.cpp`
**Main settings menu UI.**

- **Purpose:** Render main menu with all settings options
- **Key function:** `uiDrawMenuMain()`
- **Menu items:**
  1. Select Coin
  2. Display Currency (USD/TWD/EUR/etc.)
  3. LED Brightness
  4. Update Interval
  5. Refresh Mode
  6. Time Format (12h/24h)
  7. Date Format (MM/DD, DD/MM, YYYY-MM-DD)
  8. Time Size (Small/Large)
  9. Time zone
  10. Day Avg (Off / 24h mean / Cycle mean)
  11. WiFi Setup
  12. Firmware Update

**When to modify:** Adding/removing menu items or changing menu layout.

---

### `ui_timezone.cpp`
**Timezone selection submenu UI.**

- **Purpose:** Render timezone selection list
- **Key function:** `uiDrawTimezoneSubmenu()`
- **Features:**
  - Scrollable list of 27 integer UTC offsets (UTC-12 to UTC+14)
  - Displays in UTC-sorted order (`TIMEZONE_DISPLAY_ORDER[]`)
  - Highlights current selection

**When to modify:** Changing timezone list display format.

---

## User Input

### `encoder_pcnt.cpp`
**PCNT-based rotary encoder driver (hardware-accelerated).**

- **Purpose:** Read Bourns PEC11R-S0024 rotary encoder using ESP32-S3 PCNT peripheral
- **Key functions:**
  - `encoderPcntBegin()` - Initialize PCNT unit
  - `encoderPcntPoll()` - Poll counter and accumulate steps
- **Configuration:**
  - **Hardware:** Bourns PEC11R-S0024 (24 PPR, smooth/no detents)
  - **Pins:** CLK=GPIO2, DT=GPIO1 (ESP32-S3 PCNT-compatible)
  - **Mode:** X2 quadrature decoding (48 counts/rev)
  - **Step size:** 6 counts = 1 UI step (1/8 revolution)
  - **Noise filter:** 150 APB cycles (~2.5μs @ 80MHz)
  - **EMI rejection:** Threshold=16 (rejects e-ink display noise)
  - **Direction lock:** 10ms (prevents accidental reversals)
- **Tuning parameters** (edit in source):
  - `ENC_PCNT_FILTER_VAL` - Noise filter (100–300 range)
  - `ENC_COUNTS_PER_DETENT` - Counts per step (3/6/12 for different responsiveness)
  - `ENC_DIR_LOCK_MS` - Direction change filtering (0–50ms)
  - `ENC_DEBUG` - Debug verbosity (0=off, 1=basic, 2=verbose)

**When to modify:** Changing encoder hardware, adjusting sensitivity, or fixing direction issues.

---

### `app_input.cpp`
**High-level input handling (encoder + button).**

- **Purpose:** Process encoder rotation and button press events
- **Key functions:**
  - `appInputLoop()` - Poll encoder and button, update UI state
- **Features:**
  - Long-press detection (2 seconds) for special actions
  - Menu navigation (scroll + select)
  - Debounced button handling
- **Button actions:**
  - **Short press:** Select menu item / confirm action
  - **Long press (2s):** Enter menu from normal mode

**When to modify:** Changing button behavior or adding gesture support.

---

### `app_menu.cpp`
**Menu navigation logic and action handlers.**

- **Purpose:** Handle menu selection actions (what happens when user picks a menu item)
- **Key function:** `appMenuHandleSelection()`
- **Actions:**
  - Enter submenus (coin, currency, timezone)
  - Toggle settings (brightness, refresh mode, time format, etc.)
  - Trigger WiFi setup or firmware update
- **Menu state:**
  - `g_menuIndex` - Current menu item
  - `g_menuTopIndex` - Scroll position

**When to modify:** Adding new menu items or changing menu behavior.

---

## LED Indicator

### `led_status.cpp`
**WS2812 RGB LED control with trend-based color and animations.**

- **Purpose:** Visual price trend indicator with breathing effects
- **Key functions:**
  - `ledStatusBegin()` - Initialize external + onboard WS2812
  - `setLed()` / `fadeLedTo()` - Basic color control
  - `updateLedForPrice()` - Set LED color based on 24h price change
  - `ledAnimLoop()` - Non-blocking animation tick (breathing effect)
  - `ledAnimStartTask()` - Optional dedicated LED task (20–30 Hz)
  - `ledStatusService()` - Re-assert LED state to prevent "fall dark"
- **Color scheme:**
  - **Red:** Price down (-5% or more → breathing)
  - **Green:** Price up (+5% or more → breathing)
  - **Purple:** Small change (< ±5%)
  - **Yellow:** No data / API error
  - **Rainbow party mode:** +20% or more (celebration animation)
- **Brightness:**
  - Master brightness from user setting (`g_ledBrightness`)
  - 3 presets: Low (0.1), Medium (0.3), High (1.0)
- **Hardware:**
  - External WS2812: GPIO 15 (primary indicator)
  - Onboard WS2812: GPIO 48 (forced off to save power)

**When to modify:** Changing color thresholds, adding new animations, or adjusting brightness levels.

---

## Data Processing

### `coins.cpp`
**Cryptocurrency registry (single source of truth).**

- **Purpose:** Define all supported cryptocurrencies
- **Key functions:**
  - `coinCount()` - Number of registered coins
  - `coinAt(index)` - Get coin info by index
  - `coinIndexByTicker()` - Find coin by ticker string (e.g., "XRP")
- **Supported coins (as of current version):**
  - Bitcoin (BTC), Ethereum (ETH), Ripple (XRP), Litecoin (LTC)
  - Bitcoin Cash (BCH), Cardano (ADA), Polkadot (DOT), Chainlink (LINK)
  - Stellar (XLM), Dogecoin (DOGE), Polygon (MATIC), Solana (SOL)
  - Avalanche (AVAX), Uniswap (UNI), Cosmos (ATOM), Filecoin (FIL)
  - NEAR Protocol (NEAR), Algorand (ALGO), VeChain (VET), Hedera (HBAR)
  - Tron (TRX), EOS (EOS), Tezos (XTZ), Monero (XMR)
  - Maker (MKR), Aave (AAVE), Synthetix (SNX), Compound (COMP)
- **Data structure:**
  - `ticker` - Symbol (e.g., "BTC")
  - `name` - Full name (e.g., "Bitcoin")
  - `coinPaprikaId` - CoinPaprika API ID
  - `coinGeckoId` - CoinGecko API ID
  - `krakenPair` - Kraken trading pair (optional, for OHLC)

**When to modify:** Adding/removing cryptocurrencies or changing API identifiers.

---

### `chart.h` (header only, logic in `network.cpp`)
**Chart data structure and 7pm ET cycle management.**

- **Purpose:** Store intraday price samples for chart rendering
- **Data structure:**
  - `ChartSample` - (position 0.0–1.0 within day, price in USD)
  - `g_chartSamples[]` - Array of up to 300 samples
  - `g_chartSampleCount` - Number of valid samples
- **7pm ET cycle:**
  - Chart resets at 7pm US Eastern Time daily
  - Samples are normalized to relative position within 24h window
- **API functions:**
  - `updateEtCycle()` - Recalculate cycle boundaries
  - `addChartSampleForNow()` - Add sample at current time

**When to modify:** Changing chart sample rate or cycle anchor time.

---

### `day_avg.cpp`
**Day-average reference line calculation.**

- **Purpose:** Calculate and display reference price line on chart
- **Modes:**
  - **Off** - No average line
  - **Rolling 24h mean** - Average of last 24 hours (5-min buckets)
  - **ET cycle mean** - Average since 7pm ET (current cycle)
- **Key functions:**
  - `dayAvgRollingAdd()` - Add sample to rolling window
  - `dayAvgRollingGet()` - Get current 24h mean
  - `dayAvgCycleMean()` - Calculate cycle mean from chart buffer
  - `dayAvgRollingReset()` - Clear rolling buffer
- **Storage:**
  - Rolling buffer: 288 buckets (24h × 60min ÷ 5min)
  - Cycle mean: Computed from `g_chartSamples[]`

**When to modify:** Changing averaging algorithm or adding new average modes.

---

## Settings & Storage

### `settings_store.cpp`
**NVS (Non-Volatile Storage) persistence for user settings.**

- **Purpose:** Save/load user preferences to flash memory
- **Key functions:**
  - `settingsStoreLoad()` - Load all settings from NVS
  - `settingsStoreSave()` - Save all settings to NVS
  - `settingsStoreLoadWifi()` / `settingsStoreSaveWifi()` - WiFi credentials
  - `settingsStoreClearWifi()` - Erase WiFi credentials
  - `settingsStoreHasTzIndex()` - Check if timezone was ever set (for auto-detect)
- **Stored settings:**
  - `coinTicker` - Selected coin (e.g., "XRP")
  - `updPreset` - Update interval preset (0–4)
  - `briPreset` - LED brightness preset (0–2)
  - `rfMode` - Refresh mode (0=Partial, 1=Full)
  - `timeFmt` - Time format (0=24h, 1=12h)
  - `dateFmt` - Date format (0=MM/DD, 1=DD/MM, 2=YYYY-MM-DD)
  - `dtSize` - Date/Time size (0=Small, 1=Large)
  - `tzIndex` - Timezone index (0–26)
  - `dispCur` - Display currency (0=USD, 1=TWD, 2=EUR, etc.)
  - `dayAvg` - Day average mode (0=Off, 1=Rolling, 2=Cycle)
- **WiFi credentials:**
  - `w_ssid` - WiFi SSID
  - `w_pass` - WiFi password
- **Backward compatibility:**
  - Migrates old `coinIndex` (int) to `coinTicker` (string)

**When to modify:** Adding new settings or changing storage schema.

---

## Scheduler

### `app_scheduler.cpp`
**Tick-aligned update scheduler with prefetch optimization.**

- **Purpose:** Schedule price updates at predictable intervals (30s, 1min, 3min, 5min, 10min)
- **Key functions:**
  - `appSchedulerCalcNext()` - Calculate next update time (tick-aligned)
  - `appSchedulerLoop()` - Check if update is due and trigger it
- **Features:**
  - **Tick-aligned scheduling:** Updates happen at :00, :30 seconds (for 30s interval)
  - **Prefetch window:** Fetch price 5–10 seconds before display update
  - **Jitter reduction:** Random 5–10s lead time to avoid API rate limiting
  - **Time-only refresh:** Independent minute-based time display update
- **Configuration:**
  - `UPDATE_PRESETS_MS[]` - Available intervals (30s, 60s, 180s, 300s, 600s)
  - `PREFETCH_WINDOW_SEC` - Window size (30s)
  - `PREFETCH_MIN_LEAD_SEC` - Min lead time (5s)
  - `PREFETCH_FIXED_LEAD_SEC` - Max lead time (10s)

**When to modify:** Changing update intervals, prefetch timing, or scheduling algorithm.

---

## WiFi Provisioning

### `wifi_portal.cpp`
**AP-based WiFi credential provisioning portal.**

- **Purpose:** Initial setup wizard when no WiFi credentials exist
- **Key functions:**
  - `wifiPortalStart()` - Start AP mode + web portal
  - `wifiPortalLoop()` - Handle HTTP requests
  - `wifiPortalStop()` - Stop AP and web server
  - `wifiPortalHasSubmission()` - Check if user submitted credentials
  - `wifiPortalTakeSubmission()` - Retrieve submitted data
- **Portal features:**
  - **SSID:** `CryptoBar_XXXX` (last 4 MAC digits)
  - **IP:** `192.168.4.1`
  - **Web form:**
    - WiFi SSID/password
    - Initial coin selection
    - Core settings (update interval, refresh mode, brightness, time/date format, timezone, day average)
- **User flow:**
  1. Device creates open WiFi hotspot
  2. User connects phone/tablet
  3. Opens browser to `http://192.168.4.1`
  4. Fills form and submits
  5. Device saves credentials to NVS and connects to WiFi

**When to modify:** Changing portal UI, adding setup options, or customizing AP configuration.

---

## Maintenance & OTA

### `maint_mode.cpp`
**Maintenance mode web interface for firmware updates.**

- **Purpose:** OTA (Over-The-Air) firmware update via web browser
- **Key functions:**
  - `maintModeEnter()` - Start maintenance AP + web server
  - `maintModeLoop()` - Handle HTTP requests and OTA upload
  - `maintModeExit()` - Stop maintenance mode
  - `maintModeIsActive()` - Check if maintenance mode is running
  - `maintModeApSsid()` / `maintModeApIp()` - Get AP info for UI display
- **Web interface features:**
  - Device info (version, MAC, partition status, uptime)
  - OTA slot status (running partition, boot partition, safety guard)
  - Firmware upload (.bin file)
  - OTA rollback safety indicator
- **Access:**
  - **SSID:** `CryptoBar_MAINT_XXXX` (last 4 MAC digits)
  - **IP:** `http://192.168.4.1`
- **Security:**
  - Open AP (no password) - physical access required
  - HTTP only (no HTTPS)
  - User confirmation required before entering mode

**When to modify:** Changing OTA web UI or adding maintenance features.

---

### `maint_boot.cpp`
**Maintenance mode boot trigger detection.**

- **Purpose:** Detect if user wants to enter maintenance mode at boot
- **Key function:** `maintBootShouldEnter()`
- **Trigger condition:**
  - Encoder button held during power-on
  - Checked very early in `setup()`

**When to modify:** Changing boot-time maintenance trigger (e.g., different button combo).

---

### `ota_guard.cpp`
**OTA rollback safety mechanism (anti-brick protection).**

- **Purpose:** Automatically rollback to previous firmware if new firmware fails to boot
- **Key functions:**
  - `otaGuardBootBegin()` - Check for failures and trigger rollback if needed
  - `otaGuardLoop()` - Mark firmware as valid after stable uptime
  - `otaGuardSetPending()` - Called by OTA update to mark new firmware pending
  - `otaGuardIsPending()` / `otaGuardPendingSummary()` - Status queries
- **Rollback policy:**
  - **2 failed boots** → automatic rollback to previous partition
  - **25 seconds stable uptime** → mark new firmware as valid
- **Dual-partition system:**
  - ESP32-S3 has 2 OTA slots: `app0` (ota_0) and `app1` (ota_1)
  - Only one runs at a time, other is backup/update target
  - Bootloader can switch between partitions
- **NVS tracking:**
  - `ota_inprog` - OTA update pending flag
  - `ota_prev` - Previous (known-good) partition label
  - `ota_new` - New (testing) partition label
  - `ota_attempts` - Boot attempt counter
- **Layers of protection:**
  1. **NVS-based guard** (this module) - 2 failed boots → rollback
  2. **ESP-IDF native rollback** - Hardware bootloader fallback

**When to modify:** Changing rollback threshold, verification time, or adding custom recovery logic.

---

## File Organization Summary

| Module | Lines | Purpose |
|--------|-------|---------|
| `main.cpp` | ~1000 | Application entry, main loop orchestration |
| `app_state.cpp` | ~200 | Global state variables and constants |
| `network.cpp` | ~900 | API calls (price, history, FX) with fallback |
| `app_wifi.cpp` | ~130 | WiFi connection and reconnect logic |
| `app_time.cpp` | ~200 | NTP sync and timezone detection |
| `ui.cpp` | ~950 | E-paper UI rendering (all screens) |
| `ui_coin.cpp` | ~50 | Coin selection UI |
| `ui_currency.cpp` | ~60 | Currency selection UI |
| `ui_list.cpp` | ~65 | Generic scrollable list component |
| `ui_menu.cpp` | ~130 | Main menu rendering |
| `ui_timezone.cpp` | ~50 | Timezone selection UI |
| `encoder_pcnt.cpp` | ~250 | PCNT rotary encoder driver |
| `app_input.cpp` | ~130 | Input event handling |
| `app_menu.cpp` | ~340 | Menu action handlers |
| `led_status.cpp` | ~500 | WS2812 LED control and animations |
| `coins.cpp` | ~90 | Cryptocurrency registry |
| `day_avg.cpp` | ~75 | Day-average calculation |
| `settings_store.cpp` | ~130 | NVS persistence |
| `app_scheduler.cpp` | ~65 | Tick-aligned update scheduler |
| `wifi_portal.cpp` | ~670 | WiFi provisioning web portal |
| `maint_mode.cpp` | ~580 | Maintenance mode OTA web interface |
| `maint_boot.cpp` | ~15 | Maintenance mode boot trigger |
| `ota_guard.cpp` | ~180 | OTA rollback safety |
| **Total** | **~6900** | **26 source files** |

---

## Development Workflow

### Adding a New Cryptocurrency

1. Edit `coins.cpp`:
   - Add entry to `COINS[]` array with ticker, name, CoinPaprika ID, CoinGecko ID
   - Optionally add Kraken pair for OHLC support
2. Coin will automatically appear in coin selection menu

### Adding a New Setting

1. Add field to `StoredSettings` struct in `settings_store.h`
2. Add NVS key in `settings_store.cpp` (`load` and `save` functions)
3. Add global variable in `app_state.h/cpp`
4. Add menu item in `ui_menu.cpp` and `app_menu.cpp`
5. Add UI handler in appropriate `ui_*.cpp` file if needed

### Changing Display Layout

1. Edit `ui.cpp` → `uiDrawNormal()` for main screen
2. Adjust font, positioning, or add new elements
3. Test with both Partial and Full refresh modes
4. Consider small/large time size variants

### Debugging Encoder Issues

1. Enable verbose debug in `encoder_pcnt.cpp`:
   - Set `ENC_DEBUG = 2`
   - Monitor serial output for PCNT counts and GPIO levels
2. Adjust sensitivity:
   - `ENC_COUNTS_PER_DETENT` (3/6/12) - higher = less sensitive
   - `ENC_PCNT_FILTER_VAL` (100–300) - higher = more noise rejection
3. Check wiring:
   - CLK=GPIO2, DT=GPIO1, SW=GPIO21
   - Pull-up resistors if needed

### Testing OTA Rollback

1. Build firmware with intentional crash (e.g., `while(1);` in `setup()`)
2. Upload via maintenance mode
3. Device will fail to boot twice, then rollback automatically
4. Serial output will show: `"OTA safety guard: rollback to [previous_partition]"`

---

## Module Dependencies

```
main.cpp
 ├─ app_state.cpp (global state)
 ├─ app_wifi.cpp → settings_store.cpp
 ├─ app_time.cpp
 ├─ app_scheduler.cpp → app_state.cpp
 ├─ app_input.cpp → encoder_pcnt.cpp
 ├─ app_menu.cpp → app_state.cpp, settings_store.cpp
 ├─ network.cpp → coins.cpp, chart.h, day_avg.cpp
 ├─ ui.cpp → ui_*.cpp, chart.h, day_avg.cpp, coins.cpp
 ├─ led_status.cpp
 ├─ wifi_portal.cpp → coins.cpp
 ├─ maint_mode.cpp → ota_guard.cpp
 └─ ota_guard.cpp
```

**Minimal dependencies:** `coins.cpp`, `encoder_pcnt.cpp`, `settings_store.cpp` (no external dependencies)

**Highly coupled:** `main.cpp`, `app_state.cpp`, `ui.cpp`, `network.cpp` (central to architecture)

---

## See Also

- **Hardware documentation:** `hardware/` directory
- **User guides:** `docs/guides/` (DISPLAY_GUIDE.md, OTA_UPDATE_GUIDE.md, SECURITY_MECHANISMS.md)
- **Header files:** `include/README.md` for detailed API reference
