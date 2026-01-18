// CryptoBar Retro V0.99s - VFD Display Implementation
// IGL VFD STUDIO 16-character display
#include "display_vfd.h"
#include "app_state.h"
#include "app_time.h"
#include "coins.h"
#include "config.h"
#include <stdio.h>
#include <time.h>

// VFD GPIO pins (shared with E-ink)
#define VFD_CLK   EPD_SCK   // GPIO 12
#define VFD_RST   EPD_RST   // GPIO 16
#define VFD_CS    EPD_CS    // GPIO 10
#define VFD_DIN   11        // GPIO 11 (MOSI)

// Page rotation timing
#define PAGE_SWITCH_INTERVAL_MS 3000  // 3 seconds per page

// Brightness levels (0-255)
#define BRIGHT_MORNING   128    // ~50% (6:00-8:00)
#define BRIGHT_DAY       220    // ~85% (8:00-18:00)
#define BRIGHT_EVENING   128    // ~50% (18:00-22:00)
#define BRIGHT_NIGHT      64    // ~25% (22:00-2:00)
#define BRIGHT_OFF         1    // Minimum (2:00-6:00)

// Constructor
DisplayVfd::DisplayVfd() {
  currentPage = 1;
  lastPageSwitch = 0;
  priceUpdateTime = 0;
  currentBrightness = BRIGHT_DAY;
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

// Format price for display
void DisplayVfd::formatPrice(double price, const char* coin, char* output, uint8_t maxLen) {
  // Format: "BTC   90651.3437"
  // Coin name (3 chars) + space + price (right-aligned in remaining space)

  char priceStr[13];

  // Determine decimal places based on price magnitude
  if (price >= 100000.0) {
    snprintf(priceStr, sizeof(priceStr), "%.2f", price);  // 6 digits + 2 decimals
  } else if (price >= 10000.0) {
    snprintf(priceStr, sizeof(priceStr), "%.3f", price);  // 5 digits + 3 decimals
  } else if (price >= 100.0) {
    snprintf(priceStr, sizeof(priceStr), "%.4f", price);  // 3+ digits + 4 decimals
  } else if (price >= 1.0) {
    snprintf(priceStr, sizeof(priceStr), "%.4f", price);  // 1-2 digits + 4 decimals
  } else {
    snprintf(priceStr, sizeof(priceStr), "%.4f", price);  // 0. + 4 decimals
  }

  // Build output: "BTC" + centered price
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

// Scroll-up animation (0.5 second)
void DisplayVfd::scrollUp() {
  // Simple scroll effect
  const uint8_t frameCount = 4;
  const uint8_t frameDelay = 500 / frameCount;  // 0.5 sec / 4 frames = 125ms per frame

  for (uint8_t i = 0; i < frameCount; i++) {
    vfdClear();
    delay(frameDelay);
  }

  Serial.println("[VFD] Scroll-up animation complete");
}

// Update page rotation (called in loop)
void DisplayVfd::updatePageRotation() {
  // Skip rotation if price data not available
  if (!g_lastPriceOk) {
    return;
  }

  // Check if 3 seconds have elapsed since last page switch
  uint32_t now = millis();
  if (now - lastPageSwitch >= PAGE_SWITCH_INTERVAL_MS) {
    // Switch page
    uint8_t nextPage = (currentPage == 1) ? 2 : 1;

    Serial.printf("[VFD] Switching page %d → %d\n", currentPage, nextPage);

    // Play scroll animation
    scrollUp();

    // Update page
    currentPage = nextPage;
    lastPageSwitch = now;

    // Draw new page
    if (currentPage == 1) {
      drawPricePage();
    } else {
      drawChangePage();
    }
  }
}

// Apply time-based brightness control
void DisplayVfd::applyTimeBrightness() {
  struct tm local;
  if (!getLocalTimeLocal(&local)) {
    return;  // Time not available
  }

  uint8_t hour = local.tm_hour;
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

// Night mode with anti-burn-in (3:00-6:00)
void DisplayVfd::runNightMode() {
  struct tm local;
  if (!getLocalTimeLocal(&local)) {
    return;
  }

  uint8_t hour = local.tm_hour;
  uint8_t minute = local.tm_min;

  // 3:00-3:04: Anti-burn-in full-screen refresh
  if (hour == 3 && minute < 4) {
    Serial.println("[VFD] Night mode: Anti-burn-in refresh");

    uint8_t patternIndex = minute;  // 0-3

    switch (patternIndex) {
      case 0:
        // Full bright
        vfdSetBrightness(255);
        vfdCommand(0xE9);  // All segments ON
        Serial.println("[VFD] Anti-burn-in: Full bright");
        break;

      case 1:
        // Full dark
        vfdClear();
        Serial.println("[VFD] Anti-burn-in: Full dark");
        break;

      case 2:
        // Pattern A
        vfdSetBrightness(255);
        vfdWriteStr(0, "****************");
        Serial.println("[VFD] Anti-burn-in: Pattern A");
        break;

      case 3:
        // Pattern B
        vfdSetBrightness(255);
        vfdWriteStr(0, "8888888888888888");
        Serial.println("[VFD] Anti-burn-in: Pattern B");
        break;
    }

    return;  // Don't show normal display during anti-burn-in
  }

  // 3:04-6:00: Turn off display
  if ((hour == 3 && minute >= 4) || (hour >= 4 && hour < 6)) {
    if (currentBrightness != BRIGHT_OFF) {
      vfdClear();
      vfdSetBrightness(BRIGHT_OFF);
      Serial.println("[VFD] Night mode: Display off (3:04-6:00)");
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

// Menu screens (simplified for VFD)
void DisplayVfd::drawMenuScreen() {
  vfdClear();
  vfdWriteStr(0, "Menu            ");
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
