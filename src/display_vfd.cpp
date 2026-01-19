// CryptoBar Retro V0.99s - VFD Display Implementation
// IGL VFD STUDIO 16-character display
#include "display_vfd.h"
#include "app_state.h"
#include "app_time.h"
#include "coins.h"
#include "config.h"
#include "vfd_font_5x7.h"
#include <stdio.h>
#include <time.h>
#include <cmath>  // For floor() and log10()

// External global variables (for menu display)
extern int g_menuIndex;
extern int g_updatePresetIndex;
extern int g_brightnessPresetIndex;
extern int g_vfdBrightnessPresetIndex;
extern int g_displayCurrency;
extern int g_timezoneIndex;
extern int g_currentCoinIndex;
extern const char* UPDATE_PRESET_LABELS[];
extern const char* BRIGHTNESS_LABELS[];
extern const uint8_t VFD_BRIGHTNESS_PRESETS[];
extern const char* VFD_BRIGHTNESS_LABELS[];

// VFD GPIO pins (shared with E-ink)
#define VFD_CLK   EPD_SCK   // GPIO 12
#define VFD_RST   EPD_RST   // GPIO 16
#define VFD_CS    EPD_CS    // GPIO 10
#define VFD_DIN   11        // GPIO 11 (MOSI)

// Page rotation timing (10-second cycle for NTP sync)
// 4.5s price + 0.5s animation + 4.5s change% + 0.5s animation = 10s
#define PAGE_SWITCH_INTERVAL_MS 5000  // 5 seconds per page (half cycle)

// Brightness levels (0-255)
#define BRIGHT_MORNING   128    // ~50% (6:00-8:00)
#define BRIGHT_DAY       220    // ~85% (8:00-18:00)
#define BRIGHT_EVENING   128    // ~50% (18:00-22:00)
#define BRIGHT_NIGHT      64    // ~25% (22:00-2:00)
#define BRIGHT_NIGHT_TEMP 128   // ~50% (temporary boost during 22:00-2:00)
#define BRIGHT_SLEEP_TEMP  64   // ~25% (temporary boost during 2:00-6:00)
#define BRIGHT_OFF         1    // Minimum (2:00-6:00)

// Constructor
DisplayVfd::DisplayVfd() {
  currentPage = 1;
  lastPageSwitch = 0;
  priceUpdateTime = 0;
  currentBrightness = BRIGHT_DAY;
  tempBrightBoostEndTime = 0;  // No temp boost initially
  savedBrightness = BRIGHT_DAY;
  memset(lastPageContent, 0, sizeof(lastPageContent));
}

DisplayVfd::~DisplayVfd() {
  // Nothing to clean up
}

// ===== Low-level VFD communication =====

// Write a single byte via software SPI
void DisplayVfd::vfdWriteByte(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(VFD_CLK, LOW);
    delayMicroseconds(2);  // ESP32 needs delay for 0.5MHz SPI clock

    if (data & 0x01) {
      digitalWrite(VFD_DIN, HIGH);
    } else {
      digitalWrite(VFD_DIN, LOW);
    }

    data >>= 1;
    delayMicroseconds(2);
    digitalWrite(VFD_CLK, HIGH);
    delayMicroseconds(2);
  }
}

// Send a command to VFD
void DisplayVfd::vfdCommand(uint8_t cmd) {
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(cmd);
  digitalWrite(VFD_CS, HIGH);
  delayMicroseconds(5);
}

// Show command (update display)
void DisplayVfd::vfdShow() {
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0xE8);  // Display ON command
  digitalWrite(VFD_CS, HIGH);
  delayMicroseconds(5);
}

// Initialize VFD
void DisplayVfd::vfdInit() {
  Serial.println("[VFD] Initializing IGL VFD display...");

  // Set digit count to 16
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0xE0);
  delayMicroseconds(5);
  vfdWriteByte(0x0F);  // 0x0F = 16 digits
  digitalWrite(VFD_CS, HIGH);
  delayMicroseconds(5);

  Serial.println("[VFD] Set digit count: 16");

  // Set brightness to maximum for testing
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0xE4);
  delayMicroseconds(5);
  vfdWriteByte(0xFF);  // 0xFF = max brightness
  digitalWrite(VFD_CS, HIGH);
  delayMicroseconds(5);

  Serial.println("[VFD] Set brightness: 255 (max)");
}

