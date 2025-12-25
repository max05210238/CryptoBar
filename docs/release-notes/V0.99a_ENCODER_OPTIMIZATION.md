# CryptoBar V0.99a - Encoder Performance Optimization (Final)

## Hardware Configuration

**Encoder**: Bourns PEC11R-4020F-S0024
- Type: Rotary encoder, optical or mechanical incremental
- Resolution: 24 pulses per revolution (PPR)
- Detents: **Smooth (no detents)** - S suffix indicates smooth rotation
- Quality: High-quality professional encoder (not cheap)

**GPIO Pins (ESP32-S3)**:
- CLK: GPIO 2 (was GPIO 5 in V0.98 - incompatible with ESP32-S3 PCNT)
- DT:  GPIO 1 (was GPIO 6 in V0.98 - incompatible with ESP32-S3 PCNT)
- SW:  GPIO 21 (button, unchanged)

**Critical Note**: GPIO 5 and 6 are reserved for SPI Flash on ESP32-S3 and do not support PCNT peripheral.

## Issues Addressed from V0.98

V0.98 encoder had the following problems:
- ❌ Takes many rotations to move cursor one step
- ❌ Direction constantly reversed during steady rotation (±1 oscillation)
- ❌ Encoder sometimes completely unresponsive (PCNT = 0)
- ❌ Erratic jumps and direction changes
- ❌ No response despite GPIO voltage changes confirmed with multimeter

## Root Cause Analysis

### 1. **GPIO Pin Incompatibility** (Critical Hardware Issue)
**Problem**: GPIO 5 and 6 don't support PCNT on ESP32-S3
- User confirmed with multimeter: GPIO 5/6 showed 0V and 3.3V changes during rotation
- But PCNT stayed at 0 - hardware peripheral couldn't access these pins
- ESP32-S3 reserves GPIO 5/6 for SPI Flash

**Solution**: Migrated to GPIO 1 (DT) and GPIO 2 (CLK)
- GPIO 1/2 fully support PCNT on ESP32-S3
- Confirmed PCNT values appeared after pin change

### 2. **Incorrect Quadrature Decoding** (Critical Software Issue)
```cpp
// V0.98 WRONG configuration
cfg.pos_mode = PCNT_COUNT_INC;  // CLK rising edge +1
cfg.neg_mode = PCNT_COUNT_INC;  // CLK falling edge +1
```

**Problem**: This is NOT quadrature decoding - just counting CLK edges!
- Ignores phase relationship between CLK and DT
- Direction determined only by current DT level, not phase lead/lag
- Result: During steady clockwise rotation, saw +1, -1, +1, -1 oscillation

```cpp
// V0.99a CORRECT configuration
cfg.pos_mode = PCNT_COUNT_DEC;  // CLK rising: DT=HIGH→-1, DT=LOW→+1
cfg.neg_mode = PCNT_COUNT_INC;  // CLK falling: DT=HIGH→+1, DT=LOW→-1
```

**Solution**: Proper quadrature X2 mode
- Detects phase relationship between CLK and DT
- CW rotation: CLK leads DT → consistent negative counts
- CCW rotation: DT leads CLK → consistent positive counts

### 3. **Inverted CLK/DT Assignment**
After GPIO fix, direction was still erratic until CLK and DT were swapped:
- Original: CLK=GPIO1, DT=GPIO2
- Final: CLK=GPIO2, DT=GPIO1 (swapped in software, no rewiring needed)

### 4. **Wrong Sensitivity for Smooth Encoder**
**Problem**: COUNTS_PER_DETENT = 4 was too high
- Smooth encoder (no mechanical detents) needs low threshold
- With threshold=4, accumulator oscillated 0→1→2→3→2→1→0, never reaching 4
- Cursor barely moved

**Analysis for Bourns PEC11R-S0024**:
- 24 pulses/revolution
- X2 quadrature mode: 24 × 2 edges = 48 counts/revolution
- User requirement: 1/8 revolution = 1 cursor step
- Calculation: 48 ÷ 8 = 6 counts

**Solution**: COUNTS_PER_DETENT = 6

### 5. **Direction Inverted**
User reported cursor moved in wrong direction (CW moved cursor down, should move up).

**Solution**: DIR_INVERT = 1 → 0

### 6. **EMI from E-ink Display**
User observed sudden large PCNT jumps (e.g., PCNT=7, PCNT=-23).
E-ink displays generate high-voltage pulses during refresh that couple into encoder wiring.

**Solution**: Added EMI spike rejection
```cpp
const int EMI_SPIKE_THRESHOLD = 16;
if (abs(cnt) > EMI_SPIKE_THRESHOLD) {
    // Reject and clear counter
}
```

