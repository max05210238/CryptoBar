# CryptoBar V0.99r-G - 4-Color Display Support

## Overview

This branch adds support for **WaveShare 2.9" e-Paper Module (G)** - the 4-color (Red/Yellow/Black/White) version using the **GDEY029F51H** display driver.

## Hardware Compatibility

- **Display**: WaveShare 2.9" e-Paper Module (G)
- **Driver IC**: GDEY029F51H (or GDEY029F51)
- **Resolution**: 296 × 128 pixels
- **Colors**: Red, Yellow, Black, White (4-color)
- **Interface**: SPI (same pinout as V2 black/white version)

## Key Differences from Black/White Version

### Display Characteristics
- **Refresh Time**: ~15-20 seconds (vs. 2 seconds for B/W version)
- **Partial Refresh**: ❌ NOT SUPPORTED (4-color displays require full refresh)
- **Color Capability**: 4 colors available (currently using black/white only)

### Wiring
✅ **Pin connections are IDENTICAL to the V2 black/white version**:
- VCC → 3.3V
- GND → GND
- DIN → GPIO 11
- CLK → GPIO 12
- CS → GPIO 10
- DC → GPIO 17
- RST → GPIO 16
- BUSY → GPIO 4

## Code Changes

### Modified Files
1. **src/main.cpp** - Changed to use `GxEPD2_4C<GxEPD2_290_GDEY029F51H>`
2. **src/ui.cpp** - Updated display declarations
3. **src/ui_menu.cpp** - Updated display declarations
4. **src/ui_coin.cpp** - Updated display declarations
5. **src/ui_currency.cpp** - Updated display declarations
6. **src/ui_timezone.cpp** - Updated display declarations
7. **src/ui_update.cpp** - Updated display declarations
8. **src/ui_list.cpp** - Updated display declarations
9. **include/app_state.h** - Updated extern declaration

### Key Code Changes
```cpp
// OLD (Black/White):
#include <GxEPD2_BW.h>
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(...);

// NEW (4-Color):
#include <GxEPD2_4C.h>
GxEPD2_4C<GxEPD2_290_GDEY029F51H, GxEPD2_290_GDEY029F51H::HEIGHT> display(...);
```

## Building the Firmware

### Prerequisites
- PlatformIO Core or PlatformIO IDE (VS Code extension)
- GxEPD2 library v1.5.0 or later (includes GDEY029F51H support)

### Build Instructions

#### Using VS Code + PlatformIO Extension:
1. Open the project in VS Code
2. Click PlatformIO icon in sidebar
3. Select "Build" under esp32-s3-devkitc-1 environment
4. Upload to your device

#### Using Command Line:
```bash
# Navigate to project directory
cd CryptoBar

# Build
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

## Current Display Implementation

### Color Usage (V0.99r-G)
This initial implementation uses **BLACK and WHITE only** for maximum compatibility:
- All text and graphics render in BLACK
- Background is WHITE
- Red and Yellow colors are NOT used yet

### Future Color Enhancements
Potential future improvements:
- **Red**: Price increases, positive percentage changes
- **Yellow**: Reference lines, average price indicators
- **Black**: Main text and static information
- **White**: Background

## Testing Checklist

Before deploying to production:
- [ ] Display shows boot screen correctly
- [ ] WiFi configuration works
- [ ] Main screen displays price and chart
- [ ] Menu navigation works (encoder + display)
- [ ] Time updates every minute
- [ ] Price updates at configured interval
- [ ] LED color indicators work
- [ ] Settings are saved/restored correctly

## Known Limitations

1. **Slow Refresh**: Full refresh takes 15-20 seconds (vs. 2 seconds for B/W)
2. **No Partial Refresh**: Every update requires full screen refresh
3. **Frequent Updates Not Recommended**: Use update intervals ≥ 5 minutes
4. **Color Not Used**: Initial version renders in black/white only

## Troubleshooting

### Display Shows Nothing
1. Verify you have the correct **WaveShare 2.9" Module (G)** (not B or V2)
2. Check all 8 pin connections
3. Look for "GDEY029F51H" or "GDEY029F51" marking on display

### Compilation Errors
```
error: 'GxEPD2_290_GDEY029F51H' was not declared
```
**Solution**: Update GxEPD2 library to version 1.5.0 or later:
```bash
pio lib update
```

### Display Slow or Unresponsive
- This is **normal** for 4-color displays (15-20 second refresh)
- Increase update interval to 5-10 minutes in settings
- Ensure BUSY pin (GPIO 4) is connected correctly

## Reverting to Black/White Version

If you need to switch back to the standard black/white display:

```bash
git checkout main
# Or if on this branch:
git checkout V0.99r  # Standard black/white version
```

## References

- [WaveShare 2.9" Module (G) Product Page](https://www.waveshare.com/2.9inch-e-paper-module-g.htm)
- [WaveShare 2.9" Module (G) Wiki](https://www.waveshare.com/wiki/2.9inch_e-Paper_Module_(G))
- [GxEPD2 Library on GitHub](https://github.com/ZinggJM/GxEPD2)
- [Good Display GDEY029F51H Datasheet](https://www.good-display.com/product/468.html)

## Support

If you encounter issues with the 4-color display version:
1. Check that you have the correct display model (G version)
2. Verify GxEPD2 library version (≥ 1.5.0)
3. Review [troubleshooting section](#troubleshooting) above
4. Open an issue on GitHub with details

---

**Last Updated**: 2026-01-10
**Firmware Version**: V0.99r-G
**Branch**: `V0.99r-G`
