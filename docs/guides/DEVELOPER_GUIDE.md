# CryptoBar Developer Guide

**Advanced technical documentation for CryptoBar development**

This guide covers internal mechanisms, architectural decisions, and advanced debugging techniques for developers working on CryptoBar firmware.

---

## 📖 Table of Contents

### Core Mechanisms
1. [⏱️ Prefetch Scheduling Mechanism](#prefetch-scheduling-mechanism)
2. [🏗️ Architecture Design Decisions](#architecture-design-decisions)
3. [🐛 Advanced Debugging Techniques](#advanced-debugging-techniques)

### Development Workflows
4. [🔧 Building and Flashing](#building-and-flashing)
5. [📊 Performance Profiling](#performance-profiling)
6. [🧪 Testing Strategies](#testing-strategies)

### Reference
7. [📚 Code Organization](#code-organization)
8. [🔗 Related Documentation](#related-documentation)

---

## ⏱️ Prefetch Scheduling Mechanism

CryptoBar uses a sophisticated **prefetch scheduling system** to ensure display updates happen precisely on schedule while minimizing perceived network latency.

### Why Prefetch?

**Problem:** Network API calls can take 500ms–2000ms, causing visible delays when updating the display.

**Solution:** Fetch price data 5–10 seconds *before* the scheduled display update, cache it, then use the cached value at the exact update time.

**Benefits:**
- ✅ **Predictable updates:** Display changes at exactly :00, :30 seconds (or other tick-aligned times)
- ✅ **Reduced latency perception:** Network delay is hidden from user
- ✅ **API rate limit protection:** Random jitter prevents simultaneous requests from multiple devices

---

### Implementation Details

**Location:** `src/app_scheduler.cpp`

#### Configuration Constants

```cpp
// Prefetch window parameters
extern const uint32_t PREFETCH_WINDOW_SEC;      // 30 seconds
extern const uint32_t PREFETCH_MIN_LEAD_SEC;    // 5 seconds minimum lead
extern const uint32_t PREFETCH_FIXED_LEAD_SEC;  // 10 seconds fixed lead
```

#### State Variables

```cpp
// Prefetch state tracking (app_state.h)
extern uint32_t g_fetchJitterSec;      // Random jitter (5-10 seconds)
extern bool     g_prefetchValid;       // Prefetch data valid flag
extern time_t   g_prefetchForUtc;      // Target UTC time for this prefetch
extern double   g_prefetchPrice;       // Cached price from prefetch
extern double   g_prefetchChange;      // Cached 24h change percentage
```

---

### Scheduling Algorithm

#### Step 1: Calculate Next Update Time (Tick-Aligned)

For a 3-minute interval, updates happen at:
```
10:00:00, 10:03:00, 10:06:00, 10:09:00, ...
```

**Tick alignment logic:**
```cpp
time_t now = time(nullptr);
uint32_t intervalSec = UPDATE_PRESETS_MS[presetIndex] / 1000;

// Round up to next tick boundary
time_t nextTick = ((now / intervalSec) + 1) * intervalSec;
```

**Example:**
- Current time: 10:01:23
- Interval: 180 seconds (3 minutes)
- Calculation: `(601 / 180 + 1) * 180 = 4 * 180 = 720` → **10:12:00**

---

#### Step 2: Generate Random Jitter

To prevent API rate limiting when many devices update simultaneously:

```cpp
// Random jitter between 5-10 seconds
g_fetchJitterSec = PREFETCH_MIN_LEAD_SEC +
                   (esp_random() % (PREFETCH_FIXED_LEAD_SEC - PREFETCH_MIN_LEAD_SEC + 1));
```

**Example:**
- `esp_random()` returns: 42
- Jitter calculation: `5 + (42 % 6)` = `5 + 0` = **5 seconds**

---

#### Step 3: Schedule Prefetch

Calculate when to fetch price data:

```cpp
time_t prefetchTime = nextTick - g_fetchJitterSec;
```

**Example:**
- Next update: 10:12:00
- Jitter: 7 seconds
- Prefetch time: **10:11:53**

---

#### Step 4: Execute Prefetch

At prefetch time (10:11:53):

```cpp
bool success = fetchPrice(&price, &change24h);

if (success) {
    g_prefetchPrice = price;
    g_prefetchChange = change24h;
    g_prefetchForUtc = nextTick;  // 10:12:00
    g_prefetchValid = true;
}
```

---

#### Step 5: Display Update

At exact update time (10:12:00):

```cpp
if (g_prefetchValid && g_prefetchForUtc == now) {
    // Use cached data (no network delay!)
    updateDisplay(g_prefetchPrice, g_prefetchChange);
    g_prefetchValid = false;  // Invalidate cache
} else {
    // Fallback: fetch immediately if prefetch failed
    double price, change;
    fetchPrice(&price, &change);
    updateDisplay(price, change);
}
```

---

### Visual Timeline Example

```
Time:  10:11:53              10:12:00
       ↓                     ↓
       [Prefetch]            [Display Update]
       │                     │
       ├─ API call (850ms)   ├─ Use cached price
       ├─ Response received  ├─ Update e-ink display
       └─ Cache: $42,350.28  └─ User sees: $42,350.28

Network latency absorbed here ↑
User perceives instant update ↑
```

**Key observation:** The 850ms network delay happens *before* the scheduled update time, so the user sees the display change at exactly 10:12:00 with no perceived delay.

---

### Prefetch Window Details

**What is the prefetch window?**

The **prefetch window** is the time range during which a cached prefetch result remains valid.

```cpp
PREFETCH_WINDOW_SEC = 30 seconds
```

**Purpose:** Prevent stale data from being displayed if the scheduler gets delayed.

**Validation logic:**
```cpp
bool isPrefetchStale(time_t now) {
    if (!g_prefetchValid) return true;

    int32_t delta = (int32_t)(now - g_prefetchForUtc);
    return (delta < 0 || delta > PREFETCH_WINDOW_SEC);
}
```

**Example scenarios:**

| Scenario | Prefetch For | Current Time | Delta | Valid? |
|----------|-------------|--------------|-------|--------|
| On time | 10:12:00 | 10:12:00 | 0s | ✅ Yes |
| Slight delay | 10:12:00 | 10:12:15 | +15s | ✅ Yes (within 30s) |
| Major delay | 10:12:00 | 10:12:45 | +45s | ❌ No (stale) |
| Too early | 10:12:00 | 10:11:55 | -5s | ❌ No (future) |

---

### Jitter Strategy

**Why random jitter?**

Without jitter, all CryptoBar devices with the same update interval would request API data at identical times:

```
10:00:00 → 10,000 devices hit Binance API simultaneously
10:03:00 → 10,000 devices hit Binance API simultaneously
10:06:00 → ...
```

This creates **API rate limit spikes** and potential service degradation.

**With jitter:**

```
10:11:53 → Device A fetches
10:11:54 → Device B fetches
10:11:55 → Device C fetches
10:11:57 → Device D fetches
...
10:11:59 → Device J fetches

Requests spread over 5-10 second window
API load is smoothed out
```

**Jitter range:** 5–10 seconds provides good load distribution while keeping prefetch safely before the update time.

---

### Edge Cases and Fallbacks

#### Case 1: Prefetch Fails

If the API call during prefetch fails:

```cpp
bool success = fetchPrice(&price, &change24h);
if (!success) {
    g_prefetchValid = false;
    // Don't cache anything
}

// Later, at display update time:
if (!g_prefetchValid) {
    // Immediate fetch as fallback
    fetchPrice(&price, &change);
    updateDisplay(price, change);
}
```

**Result:** User sees update with network delay, but still gets fresh data.

---

#### Case 2: System Clock Adjustment

If the ESP32 adjusts its clock (NTP sync correction):

```cpp
// NTP sync changed time by +3 seconds
time_t before = 10:11:58;
ntpSync();  // Time jumps forward
time_t after = 10:12:01;

// Prefetch cache is checked:
if (isPrefetchStale(after)) {
    g_prefetchValid = false;  // Invalidate stale cache
}
```

**Result:** Stale prefetch is discarded, immediate fetch happens.

---

#### Case 3: Very Long Network Delay

If prefetch API call takes longer than jitter window:

```
10:11:53 → Prefetch starts
10:11:59 → Still waiting...
10:12:00 → Update time reached (no cached data)
10:12:01 → Prefetch finally returns
```

**Handling:**
```cpp
// At 10:12:00 (update time)
if (!g_prefetchValid) {
    // Immediate fetch (blocks display update)
    fetchPrice(&price, &change);
}

// At 10:12:01 (late prefetch returns)
if (g_prefetchForUtc != nextUpdateTime) {
    // Discard - we've already moved past this update
    g_prefetchValid = false;
}
```

**Result:** First update has delay, but system recovers normally.

---

### Performance Characteristics

**Typical prefetch success rate:** 98%+ (under normal network conditions)

**Timing accuracy:**

| Metric | Value | Notes |
|--------|-------|-------|
| **Update time precision** | ±50ms | Limited by FreeRTOS tick rate (10ms) and loop overhead |
| **Prefetch lead time variance** | 5-10s | Random jitter for API load distribution |
| **Cache validity window** | 30s | Prevents stale data after scheduler delays |

**Network failure recovery:**

- **Prefetch fails:** Immediate fetch fallback (1-2s delay visible to user)
- **Both fail:** LED shows yellow (API error), display shows last known price

---

### Configuration Tuning

**Adjusting jitter range:**

```cpp
// Tighter jitter (less API load spreading, more predictable timing)
const uint32_t PREFETCH_MIN_LEAD_SEC = 8;   // Was 5
const uint32_t PREFETCH_FIXED_LEAD_SEC = 10; // Unchanged

// Result: 8-10s jitter instead of 5-10s
```

**Adjusting prefetch window:**

```cpp
// Shorter window (stricter freshness, more fallback fetches)
const uint32_t PREFETCH_WINDOW_SEC = 15;  // Was 30

// Result: Prefetch expires faster if update is delayed
```

**Trade-offs:**

| Parameter | Increase Effect | Decrease Effect |
|-----------|----------------|-----------------|
| **Jitter range** | Better API load distribution | Tighter timing predictability |
| **Window size** | Tolerate more scheduler delays | Stricter data freshness |

---

### Debug Output

Enable scheduler debug logging in `app_scheduler.cpp`:

```cpp
#define SCHEDULER_DEBUG 1

// Outputs:
[SCHED] Next update: 10:12:00
[SCHED] Jitter: 7 seconds
[SCHED] Prefetch at: 10:11:53
[SCHED] Prefetch SUCCESS: $42,350.28 (+2.34%)
[SCHED] Display update at: 10:12:00 (using cache)
```

---

## 🏗️ Architecture Design Decisions

This section documents key architectural choices made in CryptoBar's design and the rationale behind them.

### Dual OTA Partition System

**Decision:** Use ESP-IDF's built-in dual partition OTA system (app0/app1)

**Rationale:**
- ✅ Automatic rollback protection (anti-brick)
- ✅ No custom bootloader modification needed
- ✅ Well-tested by Espressif
- ✅ Only ~3KB RAM overhead for partition table

**Trade-off:** Uses 3MB flash instead of 1.5MB (dual slots)

**Implementation:** See `src/ota_guard.cpp` for rollback logic

---

### FreeRTOS Tasks vs Main Loop

**Decision:** Use main loop architecture with minimal FreeRTOS tasks

**Current task structure:**
- **Main loop (loop() function):** Price updates, display refresh, scheduler
- **Optional LED task:** Independent breathing animations (if enabled)
- **WiFi background task:** Managed by ESP-IDF WiFi stack

**Rationale:**
- ✅ Simpler code flow (easier to debug)
- ✅ Predictable execution order
- ✅ Lower RAM usage (~8KB stack per task)
- ❌ E-ink blocking during refresh (acceptable for this use case)

**Alternative considered:** Multi-task architecture
- ✅ Non-blocking e-ink refresh
- ❌ Complex synchronization (mutexes, queues)
- ❌ Higher RAM usage
- ❌ Race condition risks

**Conclusion:** Main loop is sufficient for CryptoBar's update rates (1-10 minutes)

---

### E-ink Partial Refresh with Auto-Full

**Decision:** Support partial refresh mode with automatic full refresh every 20 updates

**Rationale:**
- ✅ Fast updates (~300ms vs 2000ms)
- ✅ Ghosting prevention through periodic full refresh
- ✅ User choice (some prefer always-full)
- ✅ Battery savings (~50% less power per update)

**Constant:**
```cpp
const uint16_t PARTIAL_REFRESH_LIMIT = 20;
```

**Why 20?**
- Empirical testing showed ghosting becomes noticeable after 15-25 partials
- 20 provides safety margin
- For 3-min updates: ~60 minutes between full refreshes

**Alternative considered:** Dynamic adjustment based on content changes
- ✅ Optimal ghosting prevention
- ❌ Complex change detection logic
- ❌ Unpredictable refresh timing

---

### NVS (Non-Volatile Storage) for Settings

**Decision:** Use ESP-IDF NVS library instead of JSON config files

**Rationale:**
- ✅ Wear leveling built-in
- ✅ Power-loss safe writes
- ✅ Type-safe API
- ✅ Compact storage (~1KB for all settings)
- ❌ Not human-readable (requires code to view)

**Alternative considered:** SPIFFS + JSON
- ✅ Human-readable config
- ❌ No wear leveling (flash degradation)
- ❌ Corruption risk on power loss
- ❌ Parsing overhead

**Implementation:** See `src/settings_store.cpp`

---

### WiFi Auto-Reconnect with Backoff

**Decision:** Implement 5-minute backoff between reconnection batches

**Rationale:**
- ✅ Prevents battery drain from continuous retry
- ✅ Gives router time to recover
- ✅ 3 attempts per batch provides balance
- ❌ Up to 5 minutes offline after disconnect

**Parameters:**
```cpp
const uint8_t  RUNTIME_RECONNECT_ATTEMPTS = 3;
const uint32_t RUNTIME_RECONNECT_TIMEOUT_MS = 30000;  // 30s per attempt
const uint32_t RUNTIME_RECONNECT_BACKOFF_MS = 300000; // 5 min between batches
```

**Why 5 minutes?**
- Too short: Battery drain from constant WiFi scanning
- Too long: User frustration from long offline periods
- 5 minutes: Good balance for desk device use case

---

### API Fallback Chain

**Decision:** Implement 4-layer price fallback, 3-layer history fallback

**Price API order:**
1. Binance (fastest, most reliable)
2. Kraken (reliable, slightly slower)
3. CoinPaprika (good uptime)
4. CoinGecko (rate limits, but broad coverage)

**Rationale:**
- ✅ Binance has best uptime and lowest latency
- ✅ CoinGecko has most altcoin coverage (fallback for rare coins)
- ✅ Each layer has 10-second timeout (prevents long hangs)

**Trade-off:** Total failover time can reach 40 seconds (4 layers × 10s timeout)

**Implementation:** See `src/network.cpp:fetchPrice()`

---

### PCNT for Rotary Encoder

**Decision:** Use ESP32's PCNT (Pulse Counter) hardware module instead of GPIO interrupts

**Rationale:**
- ✅ Hardware debouncing (no software filtering needed)
- ✅ Zero CPU overhead for counting
- ✅ Works reliably during e-ink refresh blocking
- ✅ Direction detection in hardware

**Alternative considered:** GPIO interrupts
- ✅ More flexible (any pin)
- ❌ Requires software debouncing
- ❌ Interrupt overhead
- ❌ Noise sensitivity

**Implementation:** See `src/encoder_pcnt.cpp`

---

### Independent Time Refresh

**Decision:** Update time display every minute, independent of price updates

**Rationale:**
- ✅ Users expect clock to update every minute
- ✅ Price updates can be 5-10 minutes apart
- ✅ Minimal e-ink wear (partial refresh)

**Implementation:**
```cpp
// In app_scheduler.cpp
void appSchedulerLoop() {
    // Time update check (every minute)
    if (minuteChanged()) {
        uiRefreshTimeOnly();  // Partial refresh, time area only
    }

    // Price update check (every 1-10 minutes)
    if (isUpdateDue()) {
        fetchAndUpdate();  // Full or partial refresh
    }
}
```

---

### 10-Minute NTP Resync Interval

**Decision:** Resync NTP every 10 minutes

**Rationale:**
- ✅ ESP32 RTC drifts ~1-5 seconds per day
- ✅ 10 minutes keeps drift under 0.1 second
- ✅ Low network overhead (1 packet per 10 min)
- ❌ More frequent than necessary for casual use

**Why not longer?**
- Hourly: Drift could reach 0.2-0.4 seconds
- Daily: Drift could reach 1-5 seconds (noticeable)

**Why not shorter?**
- Every minute: Unnecessary network usage
- NTP servers may rate-limit overly frequent requests

**Implementation:** See `src/app_time.cpp:appTimeLoop()`

---

## 🐛 Advanced Debugging Techniques

### Serial Monitor Debug Levels

CryptoBar uses tiered debug output:

```cpp
// In config.h or individual files
#define DEBUG_LEVEL 0  // No debug output
#define DEBUG_LEVEL 1  // Critical events only
#define DEBUG_LEVEL 2  // Verbose (includes API calls, timing)
```

**Example from `app_scheduler.cpp`:**
```cpp
#if DEBUG_LEVEL >= 2
    Serial.printf("[SCHED] Prefetch at: %ld (-%ds)\n", prefetchTime, jitter);
#endif
```

**Recommended levels:**
- **Production:** 0 (silent except errors)
- **Testing:** 1 (important events)
- **Development:** 2 (full trace)

---

### OTA Guard Debug Mode

Monitor OTA rollback safety mechanism:

```cpp
// In ota_guard.cpp
#define OTA_GUARD_DEBUG 1

// Outputs:
[OTA] Boot attempt 1/2
[OTA] Uptime: 27000ms (need 25000ms)
[OTA] Boot successful, marking valid
[OTA] Cleared rollback guard
```

**Useful for:**
- Verifying rollback protection works
- Debugging boot failures
- Testing firmware stability threshold

---

### E-ink Refresh Timing Profiling

Measure actual refresh times:

```cpp
// In ui.cpp
void uiRefresh() {
    uint32_t start = millis();

    if (fullRefresh) {
        display.displayFullRefresh();
    } else {
        display.displayPartialRefresh();
    }

    uint32_t elapsed = millis() - start;
    Serial.printf("[UI] Refresh: %s (%lums)\n",
                  fullRefresh ? "FULL" : "PARTIAL", elapsed);
}

// Typical output:
// [UI] Refresh: PARTIAL (312ms)
// [UI] Refresh: FULL (2048ms)
```

---

### API Response Time Tracking

Monitor API performance:

```cpp
// In network.cpp
bool fetchPrice(double* price, double* change24h) {
    uint32_t start = millis();
    bool success = false;

    // Try Binance
    if (binanceAPI(price, change24h)) {
        success = true;
        uint32_t elapsed = millis() - start;
        Serial.printf("[API] Binance: %lums\n", elapsed);
    }

    return success;
}

// Typical output:
// [API] Binance: 487ms
// [API] Kraken: 823ms
// [API] CoinGecko: 1542ms
```

---

### WiFi Connection State Tracking

Debug WiFi reconnection logic:

```cpp
// In app_wifi.cpp
void appWifiLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        uint32_t now = millis();
        if (now >= g_nextRuntimeReconnectMs) {
            Serial.printf("[WIFI] Reconnect batch %d (attempt %d/%d)\n",
                          g_runtimeReconnectBatch,
                          g_currentAttempt,
                          RUNTIME_RECONNECT_ATTEMPTS);
            // ... reconnect logic ...
        }
    }
}
```

---

### Memory Usage Profiling

Check heap and stack usage:

```cpp
void printMemoryStats() {
    Serial.printf("\n[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[MEM] Largest block: %d bytes\n", ESP.getMaxAllocHeap());
    Serial.printf("[MEM] Min free (since boot): %d bytes\n", ESP.getMinFreeHeap());
}

// Call periodically in loop()
static uint32_t lastMemCheck = 0;
if (millis() - lastMemCheck > 60000) {  // Every minute
    printMemoryStats();
    lastMemCheck = millis();
}
```

**Typical healthy output:**
```
[MEM] Free heap: 143256 bytes
[MEM] Largest block: 118784 bytes
[MEM] Min free (since boot): 138912 bytes
```

**Warning signs:**
- Free heap < 50KB: Risk of allocation failures
- Min free dropping over time: Memory leak

---

### Encoder Event Debugging

Track encoder rotation and button presses:

```cpp
// In encoder_pcnt.cpp
#define ENC_DEBUG 2

// Level 1 output:
[ENC] CCW rotation (count: -6)
[ENC] Button press

// Level 2 output:
[ENC] PCNT count: -12 (detents: -2)
[ENC] Direction locked: CCW (10ms)
[ENC] Button press (held: 1250ms) → LONG PRESS
```

---

### NTP Sync Verification

Monitor time synchronization:

```cpp
// In app_time.cpp
void appTimeSyncNtp() {
    time_t before = time(nullptr);

    bool success = ntpSync();

    if (success) {
        time_t after = time(nullptr);
        int32_t delta = (int32_t)(after - before);
        Serial.printf("[NTP] Synced (delta: %+ds)\n", delta);
    } else {
        Serial.println("[NTP] Sync failed");
    }
}

// Typical output:
// [NTP] Synced (delta: +2s)  ← Clock was 2 seconds slow
// [NTP] Synced (delta: 0s)   ← Clock was accurate
```

---

## 🔧 Building and Flashing

### Prerequisites

- **PlatformIO CLI or VS Code extension**
- **ESP32-S3 USB cable** (data-capable, not charge-only)
- **Python 3.7+** (for esptool.py)

### Build Commands

```bash
# Clean build
pio run -t clean
pio run

# Build specific environment
pio run -e esp32-s3

# Upload via USB
pio run -t upload

# Upload + Serial Monitor
pio run -t upload && pio device monitor

# Build only (no upload)
pio run -t build
```

### Partition Table

Located in `partitions.csv`:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x180000
app1,     app,  ota_1,   0x190000,0x180000
spiffs,   data, spiffs,  0x310000,0x0F0000
```

**Key points:**
- **app0/app1:** Each 1.5MB (dual OTA slots)
- **nvs:** 20KB (settings storage)
- **spiffs:** 960KB (reserved for future use - fonts, assets)

### Flash Size Optimization

**Current firmware size:** ~800KB–1.2MB (depending on debug symbols)

**Optimization flags:**

```ini
# In platformio.ini
build_flags =
    -Os                    # Optimize for size
    -DCORE_DEBUG_LEVEL=0   # Disable debug output
    -ffunction-sections    # Enable dead code elimination
    -fdata-sections
```

**Size breakdown:**
- **ESP-IDF framework:** ~400KB
- **GxEPD2 e-ink library:** ~150KB
- **Application code:** ~250KB
- **ArduinoJson, HTTPClient, etc.:** ~200KB

---

## 📊 Performance Profiling

### Update Cycle Timing

Typical update cycle (3-minute interval, partial refresh):

| Phase | Duration | Notes |
|-------|----------|-------|
| **Prefetch trigger** | ~10ms | Scheduler check |
| **API call (Binance)** | 400-800ms | Network latency |
| **JSON parsing** | 10-30ms | Price data extraction |
| **History update** | 50-150ms | Chart data append |
| **Display partial refresh** | 300-400ms | E-ink update |
| **LED animation** | Continuous | Independent task (optional) |
| **Total** | ~1.0-1.5s | User-perceived update time |

### Network Performance

**API latency benchmarks** (from Taiwan, typical residential internet):

| API | Average | P95 | P99 | Notes |
|-----|---------|-----|-----|-------|
| **Binance** | 350ms | 650ms | 1200ms | Fastest, most reliable |
| **Kraken** | 550ms | 950ms | 1800ms | Slightly slower |
| **CoinPaprika** | 680ms | 1100ms | 2400ms | Moderate speed |
| **CoinGecko** | 850ms | 1500ms | 3200ms | Slowest, but broad coverage |

**Note:** Latency varies significantly by region and ISP.

### Power Consumption

**Measured with USB power meter:**

| State | Current | Notes |
|-------|---------|-------|
| **Idle (WiFi connected)** | 80-120mA | Mostly WiFi radio |
| **E-ink partial refresh** | 150-200mA | 300-400ms burst |
| **E-ink full refresh** | 200-300mA | 2000ms burst |
| **WiFi connecting** | 200-400mA | 5-10 second burst |
| **Sleep (not implemented)** | N/A | Could achieve ~10mA with deep sleep |

**Daily energy** (3-min updates, partial refresh):
- **Active power:** ~100mA average
- **E-ink bursts:** 480 updates/day × 0.3s × 180mA ≈ 7mA average
- **Total:** ~110mA continuous draw at 5V = **0.55W**

---

## 🧪 Testing Strategies

### Manual Testing Checklist

**Before firmware release:**

- [ ] WiFi provisioning portal works (AP mode)
- [ ] All 20 coins display correctly
- [ ] All 9 currencies show proper decimals
- [ ] Menu navigation responsive
- [ ] Long-press exits menu
- [ ] OTA update via menu works
- [ ] OTA rollback protection triggers on bad firmware
- [ ] LED colors match price changes
- [ ] Party mode activates at +20% gain
- [ ] Timezone auto-detection (test with VPN)
- [ ] NTP sync updates time correctly
- [ ] WiFi auto-reconnect after router reboot

### Unit Testing (Future)

**Recommended test framework:** Unity (PlatformIO native)

**Test candidates:**
- `coins.cpp:findCoinBySymbol()` - Symbol lookup logic
- `app_scheduler.cpp:calcNextTick()` - Tick alignment math
- `price_fmt.cpp:formatPrice()` - Decimal formatting logic
- `utils.cpp:percentChange()` - Percentage calculation

**Example test:**
```cpp
void test_find_coin_btc() {
    const Coin* btc = findCoinBySymbol("BTC");
    TEST_ASSERT_NOT_NULL(btc);
    TEST_ASSERT_EQUAL_STRING("Bitcoin", btc->name);
}
```

### Integration Testing

**Simulated network failures:**

```cpp
// In network.cpp (test mode)
#ifdef TEST_MODE
bool fetchPrice(double* price, double* change24h) {
    static int callCount = 0;

    if (callCount++ < 3) {
        return false;  // Simulate failure
    }

    *price = 42000.0;
    *change24h = 2.5;
    return true;
}
#endif
```

**Test scenarios:**
- All APIs fail → LED yellow, display shows last price
- WiFi disconnects during fetch → auto-reconnect
- NTP sync fails → use RTC, retry later
- E-ink refresh timeout → retry with full refresh

---

## 📚 Code Organization

### Module Dependency Graph

```
main.cpp
  ├─ app_state.cpp (global state)
  ├─ app_wifi.cpp (WiFi management)
  │   └─ wifi_portal.cpp (setup portal)
  ├─ app_time.cpp (NTP, timezone)
  ├─ app_scheduler.cpp (update timing)
  │   └─ network.cpp (API calls)
  │       └─ coins.cpp (coin metadata)
  ├─ ui.cpp (e-ink rendering)
  │   ├─ price_fmt.cpp (formatting)
  │   └─ chart.cpp (24h chart)
  ├─ encoder_pcnt.cpp (user input)
  │   └─ menu.cpp (menu system)
  ├─ led.cpp (RGB LED control)
  ├─ ota_guard.cpp (rollback protection)
  └─ settings_store.cpp (NVS persistence)
```

### File Naming Conventions

- **`app_*.cpp`** - Application-level modules (WiFi, time, scheduler)
- **`ui*.cpp`** - User interface rendering
- **`*_guard.cpp`** - Safety/protection mechanisms
- **`*_store.cpp`** - Persistent storage wrappers

### Code Style

**Variable naming:**
- Global state: `g_variableName` (e.g., `g_currentPrice`)
- Constants: `SCREAMING_SNAKE_CASE` (e.g., `PARTIAL_REFRESH_LIMIT`)
- Local variables: `camelCase` (e.g., `nextUpdateTime`)

**Function naming:**
- Public API: `moduleName + Action` (e.g., `appSchedulerCalcNext()`)
- Private helpers: `camelCase` (e.g., `calculateJitter()`)

---

## 🔗 Related Documentation

### User Guides
- [Display Guide](DISPLAY_GUIDE.md) - Complete UI reference
- [OTA Update Guide](OTA_UPDATE_GUIDE.md) - Firmware update instructions
- [Hardware Assembly Guide](HARDWARE_GUIDE.md) - Building CryptoBar
- [LED Display Guide](LED_DISPLAY_GUIDE.md) - LED color meanings

### Developer References
- [src/README.md](../../src/README.md) - Source code module reference
- [include/README.md](../../include/README.md) - API header documentation
- [FEATURE_DOCUMENTATION_GAPS.md](../FEATURE_DOCUMENTATION_GAPS.md) - Documentation improvement tracking

### External Resources
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2) - E-ink display driver
- [PlatformIO Docs](https://docs.platformio.org/)
- [Binance API Documentation](https://binance-docs.github.io/apidocs/)

---

## 📝 Feedback

Have suggestions for improving this guide? Found a bug in the implementation?

- **Report issues:** [GitHub Issues](https://github.com/max05210238/CryptoBar/issues)
- **Documentation feedback:** Create an issue labeled `documentation`

---

**Last Updated:** 2025-12-26
**Firmware Version:** V0.99q+
