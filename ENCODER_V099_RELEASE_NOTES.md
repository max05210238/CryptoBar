# CryptoBar V0.99 - Encoder Performance Optimization

## Issues Addressed

V0.98 encoder had the following problems:
- ❌ Takes many rotations to move cursor one step
- ❌ Direction reversed or erratic jumps
- ❌ Encoder sometimes completely unresponsive
- ❌ Poor performance with cheap/noisy encoders

## Root Cause Analysis

### 1. **Excessive Hardware Filtering** (Most Critical)
```cpp
// V0.98 old value
ENC_PCNT_FILTER_VAL = 1023  // Maximum filter value, drops valid signals
```
**Problem**: 1023 is the maximum PCNT filter value (~12.8μs). For cheap encoders with slow signal edges, this causes many valid pulses to be filtered out.

### 2. **Aggressive Debounce Logic** (Core Issue)
```cpp
// V0.98 old logic
if (direction_changed && time<150ms && steps<=2) {
    emit = 0;
    s_encBacklog = 0;        // Clears ALL pending steps!
    s_encDetentAccum = 0;    // Clears accumulator!
}
```
**Problems**:
- 150ms direction lock is too long, even normal quick operations get filtered
- When triggered, clears **ALL** accumulated values, losing all previous rotations
- Condition too broad (`<=2` steps), continuous operations get falsely detected

### 3. **Detent Count Mismatch**
```cpp
ENC_COUNTS_PER_DETENT = 2  // Fixed value may not suit all encoders
```
**Problem**: Different encoders have different counts per detent (1, 2, or 4). Fixed value causes too slow or too fast response.

## V0.99 Solutions

### ✅ Improvement 1: Reduced Hardware Filter
```cpp
// V0.99 new value
ENC_PCNT_FILTER_VAL = 200  // Reduced from 1023 to 200 (~2.5μs)
```
**Effects**:
- More signals captured, especially slow edges from cheap encoders
- Still filters high-frequency noise
- Configurable range: 100 (very responsive) → 200 (balanced) → 500 (very noisy)

### ✅ Improvement 2: Relaxed Direction Lock
```cpp
// V0.99 new value
ENC_DIR_LOCK_MS = 30  // Reduced from 150ms to 30ms
```
**Effects**:
- Only filters genuinely accidental fast reversals (<30ms)
- Normal operations (>30ms) are not falsely detected as bounce
- Can be set to 0 to completely disable direction lock (most responsive)

### ✅ Improvement 3: Smart Debounce Logic
```cpp
// V0.99 new logic
if (ENC_DIR_LOCK_MS > 0 &&           // Allow disabling
    s_lastEncDir != 0 &&
    dir != s_lastEncDir &&
    (nowMs - s_lastEncStepMs) < ENC_DIR_LOCK_MS &&
    abs(emit) == 1) {                // Only filter single-step reversals (not <=2)

    emit = 0;                         // Only discard current emit
    // V0.99: Don't clear s_encBacklog and s_encDetentAccum
    // They might be legitimate accumulated values
}
```
**Effects**:
- Only filters single-step (`==1`) reversals, doesn't affect multi-step operations
- Doesn't clear accumulator, preserves legitimate accumulated values
- Stricter conditions, reduces false positives

### ✅ Improvement 4: Debug Mode
```cpp
// V0.99 new feature
ENC_DEBUG = 0  // Set to 1 to enable debug output
```
**Features**:
- Shows raw PCNT counts
- Shows detent steps and pending queue
- Shows debounce filter events
- Shows final steps emitted to UI

Example output when enabled:
```
[ENC] V0.99 PCNT enabled
[ENC] Config: Filter=200 APB, Counts/Detent=2, DirInvert=1, DirLock=30ms
[ENC] Raw PCNT: 4 (inverted: -4)
[ENC] Detent steps: -2, Backlog: -2, Accum: 0
[ENC] Emitting -2 steps to UI (backlog remaining: 0)
```

### ✅ Improvement 5: Comprehensive Documentation
Added complete configuration guide in `encoder_pcnt.h`, including:
- Detailed description of each parameter
- Recommended tuning ranges
- Suggested values for different use cases

## Configuration Guide

### Quick Tuning Steps