## V0.99a Solutions Summary

### ✅ Fix 1: GPIO Pin Change (Hardware)
```cpp
// V0.98
#define ENC_CLK_PIN 5  // ❌ Doesn't support PCNT on ESP32-S3
#define ENC_DT_PIN 6   // ❌ Doesn't support PCNT on ESP32-S3

// V0.99a
#define ENC_CLK_PIN 2  // ✅ Swapped and PCNT-compatible
#define ENC_DT_PIN 1   // ✅ Swapped and PCNT-compatible
```

### ✅ Fix 2: Proper Quadrature Decoding
```cpp
// V0.99a
cfg.pos_mode = PCNT_COUNT_DEC;  // Proper phase detection
cfg.neg_mode = PCNT_COUNT_INC;  // X2 quadrature mode
cfg.lctrl_mode = PCNT_MODE_REVERSE;
cfg.hctrl_mode = PCNT_MODE_KEEP;
```

### ✅ Fix 3: Smooth Encoder Optimization
```cpp
// V0.99a configuration for Bourns PEC11R-S0024
ENC_PCNT_FILTER_VAL = 150     // Responsive for smooth encoder
ENC_COUNTS_PER_DETENT = 6     // 1/8 revolution = 1 step
ENC_DIR_INVERT = 0            // Correct direction (CW = up)
ENC_DIR_LOCK_MS = 10          // Minimal lock for smooth rotation
```

### ✅ Fix 4: EMI Spike Rejection
```cpp
// Reject unrealistic jumps from E-ink display interference
if (abs(cnt) > 16) {
    if (ENC_DEBUG) {
        Serial.printf("[ENC] EMI SPIKE REJECTED: PCNT=%d\n", cnt);
    }
    pcnt_counter_clear(ENC_PCNT_UNIT);
    return;
}
```

### ✅ Fix 5: Enhanced Debug Mode
```cpp
// V0.99a: Verbose debug mode
ENC_DEBUG = 2  // Prints every 100ms: PCNT, CLK, DT, Accum, Backlog
```

Example output:
```
[ENC] V0.99a PCNT enabled
[ENC] Config: Filter=150 APB, Counts/Detent=6, DirInvert=0, DirLock=10ms
[ENC] Poll #12345 @ 10000ms: PCNT=-6, CLK=1, DT=0, Accum=0, Backlog=0
[ENC] Raw PCNT: -6 (inverted: 6)
[ENC] Detent steps: 6, Backlog: 6, Accum: 0
[ENC] Emitting 3 steps to UI (backlog remaining: 3)
[ENC] EMI SPIKE REJECTED: PCNT=-23 (threshold=16)
```

## Configuration Guide

### Current V0.99a Configuration

```cpp
// Optimized for Bourns PEC11R-S0024 smooth encoder
ENC_PCNT_FILTER_VAL = 150     // 150 APB cycles (~1.9μs @ 80MHz)
ENC_COUNTS_PER_DETENT = 6     // 1/8 revolution = 1 step
ENC_DIR_INVERT = 0            // CW = positive (up)
ENC_DIR_LOCK_MS = 10          // Minimal direction lock
ENC_DEBUG = 2                 // Verbose debug enabled
```

### Tuning Guidelines

**1. Adjust Sensitivity (Counts per Step)**
```cpp
// For different step sizes
ENC_COUNTS_PER_DETENT = 3     // 1/16 rev (more sensitive)
ENC_COUNTS_PER_DETENT = 6     // 1/8 rev (current)
ENC_COUNTS_PER_DETENT = 12    // 1/4 rev (less sensitive)
```

**2. Adjust EMI Rejection**
```cpp
// If in high-EMI environment
ENC_PCNT_FILTER_VAL = 300     // Stronger hardware filtering

// If getting false EMI spike rejections
EMI_SPIKE_THRESHOLD = 24      // Increase threshold
```

**3. Disable Debug for Production**
```cpp
ENC_DEBUG = 0  // Disable for cleaner Serial output
```

## Physical Setup Recommendations

### Wire Management
1. **Twist CLK and DT wires together** (reduces EMI pickup)
2. **Separate encoder wires from E-ink display cables** (>5cm spacing)
3. **Keep wires as short as practical**
4. **Cross display cables at 90° if they must cross**

### Optional Hardware Filtering
If software filtering insufficient, add 100nF ceramic capacitors:
```
GPIO 2 (CLK) ──[100nF]── GND
GPIO 1 (DT)  ──[100nF]── GND
```

## Modified Files

