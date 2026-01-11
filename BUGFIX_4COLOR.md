# Bug Fix: Corrected 4-Color Display Driver Class Name

## Issue
Initial commit used incorrect driver class name `GxEPD2_290_GDEY029F51H`, causing compilation errors:
```
error: 'GxEPD2_290_GDEY029F51H' was not declared in this scope
note: suggested alternative: 'GxEPD2_290c_GDEY029F51H'
```

## Fix Applied
Changed all occurrences from:
```cpp
GxEPD2_290_GDEY029F51H  // WRONG
```

To:
```cpp
GxEPD2_290c_GDEY029F51H  // CORRECT
```

## Files Modified (9 files)
1. `src/main.cpp` - Display object declaration
2. `src/ui.cpp` - Display extern declaration
3. `src/ui_menu.cpp` - Display extern declaration
4. `src/ui_coin.cpp` - Display extern declaration
5. `src/ui_currency.cpp` - Display extern declaration
6. `src/ui_timezone.cpp` - Display extern declaration
7. `src/ui_update.cpp` - Display extern declaration
8. `src/ui_list.cpp` - Display extern declaration
9. `include/app_state.h` - Display extern declaration

## Verification
After this fix, the code should compile successfully. The correct class name follows GxEPD2 library naming convention:
- `GxEPD2_290c_GDEY029F51H` - The 'c' indicates color (3-color or 4-color display)
- Part of the GxEPD2_4C template for 4-color displays

## Build Instructions
```bash
# Pull latest changes
git pull origin claude/4color-display-V099r-G-K0kQP

# Build
pio run

# Upload to ESP32
pio run --target upload
```

## Expected Result
- Firmware compiles without errors
- Display initializes correctly on boot
- All UI functions work with 4-color display (currently rendering in black/white)

---

**Last Updated**: 2026-01-11
**Fix Commit**: Corrected GxEPD2_290c_GDEY029F51H class name
