#pragma once

#include <Arduino.h>
#include <driver/pcnt.h>

// ==================== CryptoBar V0.99a Encoder Configuration ====================
//
// PCNT-based rotary encoder optimized for Bourns PEC11R-S0024 (smooth, 24 PPR).
//
// HARDWARE SETUP:
// - Encoder: Bourns PEC11R-S0024 (24 pulses/rev, smooth/no detents)
// - GPIO Pins: CLK=GPIO2, DT=GPIO1 (ESP32-S3 compatible)
// - Note: GPIO 5/6 don't support PCNT on ESP32-S3
//
// V0.99a IMPROVEMENTS FROM V0.98:
// - Fixed GPIO pins: 5/6 → 1/2 (ESP32-S3 PCNT compatibility)
// - Fixed quadrature decoding: proper X2 mode (pos_mode=DEC, neg_mode=INC)
// - Swapped CLK/DT assignment for correct direction
// - Optimized for smooth encoder: COUNTS=6 (1/8 rev = 1 step)
// - Added EMI spike rejection (threshold=16) for E-ink display interference
// - Reduced filter to 150 APB cycles for better responsiveness
// - Minimal direction lock (10ms) for smooth rotation
// - Verbose debug mode (ENC_DEBUG=2) for diagnostics
//
// CONFIGURATION GUIDE (edit encoder_pcnt.cpp):
//
// 1. ENC_PCNT_FILTER_VAL (current: 150)
//    - Hardware noise filter in APB cycles (~2.5μs @ 80MHz)
//    - Lower = more responsive, higher = more noise rejection
//    - Range: 0-1023
//    - Try: 100 (very responsive), 150 (current), 300 (EMI environment)
//
// 2. ENC_COUNTS_PER_DETENT (current: 6)
//    - How many PCNT counts = 1 UI step
//    - Current: 1/8 revolution = 1 step (48 counts/rev ÷ 8 = 6)
//    - For Bourns PEC11R-S0024: X2 mode gives 48 counts/revolution
//    - Try: 3 (1/16 rev), 6 (1/8 rev), 12 (1/4 rev)
//
// 3. ENC_DIR_INVERT (current: 0)
//    - Direction: 0 = CW is positive (up), 1 = reversed
//
// 4. ENC_DIR_LOCK_MS (current: 10)
//    - Time window to filter accidental direction reversals
//    - Smooth encoders need minimal filtering
//    - Range: 0-50ms
//    - Try: 0 (no filtering), 10 (current), 30 (more filtering)
//
// 5. ENC_DEBUG (current: 2)
//    - 0 = disabled, 1 = basic debug, 2 = verbose (every 100ms poll)
//    - Verbose mode shows PCNT values and GPIO levels for diagnostics
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