### Core Files
1. **src/app_state.cpp**
   - Updated version constant: `CRYPTOBAR_VERSION = "V0.99a"`

2. **src/main.cpp**
   - Updated header comment: `// CryptoBar V0.99a`

3. **include/app_state.h**
   - Updated GPIO pins: `ENC_CLK_PIN = 2`, `ENC_DT_PIN = 1`

4. **src/encoder_pcnt.cpp**
   - Fixed quadrature mode: `pos_mode = PCNT_COUNT_DEC`
   - Optimized config: FILTER=150, COUNTS=6, DIR_LOCK=10ms
   - Added EMI spike rejection
   - Updated all V0.99 → V0.99a

5. **include/encoder_pcnt.h**
   - Comprehensive documentation update
   - Hardware setup information (Bourns PEC11R-S0024)
   - GPIO compatibility notes for ESP32-S3
   - Configuration guide with current values

### Documentation
6. **ENCODER_V099a_RELEASE_NOTES.md** (this file)
   - Complete V0.99a changelog and technical details

## Expected Improvements

### ✅ Functionality Restored
- Encoder responds correctly to rotation
- Consistent unidirectional movement during steady rotation
- No more ±1 oscillation
- GPIO pin compatibility with ESP32-S3

### ✅ User Experience
- Correct direction (CW = cursor up)
- Balanced sensitivity (1/8 turn = 1 step)
- Smooth, predictable cursor movement
- EMI spikes automatically rejected

### ✅ Diagnostics
- Verbose debug mode shows real-time PCNT values
- GPIO levels visible for hardware troubleshooting
- EMI rejection events logged

## Testing Confirmation

User tested at **30 RPM clockwise rotation**:

**Before V0.99a**:
```
PCNT: +1, -1, +1, -1, +1, -1...  ❌ Constant oscillation
Cursor: jumps up and down erratically
```

**After V0.99a**:
```
PCNT: -6, -9, -13, -8, -11...  ✅ Consistent direction
Cursor: moves smoothly in one direction
User feedback: "太爽了，終於修好了" (So satisfying, finally fixed!)
```

## Version History

- **V0.97**: Baseline PCNT implementation
- **V0.98**: Code refactoring, extracted menu functionality
  - GPIO 5/6 for encoder (incompatible with ESP32-S3)
  - Incorrect quadrature decoding
- **V0.99**: Encoder performance optimization attempt
  - Reduced filter values
  - Relaxed direction lock
  - Added debug mode
  - **Issue**: Still had fundamental GPIO and decoding problems
- **V0.99a**: Encoder performance optimization (final)
  - Fixed GPIO pins: 5/6 → 1/2 (ESP32-S3 compatibility)
  - Fixed quadrature decoding: proper X2 mode
  - Optimized for Bourns PEC11R-S0024 smooth encoder
  - Added EMI spike rejection for E-ink interference
  - Correct direction and sensitivity
  - **Status**: ✅ Working correctly per user confirmation

## Troubleshooting

### If encoder still not working

1. **Check GPIO connections**:
   - CLK must be on GPIO 2
   - DT must be on GPIO 1
   - Verify with multimeter: voltage should change during rotation

2. **Enable verbose debug**:
   ```cpp
   ENC_DEBUG = 2
   ```
   Check Serial Monitor for:
   - PCNT values (should be non-zero during rotation)
   - CLK/DT GPIO levels (should toggle: 0/1)

3. **Test PCNT hardware**:
   If PCNT stays 0 despite GPIO changes → GPIO doesn't support PCNT

4. **Check for EMI**:
   If seeing "EMI SPIKE REJECTED" frequently → adjust threshold or add hardware filtering

### If direction is wrong
```cpp
ENC_DIR_INVERT = 1  // Toggle between 0 and 1
```

### If too sensitive/insensitive
```cpp
// More sensitive (smaller rotation per step)
ENC_COUNTS_PER_DETENT = 3

// Less sensitive (larger rotation per step)
ENC_COUNTS_PER_DETENT = 12
```

## Acknowledgments

This fix required systematic debugging through:
1. Hardware compatibility analysis (GPIO PCNT support)
2. PCNT configuration deep dive (quadrature decoding modes)
3. Physical testing with twisted wires and EMI isolation
4. Encoder datasheet analysis (Bourns PEC11R-S0024 specifications)
5. Sensitivity tuning based on user requirements (1/8 revolution)

Special thanks to the user for:
- Patient testing with 30 RPM controlled rotation
- Multimeter verification of GPIO signals
- Detailed log capture and feedback
- Clear requirements (direction and sensitivity)
