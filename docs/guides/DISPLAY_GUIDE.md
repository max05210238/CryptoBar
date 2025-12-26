# CryptoBar Display Guide

**Complete guide to understanding the CryptoBar user interface**

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

- **Visual Benchmark** - Shows average price from the previous trading day
- **Optional Feature** - Can be enabled/disabled in settings
- **Two Modes:**
  1. **Rolling 24h Average** - Continuously updated 24-hour rolling average
  2. **ET 7pm Cycle** - Resets at 7:00 PM Eastern Time (traditional trading day boundary)

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
┌─────────────────────────────┐
│ [1] Coin Selection          │ ← Choose cryptocurrency
│ [2] Display Currency        │ ← Choose display currency (USD, TWD, etc.)
│ [3] Update Preset           │ ← Price refresh interval (1/3/5/10 min)
│ [4] Refresh Mode            │ ← Partial or Full e-ink refresh
│ [5] Brightness              │ ← LED brightness (Low/Med/High)
│ [6] Date/Time Size          │ ← Small or Large date/time display
│ [7] Time Format             │ ← 24h or 12h
│ [8] Date Format             │ ← MM/DD, DD/MM, or YYYY-MM-DD
│ [9] Timezone                │ ← Select from 27 timezones
│ [10] Day Average Mode       │ ← Reference line (Off/Rolling/ET 7pm)
│ [11] WiFi Info             │ ← View WiFi connection information
│ [12] Firmware Update        │ ← OTA update mode
└─────────────────────────────┘
```

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
```
UTC-12 Baker Island
UTC-11 American Samoa
...
UTC+00 London
...
UTC+08 Taipei
...
UTC+14 Kiritimati
```

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

**Access:** Select "Firmware Update" from main menu.

```
┌─────────────────────────┐
│ OTA Update Mode         │
│                         │
│ Connect to:             │
│ http://192.168.x.x      │  ← Device IP address
│                         │
│ Upload .bin file        │
└─────────────────────────┘
```

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