// Clear display
void DisplayVfd::vfdClear() {
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0x20);  // Address 0
  for (uint8_t i = 0; i < 16; i++) {
    vfdWriteByte(' ');  // Write spaces
  }
  digitalWrite(VFD_CS, HIGH);
  vfdShow();
}

// Write string at position
void DisplayVfd::vfdWriteStr(uint8_t pos, const char* str) {
  if (pos > 15) return;

  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0x20 + pos);  // Set cursor position

  while (*str && pos < 16) {
    vfdWriteByte(*str);
    str++;
    pos++;
  }

  digitalWrite(VFD_CS, HIGH);
  vfdShow();
}

// Set brightness (0-255)
void DisplayVfd::vfdSetBrightness(uint8_t level) {
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0xE4);
  delayMicroseconds(5);
  vfdWriteByte(level);
  digitalWrite(VFD_CS, HIGH);
  delayMicroseconds(5);

  currentBrightness = level;
}

// ===== DisplayInterface implementation =====

void DisplayVfd::init() {
  Serial.println("[VFD] === IGL VFD Display Initialization ===");

  // Step 1: Setup GPIO pins
  Serial.println("[VFD] Step 1: Setting up GPIO pins...");
  pinMode(VFD_CLK, OUTPUT);
  pinMode(VFD_CS, OUTPUT);
  pinMode(VFD_DIN, OUTPUT);
  pinMode(VFD_RST, OUTPUT);

  digitalWrite(VFD_CS, HIGH);   // CS high (inactive)
  digitalWrite(VFD_CLK, HIGH);  // CLK high
  digitalWrite(VFD_DIN, LOW);   // Data low
  digitalWrite(VFD_RST, HIGH);  // RST high (inactive)
  delay(100);

  // Step 2: Reset VFD (quick pulse)
  Serial.println("[VFD] Step 2: Resetting VFD...");
  digitalWrite(VFD_RST, LOW);
  delayMicroseconds(5);
  digitalWrite(VFD_RST, HIGH);
  delay(100);

  // Step 3: Initialize VFD
  Serial.println("[VFD] Step 3: Sending initialization commands...");
  vfdInit();

  // Step 4: Test with ALL-ON mode
  Serial.println("[VFD] Step 4: Testing ALL-ON mode...");
  vfdCommand(0xE9);  // All segments ON
  delay(2000);
  Serial.println("[VFD] If VFD is working, all segments should be lit now!");

  // Step 5: Clear display
  Serial.println("[VFD] Step 5: Clearing display...");
  vfdClear();
  delay(100);

  // Step 6: Show boot message
  Serial.println("[VFD] Step 6: Showing boot message...");
  vfdWriteStr(0, "CryptoBar Retro");
  delay(2000);

  // Step 7: Clear for ready state
  Serial.println("[VFD] Step 7: Ready!");
  vfdClear();
  delay(100);

  Serial.println("[VFD] === Initialization Complete ===");
  Serial.printf("[VFD] GPIO - CS:%d CLK:%d DIN:%d RST:%d\n", VFD_CS, VFD_CLK, VFD_DIN, VFD_RST);
}

// Center text in remaining space after prefix
void DisplayVfd::centerText(const char* text, char* output, uint8_t totalWidth, uint8_t prefixLen) {
  uint8_t textLen = strlen(text);
  uint8_t remainSpace = totalWidth - prefixLen;
  uint8_t leftPad = (remainSpace - textLen) / 2;

  // If can't center perfectly, shift right by 1
  if ((remainSpace - textLen) % 2 != 0) {
    leftPad++;
  }

  // Build output string
  uint8_t pos = prefixLen;
  for (uint8_t i = 0; i < leftPad; i++) {
    output[pos++] = ' ';
  }
  for (uint8_t i = 0; i < textLen && pos < totalWidth; i++) {
    output[pos++] = text[i];
  }
  while (pos < totalWidth) {
    output[pos++] = ' ';
  }
  output[totalWidth] = '\0';
}

