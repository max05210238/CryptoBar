# CryptoBar Settings Menu Guide

Quick reference for navigating the settings menu and submenus.

**Last Updated:** V0.99r (2025-12-28)

---

## 📱 Main Settings Menu

### Navigation
- **Rotate encoder**: Scroll through menu items
- **Short press**: Select/Enter item
- **Long press**: Exit menu

### Menu Items

| Item | Action | Type |
|------|--------|------|
| **Coin** | Enter submenu → Select cryptocurrency | Submenu |
| **Update** | Enter submenu → Select refresh interval | Submenu (V0.99r) |
| **Refresh** | Toggle: Partial ↔ Full | Direct toggle |
| **LED** | Cycle: Low → Med → High → Low | Direct cycle |
| **Time** | Toggle: 24h ↔ 12h | Direct toggle |
| **Date** | Cycle: MM/DD → DD/MM → YYYY-MM-DD | Direct cycle |
| **DT Size** | Toggle: Small ↔ Large | Direct toggle |
| **Currency** | Enter submenu → Select display currency | Submenu |
| **Time zone** | Enter submenu → Select timezone | Submenu |
| **Day avg** | Cycle: Off → 24h mean → Cycle mean | Direct cycle |
| **Firmware Update** | Enter confirmation screen | Action |
| **WiFi Info** | Show WiFi details | Info screen |
| **Exit** | Return to main display | Exit |

---

## 🔄 Update Interval Submenu (V0.99r)

**NEW in V0.99r:** Update interval now uses submenu pattern (like Coin/Currency selection)

### How to Change Update Interval

1. **Enter Menu**: Short press from main screen
2. **Navigate to "Update"**: Rotate encoder
3. **Enter Submenu**: Short press on "Update: Xm"
4. **Browse Options**: Rotate encoder
   - 1m (1 minute)
   - 3m (3 minutes) ⭐ Default, recommended
   - 5m (5 minutes)
   - 10m (10 minutes)
5. **Confirm Selection**: Short press
6. **Cancel**: Long press (returns without saving)

### Why This Changed (V0.99r)

**Before:**
- Direct toggle on "Update" item
- Each press immediately changed setting
- Triggered scheduler reset and data fetch
- Couldn't browse options without side effects

**After:**
- Enter submenu to browse all 4 options
- No data fetching while browsing ✅
- Only applies changes when confirmed
- Consistent with Coin/Currency UX

**Benefits:**
- ✅ Preview all options without triggering updates
- ✅ Reduces accidental API requests
- ✅ Prevents 10-minute setting restart bug (CRITICAL FIX)
- ✅ Consistent UX across all multi-option settings

---

## 💰 Coin Selection Submenu

### How to Change Coin

