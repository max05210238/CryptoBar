// led_status.h
// CryptoBar V0.97: move LED logic (NeoPixel + breathe thresholds + re-assert) out of main.cpp
#pragma once

#include <Arduino.h>

// Initialize external WS2812 + onboard WS2812 (used only to force off/clear).
void ledStatusBegin(uint8_t neopixelPin, uint16_t neopixelCount,
                    uint8_t boardPin, uint16_t boardCount);

// Master brightness 0..1 ("LED brightness" setting acts as max ceiling).
void ledStatusSetMasterBrightness(float master01);
float ledStatusGetMasterBrightness();

// Read current logical (unscaled) RGB that firmware thinks it is showing.
void ledStatusGetLogicalRgb(uint8_t* r, uint8_t* g, uint8_t* b);

// Basic color controls (logical RGB; master brightness applied internally)
void setLed(uint8_t r, uint8_t g, uint8_t b);
void fadeLedTo(uint8_t r, uint8_t g, uint8_t b, int steps = 20, int delayMs = 15);

void setLedOff();
void setLedGreen();
void setLedRed();
void setLedBlue();
void setLedPurple();
void setLedYellow();
void setLedCyan();
void setLedWhiteLow();

// Update target LED trend + animation based on 24h change.
// priceOk indicates whether price data is valid (if false, LED shows yellow).
// Special modes: +20% triggers rainbow party mode; breathing for 5%+ moves.
void updateLedForPrice(double change24h, bool priceOk);

// Non-blocking LED animation tick; call every loop.
// appRunning indicates whether UI is in normal running state (skip updates during provisioning).
void ledAnimLoop(bool appRunning, bool priceOk);

// Optional: run LED breathing on a dedicated low-rate task (20–30 Hz recommended)
// so it keeps animating even while the main thread is blocked (e.g. e-paper refresh).
// Safe to call multiple times.
void ledAnimStartTask(uint16_t hz = 30, uint8_t core = 0);

// Periodic service tick; call every loop (safe even if ledAnimLoop isn't called).
// - Re-asserts the last external NeoPixel state so it never "falls dark".
// - Re-asserts onboard WS2812 off (black) so it stays forced-off.
void ledStatusService();