// Format price for display (using same logic as E-ink)
void DisplayVfd::formatPrice(double price, const char* coin, char* output, uint8_t maxLen) {
  // Format: "BTC 90651.3437" or "BTC  1234.56" (coin + space + price)
  // VFD: 16 chars total, coin=3 chars, min 1 space, leaves 12 chars for price
  // E-ink logic: try 4 decimals → 2 → 0 based on total length

  // Calculate integer part digit count
  int intDigits;
  if (price < 1.0) {
    intDigits = 1;  // "0.xxxx"
  } else {
    intDigits = (int)floor(log10(price)) + 1;
  }

  // Try decimal places: 4 → 2 → 0 (same as E-ink)
  // Max price length: 12 chars (16 total - 3 coin - 1 space)
  const int decimals[] = {4, 2, 0};
  int chosenDecimals = 0;

  for (int i = 0; i < 3; i++) {
    int dec = decimals[i];
    int totalLen = intDigits + (dec > 0 ? 1 : 0) + dec;  // digits + decimal point + decimals

    if (totalLen <= 12) {
      chosenDecimals = dec;
      break;
    }
  }

  // Format price with chosen decimal places
  char priceStr[13];
  snprintf(priceStr, sizeof(priceStr), "%.*f", chosenDecimals, price);

  // Build output: "BTC" + centered price (with at least 1 space)
  snprintf(output, 4, "%-3s", coin);  // Left-align coin name (3 chars)
  centerText(priceStr, output, 16, 3);
}

// Format 24h change for display
void DisplayVfd::formatChange(double change, const char* coin, char* output, uint8_t maxLen) {
  // Format: "BTC  24H +0.29%"
  // Coin name (3 chars) + space + "24H" + space + change%

  char changeStr[11];
  snprintf(changeStr, sizeof(changeStr), "24H %+.2f%%", change);

  // Build output: "BTC" + centered change
  snprintf(output, 4, "%-3s", coin);  // Left-align coin name (3 chars)
  centerText(changeStr, output, 16, 3);
}

// Draw price page (Page 1)
void DisplayVfd::drawPricePage() {
  char buf[17];
  const CoinInfo& coin = coinAt(g_currentCoinIndex);

  // Apply display currency conversion
  double displayPrice = g_lastPriceUsd;
  if (g_displayCurrency != (int)CURR_USD && g_fxValid) {
    displayPrice *= g_usdToRate[g_displayCurrency];
  }

  formatPrice(displayPrice, coin.ticker, buf, 17);

  vfdWriteStr(0, buf);

  Serial.printf("[VFD] Page 1 (Price): %s\n", buf);
}

// Draw change page (Page 2)
void DisplayVfd::drawChangePage() {
  char buf[17];
  const CoinInfo& coin = coinAt(g_currentCoinIndex);

  formatChange(g_lastChange24h, coin.ticker, buf, 17);

  vfdWriteStr(0, buf);

  Serial.printf("[VFD] Page 2 (Change): %s\n", buf);
}

// ===== Pixel-level scrolling functions =====

// Write custom character to CGRAM slot (0-7)
// pixelData: 5 bytes (columns), each byte = 7 pixels
// Format: bit 0 = top pixel, bit 6 = bottom pixel
void DisplayVfd::writeCustomChar(uint8_t cgramSlot, const uint8_t* pixelData) {
  if (cgramSlot > 7) return;

  // Write to CGRAM address (0x40 + slot number)
  digitalWrite(VFD_CS, LOW);
  vfdWriteByte(0x40 + cgramSlot);

  // Write 5 columns of pixel data directly (already in VFD format)
  for (uint8_t col = 0; col < 5; col++) {
    vfdWriteByte(pixelData[col]);
  }

  digitalWrite(VFD_CS, HIGH);
  delayMicroseconds(5);
}