1. Enter menu → Navigate to "Coin"
2. Short press to enter submenu
3. Rotate to browse coins (#rank + ticker)
4. Short press to confirm
5. Long press to cancel

**Note:** Confirming coin selection will:
- Bootstrap 24h price history
- Fetch current price immediately
- Reset chart data

---

## 💱 Currency Selection Submenu

### How to Change Display Currency

1. Enter menu → Navigate to "Currency"
2. Short press to enter submenu
3. Rotate to browse currencies (USD, TWD, EUR, etc.)
4. Short press to confirm
5. Long press to cancel

**Note:** Switching to non-USD currency triggers immediate FX rate fetch.

**Supported Currencies:**
- USD (US Dollar)
- TWD (Taiwan Dollar)
- EUR (Euro)
- GBP (British Pound)
- CAD (Canadian Dollar)
- JPY (Japanese Yen)
- KRW (South Korean Won)
- SGD (Singapore Dollar)
- AUD (Australian Dollar)

---

## 🌍 Timezone Submenu

### How to Change Timezone

1. Enter menu → Navigate to "Time zone"
2. Short press to enter submenu
3. Rotate to browse timezones (UTC-ordered)
4. Short press to confirm
5. Long press to cancel

**Note:** Timezone list displays from UTC-12 to UTC+14.

---

## 🎚️ Direct Toggle/Cycle Settings

These settings change **immediately** when selected (no submenu):

### Refresh Mode
- **Partial**: Uses partial refresh (faster, up to 20 refreshes before full)
- **Full**: Always uses full refresh (slower, cleaner)

### LED Brightness
- **Low**: 20% brightness
- **Med**: 50% brightness ⭐ Default
- **High**: 100% brightness

### Time Format
- **24h**: 00:00 - 23:59
- **12h**: 12:00 AM - 11:59 PM ⭐ Default

### Date Format
- **MM/DD/YYYY**: 12/31/2024 (US format) ⭐ Default
- **DD/MM/YYYY**: 31/12/2024 (EU format)
- **YYYY-MM-DD**: 2024-12-31 (ISO format)

### Date/Time Size
- **Small**: Compact clock display
- **Large**: Larger clock display ⭐ Default

### Day Average Line
- **Off**: No average reference line
- **24h mean**: Rolling 24-hour average price ⭐ Default
- **Cycle mean**: 7pm ET cycle average price

---

## ⚡ Quick Tips

### 🐛 V0.99r Bug Fix
**FIXED:** Setting update interval to 10 minutes no longer causes restart!
- Root cause: Array bounds violation (accessing non-existent 5th element)
- Now uses submenu pattern + corrected array size

### 🎯 Best Practices
- **Update Interval**: Use 3m for 1-3 devices, 5m for 4+ devices on same network
- **Refresh Mode**: Use Partial for less screen wear (20 refreshes before auto-full)
- **LED Brightness**: Use Med (50%) for balanced visibility and power consumption

### 🔄 Settings Persistence
All settings are saved to NVS (non-volatile storage) and persist across:
- Device restarts
- Power cycles
- Firmware updates

### 🌐 WiFi Portal Settings
When provisioning WiFi (first boot or after factory reset):
- All advanced settings can be configured via web portal
- Settings apply immediately after WiFi connection
- No need to use device menu for initial setup

---

## 🛠️ Factory Reset

**How to Reset:**
1. Enter menu
2. Press and hold encoder for **12 seconds**
3. Device restarts with defaults:
   - Update: 3m
   - LED: Med
   - Coin: BTC
   - Time: 12h
   - Date: MM/DD/YYYY
   - Timezone: UTC+0 (Auto-detect on first boot)
   - Day avg: 24h mean
   - Refresh: Full
   - WiFi credentials cleared (requires re-provisioning)

---

## 📚 Version History

### V0.99r (2025-12-28) - Settings Bug Fix & UX Improvements
- **CRITICAL FIX**: Fixed array bounds bug causing restart at 10-minute interval
- **UX IMPROVEMENT**: Update interval now uses submenu pattern
- Added: UI_MODE_UPDATE_SUB, src/ui_update.cpp
- Changed: No more accidental data fetching when browsing update intervals

### V0.99q (2025-12-25) - UI/UX Improvements
- Fixed WiFi portal settings application (all 9 advanced settings now work)
- Fixed timezone menu UTC ordering (web portal matches device menu)
- Added independent time refresh (clock updates every minute)

### Earlier Versions
- See CHANGELOG.md for full version history

---

## 🆘 Troubleshooting

### Settings Not Saving
- Check NVS space (factory reset clears storage)
- Ensure settings are confirmed (short press in submenus)
- Long press cancels without saving

### 10-Minute Restart Bug (FIXED in V0.99r)
- **If still experiencing**: Update to V0.99r immediately
- **Root cause**: Array bounds violation (5 declared, 4 actual elements)
- **Solution**: Fixed UPDATE_PRESETS_COUNT to 4

### Update Interval Changes Don't Apply
- **V0.99r+**: Must confirm selection in submenu (short press)
- Long press cancels changes
- Check serial output for "[Menu] Update interval -> Xm" confirmation

---

## 📞 Support

- **Issues**: https://github.com/max05210238/CryptoBar/issues
- **Detailed Docs**: See V0.99r_SETTINGS_FIX.md for technical details
- **LED Guide**: See LED_DISPLAY_GUIDE.md for LED color meanings
