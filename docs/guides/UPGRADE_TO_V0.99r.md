# Upgrade to V0.99r - Critical Bug Fix

**Release Date:** 2025-12-28
**Priority:** 🔴 **HIGH** - Critical bug fix, update immediately
**Version:** V0.99r

---

## ⚠️ Why You Should Upgrade

### 🔴 CRITICAL: System Restart Bug Fixed

**Problem:**
- Setting update interval to **10 minutes** caused device to restart
- After restart, changing from 10 minutes to any other interval caused another restart
- Device became **unusable** when 10-minute interval was selected

**Root Cause:**
```cpp
// BROKEN (V0.99q and earlier):
#define UPDATE_PRESETS_COUNT 5  // Declared 5 elements
const uint32_t UPDATE_PRESETS_MS[] = {
  60000,   // 1min (index 0)
  180000,  // 3min (index 1)
  300000,  // 5min (index 2)
  600000   // 10min (index 3)
};  // Only 4 elements! Index 4 doesn't exist

// When at 10min (index 3), pressing button:
g_updatePresetIndex = (3 + 1) % 5 = 4
g_updateIntervalMs = UPDATE_PRESETS_MS[4];  // ❌ OUT OF BOUNDS!
// → Read garbage memory → ESP32 watchdog triggered → RESTART
```

**Fixed (V0.99r):**
```cpp
#define UPDATE_PRESETS_COUNT 4  // ✅ Correct: matches array size
```

---

## ✨ UX Improvements

### 📱 Update Interval Now Uses Submenu

**Before (V0.99q):**
- Direct toggle on "Update" menu item
- Each button press immediately changed setting
- Triggered data fetch and scheduler reset
- Couldn't browse options without side effects
- Caused accidental API requests

**After (V0.99r):**
- Enter submenu to browse all 4 options (1m, 3m, 5m, 10m)
- Rotate encoder to preview options
- **No data fetching** until confirmed ✅
- Short press to confirm, long press to cancel
- Consistent with Coin/Currency selection UX

**Benefits:**
- ✅ Browse update intervals without triggering updates
- ✅ Reduces unnecessary API requests
- ✅ Prevents accidental settings changes
- ✅ Consistent user experience across all multi-option settings

---

## 📦 What's Included

### Files Added
- `src/ui_update.cpp` - Update interval submenu UI
- `V0.99r_SETTINGS_FIX.md` - Detailed technical documentation
- `SETTINGS_MENU_GUIDE.md` - User guide for all menu operations
- `UPGRADE_TO_V0.99r.md` - This file

### Files Modified
- `include/app_state.h` - Fixed UPDATE_PRESETS_COUNT, added UI mode & state
- `src/app_state.cpp` - Updated version, fixed array comments
- `src/app_menu.cpp` - Changed to submenu pattern
- `src/app_input.cpp` - Handle submenu selection
- `src/main.cpp` - Handle encoder navigation in submenu
- `include/ui.h` - Added submenu UI functions
- `include/app_menu.h` - Added handler declaration
- `CHANGELOG.md` - Added V0.99r entry

### Version String
```cpp
const char* CRYPTOBAR_VERSION = "V0.99r";
```

---

## 🚀 How to Upgrade

### Method 1: OTA Update (Recommended)

1. **Enter Firmware Update Mode:**
   - From main screen, short press encoder to enter menu
   - Navigate to "Firmware Update"
   - Short press to enter confirmation screen
   - **Long press (hold)** to confirm and reboot into update mode
   - Device shows purple LED and displays AP credentials

2. **Connect to Update AP:**
   - Connect your phone to WiFi: `CryptoBar-Update-XXXX`
   - Open browser, go to `http://192.168.4.1`

3. **Upload Firmware:**
   - Click "Choose File" and select `firmware.bin` (V0.99r)
   - Click "Update Firmware"
   - Wait for upload (progress bar)
   - Device automatically reboots

4. **Verify Update:**
   - Check version on startup splash screen: "V0.99r"
   - Enter settings menu
   - Try changing update interval to 10 minutes
   - Should enter submenu (no restart) ✅

### Method 2: USB Flash (Advanced)

1. **Connect via USB:**
   ```bash
   platformio run --target upload
   ```

2. **Or use esptool.py:**
   ```bash
   esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
     write_flash 0x10000 firmware.bin
   ```

3. **Verify:**
   - Device reboots automatically
   - Check version on splash screen

---

## ✅ Post-Upgrade Testing

### Critical Bug Verification

1. **Test 10-Minute Setting:**
   - Enter menu → Navigate to "Update"
   - Short press to enter submenu
   - Rotate to "10m"
   - Short press to confirm
   - **Expected:** Returns to menu, no restart ✅
   - **Old bug:** Device restarts ❌