// Mix pixels from old and new character based on scroll offset
// offset: 0-7 (0=show old, 7=show new)
// Font format: bit 0 = top pixel, bit 6 = bottom pixel
void DisplayVfd::mixCharPixels(uint8_t* output, const uint8_t* oldChar, const uint8_t* newChar, uint8_t offset) {
  if (offset == 0) {
    // Show old character completely
    memcpy(output, oldChar, 5);
    return;
  }

  if (offset >= 7) {
    // Show new character completely
    memcpy(output, newChar, 5);
    return;
  }

  // Scroll UP: old text moves up (exits from top), new text enters from bottom
  // Format: bit 0 = top pixel, bit 6 = bottom pixel
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t oldPixels = oldChar[col];
    uint8_t newPixels = newChar[col];

    // Old text: right shift (pixels move upward, exit from top/bit 0)
    // offset=1: bits 1-6 → bits 0-5, bit 6 cleared
    uint8_t oldPart = oldPixels >> offset;

    // New text: take top 'offset' pixels (bits 0 to offset-1) and shift to bottom
    // offset=1: take bit 0 (top), shift to bit 6 (bottom)
    // offset=2: take bits 0-1, shift to bits 5-6
    uint8_t newMask = (1 << offset) - 1;
    uint8_t newPart = (newPixels & newMask) << (7 - offset);

    output[col] = oldPart | newPart;
  }
}

// Pixel-level vertical scroll (old text -> new text)
// Two-phase animation with proper CGRAM stabilization
void DisplayVfd::scrollUpPixelLevel(const char* oldText, const char* newText) {
  const uint8_t FRAME_COUNT = 8;  // 8 frames for 7 pixel rows
  const uint8_t frameDelay = SCROLL_DURATION_MS / FRAME_COUNT;

  char oldBuf[17], newBuf[17];
  snprintf(oldBuf, 17, "%-16s", oldText);  // Pad to 16 chars
  snprintf(newBuf, 17, "%-16s", newText);

  // === Phase 1: Animate left half (positions 0-7) ===
  for (uint8_t frame = 0; frame < FRAME_COUNT; frame++) {
    uint8_t offset = frame;  // 0 to 7

    // Update CGRAM 0-7 with mixed pixels for left half
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t oldCharBitmap[5], newCharBitmap[5];
      getCharBitmap(oldBuf[i], oldCharBitmap);
      getCharBitmap(newBuf[i], newCharBitmap);

      uint8_t mixed[5];
      mixCharPixels(mixed, oldCharBitmap, newCharBitmap, offset);
      writeCustomChar(i, mixed);
    }

    // Critical: Wait for CGRAM to fully stabilize
    delay(2);  // 2ms for CGRAM to settle

    // Display: left half CGRAM (animated), right half ASCII smooth transition
    digitalWrite(VFD_CS, LOW);
    vfdWriteByte(0x20);  // Start at position 0
    for (uint8_t i = 0; i < 16; i++) {
      if (i < 8) {
        vfdWriteByte(0x00 + i);  // Left: CGRAM (pixel-level animation)
      } else {
        // Right: smooth ASCII transition (no jumping)
        char displayChar = (offset < 4) ? oldBuf[i] : newBuf[i];
        vfdWriteByte(displayChar);
      }
    }
    digitalWrite(VFD_CS, HIGH);

    vfdShow();
    delay(frameDelay - 2);  // Subtract CGRAM stabilization time
  }

  // === Inter-phase stabilization: Clear CGRAM to prevent artifacts ===
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t blank[5] = {0, 0, 0, 0, 0};
    writeCustomChar(i, blank);
  }
  delay(5);  // Extra delay to ensure CGRAM is completely cleared

  // === Phase 2: Animate right half (positions 8-15) ===
  for (uint8_t frame = 0; frame < FRAME_COUNT; frame++) {
    uint8_t offset = frame;  // 0 to 7

    // Update CGRAM 0-7 with mixed pixels for right half
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t oldCharBitmap[5], newCharBitmap[5];
      getCharBitmap(oldBuf[8 + i], oldCharBitmap);
      getCharBitmap(newBuf[8 + i], newCharBitmap);

      uint8_t mixed[5];
      mixCharPixels(mixed, oldCharBitmap, newCharBitmap, offset);
      writeCustomChar(i, mixed);
    }

    // Critical: Wait for CGRAM to fully stabilize
    delay(2);  // 2ms for CGRAM to settle

    // Display: left half shows final text, right half CGRAM (animated)
    digitalWrite(VFD_CS, LOW);
    vfdWriteByte(0x20);  // Start at position 0
    for (uint8_t i = 0; i < 16; i++) {
      if (i < 8) {
        // Left: show final new text (already animated in phase 1)
        vfdWriteByte(newBuf[i]);
      } else {
        vfdWriteByte(0x00 + (i - 8));  // Right: CGRAM (pixel-level animation)
      }
    }
    digitalWrite(VFD_CS, HIGH);

    vfdShow();
    delay(frameDelay - 2);  // Subtract CGRAM stabilization time
  }

  // Final: display new text using normal ASCII (clear CGRAM usage)
  vfdWriteStr(0, newText);

  Serial.println("[VFD] Pixel-level scroll complete");
}

