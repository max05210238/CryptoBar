# CryptoBar Display Guide

**Complete guide to understanding the CryptoBar user interface**

---

## 📖 Table of Contents

### Core Display Elements
1. [📺 Main Display Overview](#main-display-overview)
2. [🖤 Left Panel: Symbol Information](#left-panel-symbol-information-black-background)
3. [🤍 Right Panel: Live Data Display](#right-panel-live-data-display-white-background)
4. [🔄 Why Two API Labels?](#why-two-api-labels)

### Data Visualization
5. [📊 24-Hour Price Chart](#24-hour-price-chart)
6. [━ ━ ━ Reference Line (Day Average)](#------reference-line-dashed)
7. [⏰ Date & Time Display](#date--time-display)
8. [💰 Price Display](#price-display)
9. [💱 Multi-Currency Display](#multi-currency-display)

### Navigation & Control
10. [🔄 E-ink Refresh Mode](#e-ink-refresh-mode)
11. [🎛️ Menu Navigation](#menu-navigation)
    - Main Menu Items (All 12 Options)
    - Coin Selection (20+ Cryptocurrencies)
    - Timezone Selection (27 Timezones)

### System Screens
12. [🎨 Screen States](#screen-states)
    - Boot Sequence
    - WiFi Configuration Portal
    - OTA Firmware Update Mode

### Reference
13. [🔧 Display Settings Summary](#display-settings-summary)
14. [💡 Pro Tips](#pro-tips)
15. [🚨 Troubleshooting Display Issues](#troubleshooting-display-issues)
16. [📚 Related Guides](#related-guides)

---

## 📺 Main Display Overview

CryptoBar's main screen is divided into two sections: a **black symbol panel** (left) and a **white data panel** (right).

### Display Layout

```
┌──────────────────┬────────────────────────────────────────┐
│  [CoinGecko]     │  12/26/2025              15:30         │
│                  │                                         │
│      USD         │                                         │
│                  │          $42,350.25                     │
│      BTC         │                                         │
│                  │              ┌──────────────────────┐   │
│     +2.3%        │          ╱───╲                      │   │
│                  │       ╱──      ──╲                  │   │
│  [CoinGecko]     │    ╱──            ──╲─ ─ ─ ─ ─ ─ ─ │   │
│                  │                                         │
└──────────────────┴────────────────────────────────────────┘
 BLACK PANEL (64px)     WHITE PANEL (232px)
```

---

## 🖤 Left Panel: Symbol Information (Black Background)

The left panel displays static identification and data source information:

### Elements (Top to Bottom)

| Element | Description | Example |
|---------|-------------|---------|
| **Price API Label** | Current price data source | `CoinGecko` |
| **Display Currency** | Selected currency for price display | `USD`, `TWD`, `EUR` |
| **Coin Symbol** | Cryptocurrency ticker | `BTC`, `ETH`, `SOL` |
| **24h Change %** | Price change percentage (last 24 hours) | `+2.3%`, `-5.1%` |
| **History API Label** | Historical chart data source | `CoinGecko`, `Binance` |

### Color Coding

- **White text on black background** - High contrast for quick symbol identification
- **Change percentage color** - Always white (LED indicator shows price direction)

---

## 🤍 Right Panel: Live Data Display (White Background)

The right panel shows real-time cryptocurrency data:

### Elements (Top to Bottom)

| Element | Description | Format |
|---------|-------------|--------|
| **Date & Time** | Current date and time in selected timezone | `12/26/2025  15:30` |
| **Current Price** | Real-time cryptocurrency price | `$42,350.25` |
| **24-Hour Chart** | Price trend visualization (last 24 hours) | Line graph |
| **Reference Line** | Previous day average price (optional) | Dashed horizontal line |

---

## 🔄 Why Two API Labels?

You may notice **two API source labels** displayed on the left panel:

### Dual API Display Explained

CryptoBar uses **separate APIs for different data types**:

1. **Top Label (Price API)** - Source for current price data
   - Updates every 1-10 minutes (based on your settings)
   - Priority order: CoinGecko → CoinPaprika → Kraken → Binance

2. **Bottom Label (History API)** - Source for 24-hour chart data
   - Updates with price data
   - Priority order: CoinGecko → Binance → Kraken

### Example Scenarios

**Normal Operation:**
```
[CoinGecko]    ← Price from CoinGecko
    BTC
   +2.3%
[CoinGecko]    ← Chart from CoinGecko
```

**API Fallback Active:**
```
[CoinPaprika]  ← CoinGecko price failed, using CoinPaprika
    BTC
   +2.3%
[CoinGecko]    ← CoinGecko chart still working
```

**Why This Matters:**
- ✅ **Transparency** - You always know your data source
- ✅ **Reliability** - 4-layer fallback ensures continuous operation
- ✅ **Debugging** - Identify API issues at a glance

---

### Detailed Fallback Chain

#### Price API (4-Layer Fallback)

```
Layer 1: Binance → Layer 2: Kraken → Layer 3: Paprika → Layer 4: CoinGecko
```

**Each layer tries for 10 seconds before moving to next:**

| Layer | API | Best For | Timeout Action |
|-------|-----|----------|----------------|
| **1** | **Binance** | Real-time prices for major coins | → Try Kraken |
| **2** | **Kraken** | Coins with krakenPair configured | → Try Paprika |
| **3** | **Paprika** | General use (30s refresh) | → Try CoinGecko |
| **4** | **CoinGecko** | Backup for all coins | → Show "----" |

**Fallback Triggers:**
- ❌ HTTP error (4xx/5xx)
- ❌ Timeout (10 seconds)
- ❌ JSON parse failure
- ❌ Invalid/missing data
- ❌ Network disconnected

---

#### History API (3-Layer Fallback)

```
Layer 1: Kraken OHLC → Layer 2: Binance Klines → Layer 3: CoinGecko
```

| Layer | API | Data Points | Resolution |
|-------|-----|-------------|------------|
| **1** | **Kraken OHLC** | 288 candles | 5-min intervals |
| **2** | **Binance Klines** | 288 candles | 5-min intervals |
| **3** | **CoinGecko** | 24 samples | Hourly (lower resolution) |

**If all fail:** Chart shows "Collecting data..."

---

### API Selection Examples

**Scenario 1: Optimal (BTC with krakenPair)**
```
[Binance]  ← Fastest real-time price
   BTC
  +2.3%
[Kraken]   ← High-quality OHLC chart data
```

**Scenario 2: Altcoin (no Binance/Kraken support)**
```
[Paprika]  ← Skipped layers 1-2, using CoinPaprika
   DOGE
  +5.1%
[CoinGecko] ← No Kraken/Binance klines, using CoinGecko
```

**Scenario 3: Network Issues**
```
[CoinGecko] ← All faster APIs failed
   XRP
  +1.2%
[CoinGecko] ← Last resort for chart data
```

**Scenario 4: Complete Failure**
```
[--Error--]
   BTC
   ----     ← No price available
[--Error--]

Chart: "Collecting data..."
LED: Yellow (API error)
```

---

### Understanding API Labels

**What the labels tell you:**

| Display | Meaning | Action Needed? |
|---------|---------|---------------|
| `[Binance]` / `[Kraken]` | Optimal performance | ✅ None |
| `[Paprika]` | Normal fallback for altcoins | ✅ None |
| `[CoinGecko]` | Backup mode (rate limit or network issue) | ⚠️ Check WiFi if persists |
| Different labels (top/bottom) | Independent fallback (normal) | ✅ None |
| `[--Error--]` / `----` | All APIs failed | ❌ Check WiFi connection |

---

## 📊 24-Hour Price Chart

The chart displays cryptocurrency price movement over the last 24 hours.

### Chart Features

#### Time Range
- **X-axis:** Last 24 hours (right edge = current time, left edge = 24h ago)
- **Y-axis:** Price range (auto-scaled to fit min/max values)

#### Data Points
- **Update Frequency:** New data point added every price refresh (1-10 min intervals)
- **Maximum Points:** 96 samples (enough for 24h at 15-min intervals)
- **Minimum Required:** 4 data points to display chart (shows "Collecting data..." until reached)

#### Visual Appearance
- **Line Style:** Continuous solid line connecting price points
- **Scale:** Always displays in **USD** (even if display currency is TWD/EUR/etc.)
- **Auto-Scaling:** Chart automatically adjusts to show full price range

### Example Chart

```
Price High ──┐
             │    ╱─╲
             │   ╱   ╲
             │  ╱     ╲ ─ ─ ─ ─ ─ ← Reference line
             │ ╱          ╲
Price Low  ──┴─────────────────────
           24h ago        Now
```

---

## ━ ━ ━ Reference Line (Dashed)

The **dashed horizontal line** represents the **Previous Day Average Price**.

### What Is It?

- **Visual Benchmark** - Shows average price reference for context
- **Optional Feature** - Can be enabled/disabled in Menu [10] "Day Average Mode"
- **Three Modes:**
  1. **Off** - No reference line displayed
  2. **Rolling 24h Mean** - Continuously updated 24-hour rolling average (default)
  3. **ET 7pm Cycle Mean** - Resets at 7:00 PM Eastern Time (traditional trading day boundary)

### How It Works

#### Mode 1: Rolling 24-Hour Mean (Default)

**Technical Details:**
- Uses **288 five-minute buckets** (24 hours × 60 minutes ÷ 5 min)
- Each bucket stores the average price for that 5-minute window
- Calculates mean across all valid buckets from the last 24 hours
- **Continuously updating** - oldest bucket is replaced every 5 minutes

**Example:**
```
Current time: 2:30 PM
Reference line = Average of all prices from 2:30 PM yesterday → 2:30 PM today
```

**When to use:**
- ✅ **Recommended for most users** - provides smooth, continuous reference
- ✅ Day trading - see if price is above/below recent average
- ✅ Longer update intervals (5-10 min) - still meaningful reference

#### Mode 2: ET 7pm Cycle Mean

**Technical Details:**
- Resets at **7:00 PM Eastern Time** daily
- Calculates mean of all chart samples since last 7pm ET
- Synchronized with the 24-hour chart cycle

**Why 7pm ET?**
- Traditional crypto trading day boundary
- Post-US market close (4pm ET)
- Aligns with many exchange daily settlement times
- Provides consistent daily baseline

**Example:**
```
Current time: 2:30 PM ET
Last reset: Yesterday 7:00 PM ET
Reference line = Average from 7pm yesterday → 2:30pm today (19.5 hours of data)
```

**When to use:**
- ✅ Tracking daily performance vs. cycle start
- ✅ Comparing to exchange "daily" metrics
- ✅ Prefer fixed reset time over rolling window

#### Mode 0: Off

**When to use:**
- ✅ Prefer clean chart without reference
- ✅ Focus only on price trend, not average comparison

### How to Read It

| Chart Pattern | Meaning |
|---------------|---------|
| Price **above** dashed line | Current price higher than yesterday's average (bullish) |
| Price **below** dashed line | Current price lower than yesterday's average (bearish) |
| Price **crossing** line | Price transitioning between above/below average |

### Example Usage

```
Current: $42,350
Yesterday Avg: $41,000 (dashed line position)

    $43k ──┐
           │   ╱──╲  ← Current price ($42,350)
    $42k ──│  ╱    ╲
           │ ╱      ─ ─ ─ ─ ─ ← Reference ($41,000)
    $41k ──┼ ─ ─
           │
    $40k ──┴──────────────
```
**Interpretation:** Price is currently **+3.3% above yesterday's average** - positive momentum

---

## ⏰ Date & Time Display

CryptoBar shows local date and time based on your timezone settings.

### Display Modes

#### Small Mode (Default)
```
┌────────────────────────────────────┐
│ 12/26/2025              15:30      │  ← Top of white panel
│                                     │
│          $42,350.25                 │
```
- **Font:** 6x8 pixel default font
- **Position:** Date left-aligned, time right-aligned

#### Large Mode
```
┌────────────────────────────────────┐
│      12/26/2025 15:30 PM           │  ← Centered, larger font
│                                     │
│          $42,350.25                 │
```
- **Font:** FreeSansBold 9pt
- **Position:** Centered (date and time together)

### Format Options

#### Date Formats
- **MM/DD/YYYY** - `12/26/2025` (US format)
- **DD/MM/YYYY** - `26/12/2025` (European format)
- **YYYY-MM-DD** - `2025-12-26` (ISO format)

#### Time Formats
- **24-Hour** - `15:30`
- **12-Hour** - `3:30 PM`

### Independent Time Refresh

- **Clock updates every minute** regardless of price update interval
- **Partial refresh only** - Time area updates without full screen refresh
- **Battery-friendly** - Time refreshes don't count toward e-ink wear limit
- **Use as desk clock** - Set price updates to 10 minutes, clock still updates every minute

---

### Time Accuracy & NTP Sync

CryptoBar maintains highly accurate time through automatic NTP synchronization.

#### How Time Synchronization Works

```
┌──────────────────────────────────────────────────┐
│ 1. WiFi Connects                                 │
│    → Immediate NTP sync                          │
└──────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────┐
│ 2. Every 10 Minutes                              │
│    → Automatic background NTP resync             │
│    → No display interruption                     │
└──────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────┐
│ 3. Time Stays Accurate                           │
│    → ESP32 RTC drift corrected continuously      │
│    → Error kept under 0.1 seconds               │
└──────────────────────────────────────────────────┘
```

#### NTP Sync Details

| Parameter | Value | Why? |
|-----------|-------|------|
| **Sync Interval** | 10 minutes | ESP32 RTC drifts ~1-5 sec/day, this ensures < 0.1s error |
| **Initial Sync** | On WiFi connect | Get accurate time immediately |
| **Primary Server** | `pool.ntp.org` | Global NTP pool, low latency |
| **Backup Server** | `time.nist.gov` | US government atomic clock |
| **Timeout** | 10 seconds | Retry if no response |

**Time Accuracy Guarantee:**
- ✅ **< 0.1 second error** under normal conditions
- ✅ **< 1 second error** even if NTP unreachable for 24 hours (RTC drift)

#### What You'll See

**During Normal Operation:**
- Time updates every minute (independent refresh)
- NTP sync happens silently in background (every 10 min)
- No user indication (transparent operation)

**If NTP Fails:**
- Time continues using ESP32 internal RTC
- Accuracy degrades slowly (~1-5 sec/day drift)
- Next successful sync corrects any drift

**After Power Loss:**
- ESP32 RTC resets to default (incorrect time)
- On WiFi reconnect → immediate NTP sync
- Time becomes accurate within ~1 second

#### Troubleshooting Time Issues

| Issue | Likely Cause | Solution |
|-------|--------------|----------|
| **Time off by hours** | Wrong timezone selected | Change timezone in Menu [9] |
| **Time off by minutes** | NTP sync failed, using RTC drift | Check WiFi connection, wait for next sync |
| **Time resets after power loss** | Normal ESP32 behavior | Time will correct on next WiFi connect |
| **Date shows year 1970** | No NTP sync yet since boot | Ensure WiFi connected, wait 10 seconds |

**💡 Pro Tip:** If you need exact time synchronization (within milliseconds), wait at least 1 minute after WiFi connects to ensure NTP has completed successfully.

---

## 💰 Price Display

The center of the white panel shows the current cryptocurrency price in large font.

### Display Features

#### Auto-Scaling Font
- **18pt font** - Standard display (price fits comfortably)
- **12pt font** - Automatically downgrades if price is too long
- **9pt font** - Used if even 12pt is too wide

#### Auto-Adjusting Decimals

The display intelligently adjusts decimal places based on total digit count:

| Total Length | Decimal Places | Example |
|--------------|----------------|---------|
| Short (≤6 digits) | 4 decimals | `$0.1234` |
| Medium (7-8 digits) | 2 decimals | `$1,234.56` |
| Long (≥9 digits) | 0 decimals | `$123,456` |

**Why?** Prevents price overflow on small e-ink display while maintaining maximum precision.

#### Currency Support

All 9 supported currencies use the same auto-decimal logic:

| Currency | Symbol | Example Display |
|----------|--------|-----------------|
| USD | $ | `$42,350.25` |
| TWD | NT$ | `NT$1,350,000` |
| EUR | € | `€39,850.50` |
| GBP | £ | `£33,250.75` |
| CAD | C$ | `C$58,900.00` |
| JPY | ¥ | `¥6,350,000` |
| KRW | ₩ | `₩56,800,000` |
| SGD | S$ | `S$57,100.00` |
| AUD | A$ | `A$63,500.00` |

---

## 💱 Multi-Currency Display

CryptoBar supports **9 global currencies** for price display. All cryptocurrencies can be displayed in any currency.

### Supported Currencies

| Code | Name | Symbol | Decimal Logic | Example |
|------|------|--------|---------------|---------|
| **USD** | US Dollar | $ | Length-based (0-4) | $42,350.2588 |
| **TWD** | Taiwan Dollar | NT | Length-based (0-4) | NT1,350,245 |
| **EUR** | Euro | EUR | Length-based (0-4) | EUR39,850.50 |
| **GBP** | British Pound | GBP | Length-based (0-4) | GBP33,250.75 |
| **CAD** | Canadian Dollar | C$ | Length-based (0-4) | C$58,900.00 |
| **JPY** | Japanese Yen | JPY | Length-based (0-4) | JPY6,350,125 |
| **KRW** | Korean Won | KRW | Length-based (0-4) | KRW56,812,500 |
| **SGD** | Singapore Dollar | S$ | Length-based (0-4) | S$57,100.25 |
| **AUD** | Australian Dollar | A$ | Length-based (0-4) | A$63,500.00 |

### Length-Based Decimal Display (V0.99p)

**All currencies use unified 10-character length limit:**

| Total Display Length | Decimals Shown | Example (BTC in USD) |
|---------------------|----------------|----------------------|
| ≤ 10 characters | **4 decimals** | `$1,234.5678` (10 chars) |
| 11-12 characters | **2 decimals** | `$123,456.78` (11 chars) |
| ≥ 13 characters | **0 decimals** | `$12,345,678` (12 chars) |

**Important:** V0.99p removed the old restriction that forced JPY/KRW to always show integers. Now **all currencies** (including JPY/KRW) can show decimals when appropriate based on length.

**Examples with different currencies:**

```
BTC in USD:  $87,619.3624  (10 chars, 4 decimals)
BTC in JPY:  JPY13,422,202 (13 chars, 0 decimals)
ETH in KRW:  KRW5,248.3621 (12 chars, 4 decimals) ← KRW can have decimals!
XRP in EUR:  EUR1.8635     (9 chars, 4 decimals)
```

### Exchange Rate Updates

**How it works:**
- Exchange rates fetched from **ExchangeRate-API.com**
- Updates **synchronized with price updates** (every 1-10 minutes based on your setting)
- Cryptocurrency price fetched in USD, then converted to display currency
- Formula: `Display Price = USD Price × Exchange Rate`

**Example:**
```
BTC Price: $87,619.36 USD
USD→TWD Rate: 32.15
Display: NT 2,816,962 (= 87,619.36 × 32.15)
```

**Fallback behavior:**
- If exchange rate fetch fails, display shows `----` until successful fetch
- Crypto price and FX rate must both be valid for display

### How to Change Display Currency

1. **Enter Menu** - Short press encoder from main screen
2. **Navigate to [2] "Display Currency"**
3. **Short press to enter** currency selection submenu
4. **Scroll through 9 currencies** using encoder
5. **Short press to select** - Returns to main screen with new currency

**Saved to NVS:** Your currency preference is saved and persists across reboots.

### Multi-Currency Tips

**💡 Best Practices:**

1. **Local Currency Preference**
   - Use your local currency for easier mental conversion
   - Taiwan users: TWD shows as "NT" (compressed for space)

2. **Decimal Precision Varies**
   - Higher-value currencies (USD, EUR, GBP) show more decimals
   - Lower-value currencies (JPY, KRW) often show fewer decimals (due to larger numbers)

3. **Exchange Rate Freshness**
   - FX rates update with crypto prices (1-10 min intervals)
   - More frequent than traditional FX sources (daily updates)
   - Good for approximate conversions, not for arbitrage trading

4. **Symbol Rendering**
   - Single-character symbols ($, €, £, ¥, ₩): Full-size font
   - Two-character symbols (NT, C$, S$, A$, EUR, GBP, JPY, KRW): Compressed font

**⚠️ Important Notes:**

- **Exchange rates are approximate** - sourced from aggregated market data
- **Not for financial decisions** - use official exchange sources for trading
- **Network required** - both crypto price AND FX rate need internet connection

---

## 🔄 E-ink Refresh Mode

CryptoBar's e-ink display offers two refresh modes with different trade-offs between speed and visual quality.

### Refresh Mode Options

**Configured in Menu [4] "Refresh Mode"**

| Mode | Speed | Image Quality | Ghosting | Power | Best For |
|------|-------|---------------|----------|-------|----------|
| **Partial** | ⚡ Fast (~300ms) | Good | Minor ghosting after many updates | Lower | Frequent updates (1-3 min) |
| **Full** | 🐢 Slow (~2s) | ✨ Perfect | No ghosting | Higher | Clean display priority |

---

### Partial Refresh Mode

**How it works:**
- Updates only changed pixels (fast, low power)
- Skips the full screen clearing cycle
- Ideal for frequent price updates

**Automatic Full Refresh Safety:**
- **After 20 partial refreshes** → automatic full refresh
- Prevents ghosting buildup
- Counter resets after each full refresh
- Transparent to user (happens automatically)

**Visual characteristics:**
```
Update 1-19:  Fast updates, minor ghosting may accumulate
Update 20:    Automatic full refresh (clears ghosting)
Update 21-40: Cycle repeats...
```

**⚡ When to use Partial mode:**
- Update interval ≤ 3 minutes
- You prioritize responsiveness over perfect clarity
- Battery conservation is important
- You don't mind occasional brief full refreshes

---

### Full Refresh Mode (Default)

**How it works:**
- Clears entire screen to white, then redraws content
- Takes ~2 seconds per update (visible flicker)
- Eliminates all ghosting artifacts

**Visual characteristics:**
```
Every update: White flash → Black content → Perfect clarity
```

**✨ When to use Full mode:**
- You want pristine display quality
- Update interval ≥ 5 minutes (flicker less noticeable)
- Display is visible to others (professional appearance)
- Power consumption is not a concern

---

### Comparison Examples

**Scenario 1: Day trader (1-min updates)**
```
Recommended: Partial mode
- 60 updates/hour with Full mode = 120 seconds of flicker/hour
- Partial mode: smooth updates, auto-full every 20× keeps display clean
```

**Scenario 2: Casual monitoring (5-min updates)**
```
Recommended: Full mode
- Only 12 updates/hour = 24 seconds of flicker/hour
- Always perfect clarity, no ghosting
```

**Scenario 3: Office display (3-min updates)**
```
Either mode works:
- Full: Professional appearance, occasional flicker acceptable
- Partial: Faster response, automatic cleanup every ~60 minutes
```

---

### Technical Details

**Partial Refresh:**
- Update time: ~300ms
- Power draw: ~15mA during update
- Automatic full refresh trigger: 20 partial updates (configurable in code: `PARTIAL_REFRESH_LIMIT`)

**Full Refresh:**
- Update time: ~2000ms
- Power draw: ~30mA during update
- Phases: Clear (white) → Draw (black) → Stabilize

**Ghosting explanation:**
E-ink pixels don't always return to perfect white/black without a full clear cycle. Partial refresh leaves microscopic charge residue that accumulates over many updates. The 20× auto-full rule prevents this from becoming visible.

---

### Configuration

**To change refresh mode:**

1. Long-press encoder to enter menu
2. Rotate to menu item [4] "Refresh Mode"
3. Short-press to toggle: `Partial` ↔ `Full`
4. Long-press to save and exit

**Default:** Full (prioritizes visual quality)

**⭐ Recommended:**
- Use **Full** if unsure
- Switch to **Partial** only if you need faster updates and don't mind occasional flicker

---

## 🎛️ Menu Navigation

CryptoBar uses a **rotary encoder** (knob) for all navigation.

### Controls

| Action | Control | Result |
|--------|---------|--------|
| **Rotate Clockwise** | Turn knob right → | Move down menu / Increase value |
| **Rotate Counter-Clockwise** | Turn knob left ← | Move up menu / Decrease value |
| **Press (Short)** | Push knob down (< 0.5s) | Select menu item / Confirm |
| **Press (Long)** | Hold knob down (≥ 0.5s) | Exit menu / Go back |

### Main Menu Items

**Access:** Short press from main screen to enter menu.

```
┌─────────────────────────────────────────────────────────────┐
│ [1]  Coin Selection      │ Choose cryptocurrency            │
│                          │ → Default: BTC                   │
│ [2]  Display Currency    │ Choose display currency          │
│                          │ → Default: USD                   │
│                          │ → Options: USD/TWD/EUR/GBP/CAD/  │
│                          │            JPY/KRW/SGD/AUD       │
│ [3]  Update Preset       │ Price refresh interval           │
│                          │ → Default: 3 min ⭐ Recommended  │
│                          │ → Options: 1 min / 3 min /       │
│                          │            5 min / 10 min        │
│ [4]  Refresh Mode        │ Partial or Full e-ink refresh    │
│                          │ → Default: Full                  │
│                          │ → Partial: Auto full every 20×   │
│ [5]  Brightness          │ LED brightness                   │
│                          │ → Default: Medium                │
│                          │ → Options: Low / Med / High      │
│ [6]  Date/Time Size      │ Small or Large date/time display │
│                          │ → Default: Large                 │
│ [7]  Time Format         │ 24-hour or 12-hour clock         │
│                          │ → Default: 12h                   │
│ [8]  Date Format         │ Date display format              │
│                          │ → Default: MM/DD                 │
│                          │ → Options: MM/DD / DD/MM /       │
│                          │            YYYY-MM-DD            │
│ [9]  Timezone            │ Select from 27 timezones         │
│                          │ → Default: UTC-08 Seattle        │
│                          │ → Auto-detect on first boot      │
│ [10] Day Average Mode    │ Reference line on chart          │
│                          │ → Default: Rolling 24h Mean      │
│                          │ → Options: Off / Rolling /       │
│                          │            ET 7pm Cycle          │
│ [11] WiFi Info          │ View WiFi connection details     │
│ [12] Firmware Update     │ Enter OTA update mode            │
└─────────────────────────────────────────────────────────────┘
```

**⭐ Recommended Settings:**
- **Update Interval:** 3 minutes (balances API limits and timeliness)
- **Refresh Mode:** Full (clean display, no ghosting)
- **Day Average:** Rolling 24h Mean (smooth continuous reference)

### Navigation Tips

#### Scrolling Long Lists
- Menu auto-scrolls when you reach the bottom of the screen
- Top item indicator shows current position in list
- Long press to exit menu at any time

#### Coin Selection (20 Coins)

The Coin Selection menu uses a **submenu system**:

1. **Select "Coin Selection"** from main menu (short press)
2. **Submenu appears** with 20 cryptocurrency options
3. **Navigate** using rotary encoder (up/down)
4. **Confirm selection** with short press
5. **Returns to main screen** with new coin displayed

**Available cryptocurrencies (by market cap rank):**
1. BTC (Bitcoin)
2. ETH (Ethereum)
3. BNB (Binance Coin)
4. XRP (Ripple)
5. SOL (Solana)
6. TRX (Tron)
7. DOGE (Dogecoin)
8. ADA (Cardano)
9. BCH (Bitcoin Cash)
10. LINK (Chainlink)
11. XMR (Monero)
12. XLM (Stellar)
13. LTC (Litecoin)
14. AVAX (Avalanche)
15. HBAR (Hedera)
16. SHIB (Shiba Inu)
17. TON (Toncoin)
18. UNI (Uniswap)
19. DOT (Polkadot)
20. KAS (Kaspa)

**Note:** List ordered by market cap ranking (as of device firmware). Stablecoins (USDT/USDC) intentionally excluded.

#### Timezone Selection (27 Timezones)

**Available timezones:**
```
UTC-12 Baker Island
UTC-11 American Samoa
UTC-10 Honolulu
UTC-09 Anchorage
UTC-08 Seattle (Default)
UTC-07 Denver
UTC-06 Chicago
UTC-05 New York
UTC-04 Halifax
UTC-03 São Paulo
UTC-02 South Georgia
UTC-01 Azores
UTC+00 London
UTC+01 Paris
UTC+02 Cairo
UTC+03 Moscow
UTC+04 Dubai
UTC+05 Karachi
UTC+06 Dhaka
UTC+07 Bangkok
UTC+08 Taipei
UTC+09 Tokyo
UTC+10 Sydney
UTC+11 Noumea
UTC+12 Auckland
UTC+13 Samoa
UTC+14 Kiritimati
```

**Automatic Timezone Detection (First Boot):**

On first boot, CryptoBar attempts to automatically detect your timezone:

1. **Trigger Condition:**
   - No timezone saved in settings (first boot)
   - WiFi successfully connected
   - One-time attempt only

2. **Detection Method:**
   - Uses IP-based geolocation (worldtimeapi.org API)
   - Returns UTC offset for your current IP address
   - Matches to closest timezone in the 27-timezone list
   - Saves detected timezone to settings

3. **Fallback Behavior:**
   - **Auto-detection fails** → defaults to UTC-08 (Seattle)
   - **No network** → defaults to UTC-08 (Seattle)
   - You can always manually change timezone later via Menu [9]

**⚠️ Important Notes for VPN Users:**

If you're using a VPN during first boot:
- Auto-detection will use **VPN exit location** timezone (not your physical location)
- **Recommendation:** Disable VPN during initial setup, or manually set timezone in Menu [9]

**Manual Override:**

Even after auto-detection, you can change timezone anytime:
1. Long-press encoder → Menu
2. Navigate to [9] Timezone
3. Select your preferred timezone
4. Long-press to save

---

## 🎨 Screen States

CryptoBar displays different screens based on its current state:

### Boot Sequence

```
[1] Splash Screen
    ┌─────────────────────┐
    │                     │
    │    CryptoBar        │  ← Large title (Space Grotesk 24pt)
    │                     │
    │            V0.99q   │  ← Version (bottom-right)
    └─────────────────────┘
    Duration: 2 seconds

[2] WiFi Connection
    ┌─────────────────────┐
    │ WiFi Setup          │  ← If not configured
    │ Preparing AP...     │
    │ Connect to:         │
    │ CryptoBar-XXXX      │
    └─────────────────────┘
    OR
    ┌─────────────────────┐
    │ Connecting WiFi...  │  ← If already configured
    │ Network: MyWiFi     │
    └─────────────────────┘

[3] Time Sync
    ┌─────────────────────┐
    │ Syncing time...     │
    │ NTP: pool.ntp.org   │
    └─────────────────────┘

[4] First Data Fetch
    ┌─────────────────────┐
    │ Fetching prices...  │
    │ API: CoinGecko      │
    └─────────────────────┘

[5] Main Display (Ready)
```

### WiFi Configuration Portal

**When to access:** First boot or when WiFi credentials not saved.

**Device creates AP:** `CryptoBar-XXXX` (XXXX = last 4 digits of MAC address)

**Portal Features:**
- WiFi network selection and password entry
- Initial device configuration:
  - Coin selection
  - Display currency
  - Update interval
  - Refresh mode
  - LED brightness
  - Time/date formats
  - Timezone
  - Day average mode

**Access:** Connect to CryptoBar AP, browser automatically opens portal (or navigate to `192.168.4.1`)

### OTA Firmware Update Mode

**Access:** Select "Firmware Update" from main menu (Menu [12]).

**Overview:**
- Device creates dedicated WiFi hotspot: `CryptoBar_MAINT_XXXX`
- Connect to hotspot with phone/tablet
- Upload .bin firmware file via web browser
- Device automatically reboots with new firmware

**Display shows:**
```
┌─────────────────────────┐
│ Firmware Update         │
│                         │
│ 1) Connect phone to:    │
│    CryptoBar_MAINT_XXXX │
│                         │
│ 2) Open browser to:     │
│    http://192.168.4.1   │
│                         │
│ 3) Upload .bin file     │
└─────────────────────────┘
```

**📘 Complete Guide:** See [OTA Update Guide](OTA_UPDATE_GUIDE.md) for detailed step-by-step instructions, security considerations, and troubleshooting.

---

## 🔧 Display Settings Summary

Quick reference for all display-related settings:

| Setting | Options | Default | Location |
|---------|---------|---------|----------|
| **Date/Time Size** | Small, Large | Small | Menu [6] |
| **Time Format** | 24h, 12h | 24h | Menu [7] |
| **Date Format** | MM/DD, DD/MM, YYYY-MM-DD | MM/DD | Menu [8] |
| **Refresh Mode** | Partial, Full | Full | Menu [4] |
| **Update Interval** | 1min, 3min, 5min, 10min | 3min | Menu [3] |
| **Day Average Line** | Off, Rolling 24h, ET 7pm | Off | Menu [10] |
| **Display Currency** | 9 currencies | USD | Menu [2] |

---

## 💡 Pro Tips

### Display Optimization

1. **Partial vs Full Refresh:**
   - **Partial** - Fast updates (1-2 seconds), may accumulate ghosting over time
   - **Full** - Complete refresh (5-6 seconds), eliminates all ghosting, **recommended default**
   - **Default Setting:** Full refresh mode (prevents ghosting)
   - **Note:** Device automatically does full refresh every 20 partial updates if using Partial mode

2. **Desk Clock Mode:**
   - Set update interval to 5 or 10 minutes to reduce API calls
   - Time still updates every minute (independent refresh)
   - Use Large date/time mode for better visibility

3. **Battery/E-ink Longevity:**
   - Longer update intervals = less e-ink wear
   - Partial refresh = minimal wear vs. full refresh
   - Time-only refreshes don't count toward e-ink wear limit

4. **Reading the Chart:**
   - Chart always shows USD prices (even if display is in TWD/EUR)
   - Steep slopes = high volatility
   - Flat lines = stable price
   - Reference line helps identify trend direction

5. **API Source Transparency:**
   - Normally both labels show `CoinGecko` (best quality data)
   - If you see different APIs, system is using fallback (still reliable)
   - Common fallback order: CoinGecko → CoinPaprika → Binance → Kraken

---

## 📶 WiFi Connection Management

CryptoBar includes intelligent WiFi reconnection to handle network interruptions.

### Automatic Reconnection Strategy

When WiFi connection is lost during normal operation, CryptoBar automatically attempts to reconnect:

```
┌──────────────────────────────────────────────────┐
│ WiFi Connection Lost                             │
│ (Router reboot, signal loss, etc.)               │
└──────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────┐
│ Wait 5 Minutes (Cooldown Period)                 │
│ → Prevents aggressive battery drain              │
│ → Allows temporary issues to resolve             │
└──────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────┐
│ Reconnection Batch: 3 Attempts                   │
│ → Attempt 1: Try connect (30s timeout)           │
│ → Attempt 2: Try connect (30s timeout)           │
│ → Attempt 3: Try connect (30s timeout)           │
└──────────────────────────────────────────────────┘
         ↓
    ┌─── Success? ───┐
    │                 │
   YES               NO
    │                 │
    ↓                 ↓
  Resume         Wait 5 Minutes
  Operation      → Try Next Batch
```

### Reconnection Behavior

| Parameter | Value | Purpose |
|-----------|-------|---------|
| **Cooldown Period** | 5 minutes | Prevents battery drain from continuous retry |
| **Attempts per Batch** | 3 | Reasonable retry without being aggressive |
| **Timeout per Attempt** | 30 seconds | Long enough for slow routers |
| **Batch Interval** | 5 minutes | Exponential backoff strategy |

### What You'll See

**During WiFi Disconnection:**
- Display shows last known price (frozen)
- LED may turn yellow (API error after ~40s timeout)
- Time continues updating (ESP32 RTC)
- Device does NOT create WiFi portal (runtime mode)

**During Reconnection:**
- No visual indication (background process)
- On success → LED returns to price color
- On success → Price updates resume within 1-3 minutes

**If Reconnection Fails:**
- Device continues retrying every 5 minutes
- Display remains functional (shows last known data)
- Manual reboot may help if router changed settings

### Protection Against Battery Drain

**Why the 5-minute cooldown?**
- WiFi radio consumes significant power when scanning/connecting
- Continuous retry would drain battery in USB-powered setup
- Gives router time to fully reboot (~2-3 minutes typical)

**First-Boot Exception:**
- If device has **never connected** to WiFi since boot → **NO automatic reconnection**
- Rationale: Wrong password or out-of-range scenarios
- Solution: Device creates WiFi portal for reconfiguration

### Manual Intervention

**If automatic reconnection isn't working:**

1. **Check router status** - Ensure WiFi network is actually available
2. **Power cycle device** - Sometimes helps clear WiFi stack issues
3. **Reconfigure WiFi** - Long-press encoder → Menu [11] WiFi Info → Follow instructions

**WiFi credential change:**
- If router SSID/password changed → device won't reconnect automatically
- Must reconfigure via WiFi portal (see Menu [11])

### Common Scenarios

| Scenario | Device Behavior | Action Needed |
|----------|----------------|---------------|
| **Router reboots (2-3 min)** | Waits 5 min, reconnects on 1st attempt | ✅ None (automatic) |
| **Brief signal loss (< 1 min)** | Waits 5 min, reconnects immediately | ✅ None (automatic) |
| **Router SSID changed** | Retries forever, never succeeds | ⚠️ Reconfigure WiFi |
| **Router password changed** | Retries forever, never succeeds | ⚠️ Reconfigure WiFi |
| **Moved device out of range** | Retries every 5 min, no success | ⚠️ Move closer or reconfigure |
| **First boot, wrong password** | NO auto-retry, shows WiFi portal | ⚠️ Enter correct password |

**💡 Pro Tip:** If you see persistent yellow LED + frozen price for > 15 minutes, check your WiFi router status. Device is likely retrying in background.

---

## 🚨 Troubleshooting Display Issues

### Chart Shows "Collecting data..."

**Cause:** Less than 4 data points collected (device just booted or coin just changed).

**Solution:** Wait for 4 price updates. At 3-minute intervals, this takes ~12 minutes.

### Time Not Updating

**Cause:** Time refresh might be disabled or WiFi connection lost.

**Solution:**
1. Check WiFi connection (LED will show yellow if disconnected)
2. Verify timezone is set correctly (Menu [9])
3. Device auto-syncs time via NTP every 24 hours

### Price Shows Dashes `----`

**Cause:** No price data available (API failed or initial boot).

**Solution:**
1. Check WiFi connection
2. Wait for next price update
3. If persists, check API status (should auto-fallback)

### Ghosting on Display

**Cause:** Too many partial refreshes without full refresh.

**Solution:**
1. Switch to Full Refresh mode (Menu [4])
2. Or wait - device automatically does full refresh every 20 partial refreshes
3. Manual trigger: Change any setting to force full refresh

### API Labels Different

**Cause:** Primary API (CoinGecko) is experiencing issues, system using fallback.

**Solution:** This is **normal behavior** - CryptoBar's multi-layer fallback ensures continuous operation. Data quality remains high (all sources are aggregated or major exchanges).

---

## 📚 Related Guides

- **[LED Display Guide](LED_DISPLAY_GUIDE.md)** - LED color meanings and animations
- **[Hardware Guide](HARDWARE_GUIDE.md)** - Assembly and GPIO connections
- **[Changelog](../../CHANGELOG.md)** - Version history and updates

---

## 📝 Feedback

Found this guide helpful? Have suggestions for improvement?

- 🐛 **Report Issues:** [GitHub Issues](https://github.com/max05210238/CryptoBar/issues)
- 💡 **Suggest Features:** [GitHub Discussions](https://github.com/max05210238/CryptoBar/discussions)

---

**Last Updated:** V0.99q (2025-12-26)
