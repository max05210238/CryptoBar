# CryptoBar LED Display Guide

Quick reference for understanding LED colors and animations.

---

## 📖 Table of Contents

### LED Behavior
1. [🎨 LED Color Meanings](#led-color-meanings)
   - Price Movement Colors (Red/Green)
   - System Status Colors (Yellow/Blue/Purple)
2. [🫁 Breathing Animations](#breathing-animations)
3. [🎉 Party Mode](#party-mode)
4. [🔄 Color Priority](#color-priority)

### Examples & Control
5. [📊 Examples](#examples)
6. [🎚️ Brightness Control](#brightness-control)
7. [🔧 Technical Details](#technical-details)

### Reference
8. [🐛 Troubleshooting](#troubleshooting)
9. [📝 Configuration](#configuration)
10. [🌟 Best Practices](#best-practices)
11. [📚 Version History](#version-history)

---

## 🎨 LED Color Meanings

### Price Movement (Red/Green Reserved)

| Color | Meaning | Condition |
|-------|---------|-----------|
| 🟢 **Green** | Price rising | 24h gain > +0.02% |
| 🔴 **Red** | Price falling | 24h loss < -0.02% |
| ⚪ **Gray** | Price neutral | Between -0.005% and +0.005% |
| 🌈 **Rainbow** | Party Mode! | 24h gain ≥ +20% 🎉 |

### System Status (Avoid Red/Green)

| Color | Meaning | When It Appears |
|-------|---------|----------------|
| 🟡 **Yellow** | API failure | Can't fetch crypto prices |
| 🔵 **Blue** | Connecting | System startup, WiFi connecting |
| 🟣 **Purple** | Configuration | WiFi setup portal, firmware update |
| 🩵 **Cyan** | Undefined | Fallback/error state (rare) |

---

## 🫁 Breathing Animations

The LED "breathes" (pulses brighter/dimmer) based on price volatility:

| Animation | 24h Change | Period |
|-----------|------------|--------|
| 📊 **Static** | < 5% | No breathing, constant brightness |
| 🫁 **Slow Breathing** | 5% ~ 10% | 2.4 second cycle |
| 💨 **Fast Breathing** | ≥ 10% | 0.9 second cycle |

**Brightness range**: 15% ~ 100% (smooth sine wave)

---

## 🎉 Party Mode

**Triggers when**: 24-hour gain reaches +20%

**Effect**: Smooth rainbow gradient cycling

**Duration**: Until price drops below +15%

**Colors**: Red → Orange → Yellow → Green → Blue → Purple → Red...

**Speed**: 2.5 seconds per full cycle

**Breathing**: None (pure color rotation)

---

## 🔄 Color Priority

If multiple states apply, LED follows this priority:

```
1️⃣ Party Mode (+20%)           Highest priority
2️⃣ API Failure (Yellow)
3️⃣ System Config (Purple/Blue)
4️⃣ Price Movement (Red/Green/Gray)  Lowest priority
```

---

## 📊 Examples

### Normal Operation

| 24h Change | Color | Animation |
|------------|-------|-----------|
| +25% | 🌈 Rainbow | Party mode cycling |
| +12% | 🟢 Green | Fast breathing (0.9s) |
| +7% | 🟢 Green | Slow breathing (2.4s) |
| +2% | 🟢 Green | Static (no breathing) |
| +0.01% | ⚪ Gray | Static |
| -0.01% | ⚪ Gray | Static |
| -3% | 🔴 Red | Static |
| -8% | 🔴 Red | Slow breathing (2.4s) |
| -15% | 🔴 Red | Fast breathing (0.9s) |

### System States

| Situation | Color | Note |
|-----------|-------|------|
| Power on | 🔵 Blue | Brief, during startup |
| WiFi connecting | 🔵 Blue | Until connected |
| WiFi setup portal | 🟣 Purple | Press AP button or first boot |
| Firmware update mode | 🟣 Purple | Long press encoder (12s) |
| API timeout | 🟡 Yellow | Check internet connection |
| All APIs failed | 🟡 Yellow | Check time, retry in 30-60s |

---

## 🎚️ Brightness Control

LED brightness can be adjusted in settings menu:

- **Low**: 20% (0.2)
- **Medium**: 50% (0.5) - Default
- **High**: 100% (1.0)

**Note**: This is the *maximum* brightness. Breathing animations vary within this limit.

---

## 🔧 Technical Details

### Hysteresis (Anti-Flicker)

To prevent LED flickering near boundaries:

- **Enter green/red**: Must exceed ±0.02%
- **Exit to gray**: Must return to ±0.005%

**Example**:
- At +0.01% (gray) → rotate to +0.03% → turns green ✅
- At +0.03% (green) → rotate to +0.01% → stays green ⚠️
- At +0.03% (green) → rotate to +0.004% → turns gray ✅

### Party Mode Hysteresis

- **Activate**: Gain reaches +20%
- **Deactivate**: Drops below +15%

**Example**:
- At +19.5% (green fast breathing) → +20.1% → 🌈 party mode ✅
- At +20.5% (party mode) → +19% → still party mode ⚠️
- At +20.5% (party mode) → +14.8% → green breathing ✅

---

## 🐛 Troubleshooting

### LED stays yellow
- **Cause**: API failures (can't fetch price)
- **Check**: WiFi connection, internet access
- **Fix**: Wait 30-60s for automatic retry, or check network settings

### LED stays blue
- **Cause**: Can't connect to WiFi
- **Check**: SSID/password correct, router nearby
- **Fix**: Long press encoder (12s) → factory reset → reconfigure WiFi

### LED stays purple
- **Cause**: In configuration mode
- **Fix**: If unintended, reboot device

### No LED at all
- **Cause**: Brightness set to 0, or LED hardware issue
- **Check**: Settings → LED Brightness
- **Fix**: Increase brightness in menu

### Wrong colors
- **Cause**: Incorrect firmware or configuration
- **Fix**: Update to V0.99h or later

---

## 📝 Configuration

### LED Settings Location

**In Device Menu**:
1. Long press encoder
2. Navigate to "Settings"
3. Select "LED Brightness"
4. Rotate to choose: Low / Med / High
5. Short press to confirm

**In Code** (`src/app_state.cpp`):
```cpp
float g_ledBrightness = 0.5f;  // 0.2, 0.5, or 1.0
```

### Customizing Thresholds

**Party Mode** (`src/led_status.cpp`):
```cpp
LED_PARTY_ENTER_THRESH = 20.0f;  // +20% (default)
LED_PARTY_EXIT_THRESH = 15.0f;   // +15% (default)
```

**Breathing Sensitivity**:
```cpp
LED_EVENT_SLOW_THRESH = 5.0f;   // 5% for slow breathing
LED_EVENT_FAST_THRESH = 10.0f;  // 10% for fast breathing
```

**Color Entry/Exit**:
```cpp
const float ENTER = 0.02f;  // ±0.02% to enter red/green
const float EXIT  = 0.005f; // ±0.005% to return to gray
```

---

## 🌟 Best Practices

### For Daily Use
- ✅ Keep brightness at Medium (default)
- ✅ Check LED during price updates (every 30-300s based on settings)
- ✅ Yellow LED = check network connection

### For Presentations
- ✅ Set brightness to High for visibility
- ✅ Demo party mode with high-volatility coins
- ✅ Use breathing as visual feedback for market activity

### For Night Use
- ✅ Set brightness to Low
- ✅ Consider disabling LED in settings (brightness = 0)
- ✅ LED won't disturb sleep

---

## 📚 Version History

### V0.99h (Current)
- ✅ Added party mode (+20% rainbow)
- ✅ Changed API failure from purple → yellow
- ✅ Added cyan for undefined states
- ✅ Optimized breathing animations

### V0.99a-g
- Price trend colors (red/green/gray)
- Breathing animations (slow/fast)
- System status colors (blue/purple)

### V0.97-V0.98
- Basic LED support
- Single-color indicators

---

**Last Updated**: 2025-12-21 (V0.99h)
**See Also**: V0.99h_LED_OPTIMIZATION.md for technical details