// Horizontal left scroll (character-level)
// Old screen slides out to left, new screen slides in from right
void DisplayVfd::scrollLeftCharLevel(const char* oldText, const char* newText) {
  const uint8_t DISPLAY_WIDTH = 16;
  const uint8_t FRAME_COUNT = DISPLAY_WIDTH + 1;  // 17 frames (0 to 16)
  const uint8_t frameDelay = SCROLL_DURATION_MS / FRAME_COUNT;

  char oldBuf[17], newBuf[17];
  snprintf(oldBuf, 17, "%-16s", oldText);  // Pad to 16 chars
  snprintf(newBuf, 17, "%-16s", newText);

  // Create a 32-character virtual buffer: [old text][new text]
  char virtualBuf[33];
  snprintf(virtualBuf, 33, "%s%s", oldBuf, newBuf);

  // Animate: slide viewing window from left to right across virtual buffer
  for (uint8_t frame = 0; frame <= DISPLAY_WIDTH; frame++) {
    char displayBuf[17];

    // Extract 16-character window starting at position 'frame'
    for (uint8_t i = 0; i < DISPLAY_WIDTH; i++) {
      displayBuf[i] = virtualBuf[frame + i];
    }
    displayBuf[DISPLAY_WIDTH] = '\0';

    // Display this frame
    vfdWriteStr(0, displayBuf);
    vfdShow();
    delay(frameDelay);
  }

  Serial.println("[VFD] Left scroll complete");
}

// Scroll-up animation (calls pixel-level implementation)
void DisplayVfd::scrollUp() {
  // This will be called with the old and new page content
  // For now, just clear (will be replaced by actual content in updatePageRotation)
  vfdClear();
  delay(SCROLL_DURATION_MS / 8);
  Serial.println("[VFD] Scroll-up animation (placeholder)");
}

// Update page rotation (called in loop)
// 10-second sync cycle: 4.5s price + 0.5s animation + 4.5s change% + 0.5s animation
void DisplayVfd::updatePageRotation() {
  // Skip rotation if price data not available
  if (!g_lastPriceOk) {
    return;
  }

  // Check if 5 seconds have elapsed since last page switch (half cycle)
  uint32_t now = millis();
  if (now - lastPageSwitch >= PAGE_SWITCH_INTERVAL_MS) {
    // Switch page
    uint8_t nextPage = (currentPage == 1) ? 2 : 1;

    Serial.printf("[VFD] Switching page %d → %d (pixel scroll)\n", currentPage, nextPage);

    // Generate new page content
    char newPageBuf[17];
    const CoinInfo& coin = coinAt(g_currentCoinIndex);

    if (nextPage == 1) {
      // Price page
      double displayPrice = g_lastPriceUsd;
      if (g_displayCurrency != (int)CURR_USD && g_fxValid) {
        displayPrice *= g_usdToRate[g_displayCurrency];
      }
      formatPrice(displayPrice, coin.ticker, newPageBuf, 17);
    } else {
      // Change page
      formatChange(g_lastChange24h, coin.ticker, newPageBuf, 17);
    }

    // Horizontal left scroll from old to new page
    if (lastPageContent[0] != '\0') {
      scrollLeftCharLevel(lastPageContent, newPageBuf);
    } else {
      // First display, no animation
      vfdWriteStr(0, newPageBuf);
    }

    // Cache new page content for next scroll
    snprintf(lastPageContent, 17, "%s", newPageBuf);

    // Update page state
    currentPage = nextPage;
    lastPageSwitch = now;

    Serial.printf("[VFD] Page %d content: %s\n", currentPage, newPageBuf);
  }
}

