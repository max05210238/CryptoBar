#pragma once

#include <Arduino.h>
#include <driver/pcnt.h>

// ==================== CryptoBar V0.99 Encoder Configuration ====================
//
// PCNT-based rotary encoder with optimized performance for cheap/noisy encoders.
//
// V0.99 IMPROVEMENTS:
// - Reduced hardware filter (1023→200 APB cycles) for better signal capture
// - Relaxed direction lock (150ms→30ms) to prevent dropping legitimate movements
// - Smarter debounce that only filters single-step reversals
// - Debug mode for diagnostics
//
// CONFIGURATION GUIDE (edit encoder_pcnt.cpp):
//
// 1. ENC_PCNT_FILTER_VAL (default: 200)
//    - Hardware noise filter in APB cycles (~2.5μs @ 80MHz)
//    - Lower = more responsive but more noise
//    - Higher = less noise but may miss signals
//    - Range: 0-1023
//    - Try: 100 (very responsive), 200 (balanced), 500 (very noisy encoder)
//
// 2. ENC_COUNTS_PER_DETENT (default: 2)
//    - How many PCNT counts = 1 click/detent
//    - Depends on your encoder hardware
//    - Try: 1 (fast response), 2 (standard), 4 (high-resolution encoder)
//
// 3. ENC_DIR_INVERT (default: 1)
//    - Set to 1 if direction is reversed, 0 for normal
//
// 4. ENC_DIR_LOCK_MS (default: 30)
//    - Time window to filter accidental direction reversals
//    - Lower = more responsive but may allow some bounce
//    - Higher = filters more bounce but may feel sluggish
//    - Range: 0-150ms
//    - Try: 0 (no filtering, most responsive), 30 (balanced), 50 (noisy encoder)
//
// 5. ENC_DEBUG (default: 0)
//    - Set to 1 to enable Serial debug output
//    - Shows raw PCNT counts, steps, and filtering events
//    - Useful for diagnosing encoder issues
//
// ==================== API Functions ====================

// Initialize PCNT unit for the encoder.
// - clkPin: encoder A/CLK
// - dtPin: encoder B/DT (direction)
void encoderPcntBegin(int clkPin, int dtPin);

// Poll PCNT counter and accumulate steps into the provided accumulator.
// - appRunning: true when UI is in normal running state; if false, rotation is discarded.
// - stepAccum/mux: destination accumulator (same semantics as previous g_encStepAccum + g_encMux)
void encoderPcntPoll(bool appRunning, volatile int* stepAccum, portMUX_TYPE* mux);