2. **Test 10m → Other Interval:**
   - With 10m selected, enter menu again
   - Navigate to "Update" → Enter submenu
   - Change to "1m" or "3m" or "5m"
   - **Expected:** Changes successfully, no restart ✅
   - **Old bug:** Device restarts ❌

3. **Test Reboot with 10m:**
   - Set to 10 minutes
   - Long press encoder (12s) for factory reset OR power cycle
   - Device reboots
   - Check settings menu shows "Update: 10m"
   - **Expected:** Boots normally ✅

### UX Improvement Verification

1. **Test Submenu Navigation:**
   - Enter menu → "Update" → Short press
   - **Expected:** Enters submenu showing all 4 options
   - Rotate encoder → Highlights different options
   - **Expected:** No data fetching, no scheduler reset

2. **Test Confirmation:**
   - In submenu, rotate to different option
   - Short press
   - **Expected:** Applies setting, returns to menu
   - Check serial output: `[Menu] Update interval -> Xm`

3. **Test Cancellation:**
   - Enter submenu, rotate to different option
   - **Long press** instead of short press
   - **Expected:** Returns to menu without saving change

4. **Test Serial Logging:**
   ```
   [Menu] Enter UPDATE submenu
   [Menu] Update interval -> 3m
   [Sched] Reset(MENU_UPD): int=180s nextUtc=...
   ```

---

## 🔄 Rollback (If Needed)

If you experience issues with V0.99r:

### OTA Rollback
1. ESP32 OTA partition system keeps previous firmware
2. If new firmware crashes repeatedly, ESP32 auto-rolls back
3. Manual rollback: Flash V0.99q firmware via OTA update

### Factory Reset
If settings are corrupted:
1. Enter menu
2. Press and hold encoder for **12 seconds**
3. Device resets to defaults (WiFi credentials cleared)

---

## 📊 Version Comparison

| Feature | V0.99q | V0.99r |
|---------|--------|--------|
| **Update interval presets** | 4 options (1m, 3m, 5m, 10m) | 4 options (1m, 3m, 5m, 10m) |
| **Array bounds bug** | ❌ Crashes at 10m | ✅ Fixed |
| **Update interval UI** | Direct toggle | Submenu |
| **Browse without fetching** | ❌ No | ✅ Yes |
| **Consistent UX** | Partial | ✅ Full |
| **UPDATE_PRESETS_COUNT** | 5 (WRONG) | 4 (CORRECT) |

---

## 🆘 Troubleshooting

### Still Experiencing Restarts?

1. **Verify version:**
   - Check splash screen shows "V0.99r"
   - Serial output: `[Version] V0.99r`

2. **Check firmware upload:**
   - OTA update shows "Update OK. Rebooting..."
   - File size should be ~700KB - 1MB

3. **Clear NVS (if corrupted):**
   - Factory reset (12s long press)
   - Re-provision WiFi
   - Re-configure settings

### Submenu Not Appearing?

1. **Check version:** Must be V0.99r or later
2. **Serial debug:**
   ```
   [Menu] Enter UPDATE submenu  ← Should appear
   ```
3. **Reflash firmware** if menu behavior unchanged

### Settings Not Saving?

1. **Confirm with short press** (not long press)
2. **Check serial output** for save confirmation
3. **Factory reset** if NVS corrupted

---

## 📚 Additional Resources

- **Technical Details:** See `V0.99r_SETTINGS_FIX.md`
- **Settings Guide:** See `SETTINGS_MENU_GUIDE.md`
- **LED Guide:** See `LED_DISPLAY_GUIDE.md`
- **Full Changelog:** See `CHANGELOG.md`
- **Issues:** https://github.com/max05210238/CryptoBar/issues

---

## 🎯 Summary

**V0.99r is a critical update that:**
1. ✅ Fixes system restart bug at 10-minute update interval
2. ✅ Improves update interval selection UX (submenu pattern)
3. ✅ Prevents accidental data fetching when browsing settings
4. ✅ Ensures consistent user experience across all settings

**Upgrade Priority: HIGH** - Update immediately to prevent crashes

**Tested Platforms:**
- ESP32-S3-DevKitC-1 ✅
- GxEPD2_290_BS e-paper display ✅
- Arduino-ESP32 framework 2.0.x ✅

**Breaking Changes:** None (settings format unchanged)

**Migration:** Automatic (no manual configuration needed)

---

## 📝 Credits

Bug reported by user experiencing restart issues when selecting 10-minute update interval.

Fixed by Claude Code Agent SDK in collaboration with project maintainer.

---

**Last Updated:** 2025-12-28
**Document Version:** 1.0