// Handle encoder activity (temporary brightness boost)
void DisplayVfd::handleEncoderActivity() {
  struct tm local;
  if (!getLocalTimeLocal(&local)) {
    return;  // Time not available
  }

  uint8_t hour = local.tm_hour;
  unsigned long now = millis();

  // Check if we're in night time or sleep time
  bool isNightTime = (hour >= 22 || hour < 2);   // 22:00-02:00
  bool isSleepTime = (hour >= 2 && hour < 6);    // 02:00-06:00

  if (isNightTime || isSleepTime) {
    // Activate temporary brightness boost
    uint8_t tempBright = isNightTime ? BRIGHT_NIGHT_TEMP : BRIGHT_SLEEP_TEMP;

    // Save current brightness if not already in temp boost mode
    if (tempBrightBoostEndTime == 0 || now > tempBrightBoostEndTime) {
      savedBrightness = currentBrightness;
    }

    tempBrightBoostEndTime = now + TEMP_BRIGHT_BOOST_DURATION_MS;
    vfdSetBrightness(tempBright);

    Serial.printf("[VFD] Temp brightness boost: %d/255 for 5 min (hour=%d)\n", tempBright, hour);
  }
}

// Apply time-based brightness control
void DisplayVfd::applyTimeBrightness() {
  struct tm local;
  if (!getLocalTimeLocal(&local)) {
    return;  // Time not available
  }

  uint8_t hour = local.tm_hour;
  unsigned long now = millis();

  // Check if temp brightness boost is active
  if (tempBrightBoostEndTime > 0 && now < tempBrightBoostEndTime) {
    // Temp boost is still active, don't change brightness
    return;
  }

  // Temp boost expired, restore normal brightness
  if (tempBrightBoostEndTime > 0 && now >= tempBrightBoostEndTime) {
    tempBrightBoostEndTime = 0;  // Clear temp boost flag
    Serial.println("[VFD] Temp brightness boost expired, restoring normal brightness");
  }

  uint8_t targetBright = BRIGHT_DAY;

  // Determine brightness based on time of day
  if (hour >= 6 && hour < 8) {
    targetBright = BRIGHT_MORNING;     // ~50% (6:00-8:00)
  } else if (hour >= 8 && hour < 18) {
    targetBright = BRIGHT_DAY;         // ~85% (8:00-18:00)
  } else if (hour >= 18 && hour < 22) {
    targetBright = BRIGHT_EVENING;     // ~50% (18:00-22:00)
  } else if (hour >= 22 || hour < 2) {
    targetBright = BRIGHT_NIGHT;       // ~25% (22:00-2:00)
  } else {
    targetBright = BRIGHT_OFF;         // Minimum (2:00-6:00)
  }

  // Update brightness if changed
  if (targetBright != currentBrightness) {
    vfdSetBrightness(targetBright);
    Serial.printf("[VFD] Brightness: %d/255 (hour=%d)\n", targetBright, hour);
  }
}

