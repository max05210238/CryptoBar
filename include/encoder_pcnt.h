#pragma once

#include <Arduino.h>
#include <driver/pcnt.h>

// PCNT-based rotary encoder decode (baseline from V0.97).
// refactor-only: moved out of main.cpp. No behavior change.

// Initialize PCNT unit for the encoder.
// - clkPin: encoder A/CLK
// - dtPin: encoder B/DT (direction)
void encoderPcntBegin(int clkPin, int dtPin);

// Poll PCNT counter and accumulate steps into the provided accumulator.
// - appRunning: true when UI is in normal running state; if false, rotation is discarded.
// - stepAccum/mux: destination accumulator (same semantics as previous g_encStepAccum + g_encMux)
void encoderPcntPoll(bool appRunning, volatile int* stepAccum, portMUX_TYPE* mux);
