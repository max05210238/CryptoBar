// CryptoBar V0.98-rc4 (Refactored: Step 4)
// app_input.h - Input handling (encoder/button events)

#ifndef APP_INPUT_H
#define APP_INPUT_H

// ==================== Encoder / Button Event Handlers =====================

// Poll encoder button state and detect press events
void pollEncoder();

// Handle short button press (30-800ms)
void handleShortPress();

// Handle long button press (800-12000ms)
void handleLongPress();

// Handle factory reset (12+ seconds)
void handleFactoryReset();

#endif // APP_INPUT_H