// Night mode with anti-burn-in (04:00-06:00)
void DisplayVfd::runNightMode() {
  struct tm local;
  if (!getLocalTimeLocal(&local)) {
    return;
  }

  uint8_t hour = local.tm_hour;
  uint8_t minute = local.tm_min;

  // 04:00-04:04: Anti-burn-in uniform aging (4 patterns × 1 minute each)
  if (hour == 4 && minute < 4) {
    Serial.println("[VFD] Night mode: Anti-burn-in uniform aging");

    uint8_t patternIndex = minute;  // 0-3

    switch (patternIndex) {
      case 0:
        // Full bright - all pixels ON
        vfdSetBrightness(255);
        vfdCommand(0xE9);  // All segments ON
        Serial.println("[VFD] Anti-burn-in aging: Full bright (minute 0)");
        break;

      case 1:
        // Full dark - all pixels OFF
        vfdClear();
        vfdSetBrightness(255);
        Serial.println("[VFD] Anti-burn-in aging: Full dark (minute 1)");
        break;

      case 2:
        // Checkerboard pattern
        vfdSetBrightness(255);
        vfdWriteStr(0, "* * * * * * * * ");
        Serial.println("[VFD] Anti-burn-in aging: Checkerboard (minute 2)");
        break;

      case 3:
        // All segments pattern (8)
        vfdSetBrightness(255);
        vfdWriteStr(0, "8888888888888888");
        Serial.println("[VFD] Anti-burn-in aging: All segments (minute 3)");
        break;
    }

    return;  // Don't show normal display during anti-burn-in
  }

  // 02:00-04:00 and 04:04-06:00: Turn off display
  if ((hour >= 2 && hour < 4) || (hour == 4 && minute >= 4) || (hour == 5)) {
    if (currentBrightness != BRIGHT_OFF) {
      vfdClear();
      vfdSetBrightness(BRIGHT_OFF);
      Serial.println("[VFD] Night mode: Display off (02:00-06:00)");
    }
    return;
  }
}

// Main screen drawing
void DisplayVfd::drawMainScreen(bool forceFullRefresh) {
  // Check night mode first
  runNightMode();

  // Apply time-based brightness
  applyTimeBrightness();

  // Update page rotation
  updatePageRotation();
}

// Time-only update (VFD doesn't show time, so this is a no-op)
void DisplayVfd::drawMainScreenTimeOnly(bool forceFullRefresh) {
  // VFD Retro doesn't display time, skip
}

// ===== Menu helper functions =====

// Get menu item text string
void DisplayVfd::getMenuItemText(VfdMenuItem item, char* output, uint8_t maxLen) {
  switch (item) {
    case VFD_MENU_COIN:
      snprintf(output, maxLen, "Coin: %s", coinAt(g_currentCoinIndex).ticker);
      break;
    case VFD_MENU_UPDATE:
      snprintf(output, maxLen, "Update: %s", UPDATE_PRESET_LABELS[g_updatePresetIndex]);
      break;
    case VFD_MENU_LED_BRIGHTNESS:
      snprintf(output, maxLen, "LED: %s", BRIGHTNESS_LABELS[g_brightnessPresetIndex]);
      break;
    case VFD_MENU_VFD_BRIGHTNESS:
      snprintf(output, maxLen, "Bright: %s", VFD_BRIGHTNESS_LABELS[g_vfdBrightnessPresetIndex]);
      break;
    case VFD_MENU_CURRENCY:
      snprintf(output, maxLen, "Currency: %s", CURRENCY_INFO[g_displayCurrency].code);
      break;
    case VFD_MENU_TIMEZONE:
      snprintf(output, maxLen, "Timezone: %s", TIMEZONES[g_timezoneIndex].label);
      break;
    case VFD_MENU_FIRMWARE:
      snprintf(output, maxLen, "Firmware Update");
      break;
    case VFD_MENU_WIFI_INFO:
      snprintf(output, maxLen, "WiFi Info");
      break;
    case VFD_MENU_EXIT:
      snprintf(output, maxLen, "Exit");
      break;
    default:
      snprintf(output, maxLen, "Unknown");
      break;
  }
}