1. **If encoder is too slow or loses steps**:
   ```cpp
   // Reduce filter value
   ENC_PCNT_FILTER_VAL = 100  // Or lower to 50

   // Disable direction lock
   ENC_DIR_LOCK_MS = 0

   // If still slow, check detent count
   ENC_COUNTS_PER_DETENT = 1  // Try 1 instead of 2
   ```

2. **If encoder has erratic jumps or false triggers**:
   ```cpp
   // Increase filter value
   ENC_PCNT_FILTER_VAL = 300  // Or higher to 500

   // Increase direction lock
   ENC_DIR_LOCK_MS = 50
   ```

3. **If direction is reversed**:
   ```cpp
   // Toggle direction inversion
   ENC_DIR_INVERT = 0  // Or change to 1
   ```

4. **To diagnose issues**:
   ```cpp
   // Enable debug mode
   ENC_DEBUG = 1

   // Compile, upload, then open Serial Monitor (115200 baud)
   // Observe raw encoder counts and processing
   ```

### Recommended Configurations (by Encoder Type)

#### High-Quality Encoders (e.g., Alps, Bourns)
```cpp
ENC_PCNT_FILTER_VAL = 100
ENC_COUNTS_PER_DETENT = 2 or 4
ENC_DIR_LOCK_MS = 0
```

#### Standard Cheap Encoders (typical mechanical rotary encoders)
```cpp
ENC_PCNT_FILTER_VAL = 200  // V0.99 default
ENC_COUNTS_PER_DETENT = 2
ENC_DIR_LOCK_MS = 30
```

#### Very Noisy/Bouncy Encoders
```cpp
ENC_PCNT_FILTER_VAL = 400-500
ENC_COUNTS_PER_DETENT = 2
ENC_DIR_LOCK_MS = 50
```

## Modified Files

1. **src/encoder_pcnt.cpp**
   - Updated version to V0.99
   - Reduced `ENC_PCNT_FILTER_VAL`: 1023 → 200
   - Reduced `ENC_DIR_LOCK_MS`: 150 → 30
   - Improved debounce logic: only filter single-step reversals, don't clear accumulator
   - Added debug output

2. **include/encoder_pcnt.h**
   - Added comprehensive configuration guide documentation
   - Explained purpose and tuning suggestions for each parameter

3. **src/main.cpp**
   - Updated version to V0.99

## Expected Improvements

### Responsiveness
- ✅ Immediate response to encoder rotation, no lag
- ✅ Fast continuous rotation correctly accumulates steps
- ✅ No lost steps when changing direction

### Stability
- ✅ Signals from cheap encoders are properly captured
- ✅ Reduced false bounce detection
- ✅ Preserves legitimate accumulated steps

### Configurability
- ✅ Easy adaptation to different encoders via simple constant changes
- ✅ Debug mode helps diagnose issues
- ✅ Detailed documentation guides tuning

## Testing Recommendations

1. **Basic Tests**:
   - Slow single-direction rotation: should respond once per detent
   - Fast single-direction rotation: all steps should accumulate correctly
   - Fast direction changes: direction should change immediately without losing steps

2. **Stress Tests**:
   - Very fast rotation: check for lost counts
   - Frequent direction changes: check for false filtering
   - Slight bouncy rotation: check for false triggers

3. **Debug Tests**:
   - Enable `ENC_DEBUG = 1`
   - Observe Serial output
   - Verify raw PCNT values and final emitted steps are reasonable

## If Issues Persist

If V0.99 default configuration still doesn't work well:

1. **Enable debug mode** (`ENC_DEBUG = 1`)
2. **Record Serial output**, including:
   - Raw PCNT values
   - Detent steps
   - Whether Bounce filter is triggering
3. **Try different configuration combinations**
4. **Verify encoder hardware connections**:
   - CLK → GPIO 5
   - DT → GPIO 6
   - SW → GPIO 21
   - Common ground (GND)
   - Check pull-up resistors (internal pull-ups are enabled)

## Version History

- **V0.97**: Baseline PCNT implementation
- **V0.98**: Code refactoring, extracted menu functionality
- **V0.99**: Encoder performance optimization (this version)
  - Reduced hardware filter value
  - Relaxed direction lock
  - Improved debounce logic
  - Added debug mode
  - Enhanced configuration documentation