// Draw menu item with auto-scroll if text > 14 chars
void DisplayVfd::drawMenuItemText(VfdMenuItem item) {
  char text[64];  // Buffer for menu item text
  getMenuItemText(item, text, sizeof(text));

  uint8_t textLen = strlen(text);

  // If text fits in 16 chars, display directly
  if (textLen <= 16) {
    char buf[17];
    snprintf(buf, 17, "%-16s", text);
    vfdWriteStr(0, buf);
    return;
  }

  // Text is too long, auto-scroll:
  // 1. Stop 1s
  // 2. Scroll left until end is visible
  // 3. Stop at end 1s
  // 4. Loop (optional, for now just show start)

  // For simplicity, show first 16 chars (we can enhance scrolling later)
  char buf[17];
  snprintf(buf, 17, "%-16.16s", text);  // Truncate to 16 chars for now
  vfdWriteStr(0, buf);

  // TODO: Implement full auto-scroll (stop 1s, scroll left, stop 1s, loop)
}

// Menu screens (VFD-specific 9-item menu)
void DisplayVfd::drawMenuScreen() {
  // Display current menu item (VFD can only show 1 line at a time)
  if (g_menuIndex < 0 || g_menuIndex >= VFD_MENU_COUNT) {
    g_menuIndex = 0;
  }

  VfdMenuItem item = static_cast<VfdMenuItem>(g_menuIndex);
  drawMenuItemText(item);

  Serial.printf("[VFD] Menu: %d\n", g_menuIndex);
}

void DisplayVfd::drawCoinList() {
  const CoinInfo& coin = coinAt(g_currentCoinIndex);
  char buf[17];
  snprintf(buf, 17, "Coin: %-10s", coin.ticker);
  vfdWriteStr(0, buf);
}

void DisplayVfd::drawCurrencyList() {
  char buf[17];
  snprintf(buf, 17, "Curr: %-10s", CURRENCY_INFO[g_displayCurrency].code);
  vfdWriteStr(0, buf);
}

void DisplayVfd::drawTimezoneList() {
  vfdClear();
  vfdWriteStr(0, "Timezone        ");
}

void DisplayVfd::drawSettingsScreen(const char* key, const char* value) {
  char buf[17];
  snprintf(buf, 17, "%-6s:%-9s", key, value);
  vfdWriteStr(0, buf);
}

void DisplayVfd::drawWifiSetupScreen(const char* ssid, const char* ip) {
  Serial.println("[VFD] drawWifiSetupScreen called");
  Serial.printf("[VFD] SSID: %s, IP: %s\n", ssid ? ssid : "null", ip ? ip : "null");

  vfdClear();
  delay(50);

  if (ssid && strlen(ssid) > 0) {
    char buf[17];
    snprintf(buf, 17, "%-16s", ssid);
    Serial.printf("[VFD] Printing SSID: '%s'\n", buf);
    vfdWriteStr(0, buf);
  } else {
    Serial.println("[VFD] Printing 'WiFi: Setup'");
    vfdWriteStr(0, "WiFi: Setup     ");
  }

  Serial.println("[VFD] drawWifiSetupScreen complete");
}

void DisplayVfd::drawOtaScreen(const char* status) {
  char buf[17];
  snprintf(buf, 17, "OTA: %-11s", status);
  vfdWriteStr(0, buf);
}

void DisplayVfd::drawErrorScreen(const char* message) {
  Serial.println("[VFD] drawErrorScreen called");
  Serial.printf("[VFD] Error message: %s\n", message ? message : "null");

  char buf[17];
  snprintf(buf, 17, "ERR: %-11s", message ? message : "Unknown");
  Serial.printf("[VFD] Printing: '%s'\n", buf);
  vfdWriteStr(0, buf);

  Serial.println("[VFD] drawErrorScreen complete");
}

void DisplayVfd::clear() {
  vfdClear();
}

void DisplayVfd::sleep() {
  vfdSetBrightness(BRIGHT_OFF);
  vfdClear();
}

void DisplayVfd::wake() {
  vfdSetBrightness(currentBrightness);
}

void DisplayVfd::setBrightness(uint8_t level) {
  vfdSetBrightness(level);
}
