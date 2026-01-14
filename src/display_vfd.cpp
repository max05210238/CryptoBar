// CryptoBar Retro V0.99s - VFD Display Implementation
#include "display_vfd.h"
#include "app_state.h"
#include "coins.h"
#include "config.h"
#include <stdio.h>
#include <time.h>

// PT6302 connection (shared GPIO with E-ink)
#define VFD_CLK   EPD_SCK   // GPIO 12
#define VFD_RST   EPD_RST   // GPIO 16
#define VFD_CS    EPD_CS    // GPIO 10
#define VFD_DIN   11        // GPIO 11 (MOSI)

// Page rotation timing
#define PAGE_SWITCH_INTERVAL_MS 3000  // 3 seconds per page

// Brightness levels (0-255)
#define BRIGHTNESS_MORNING  127  // 50% (6:00-8:00)
#define BRIGHTNESS_DAY      178  // 70% (8:00-18:00)
#define BRIGHTNESS_EVENING  127  // 50% (18:00-22:00)
#define BRIGHTNESS_NIGHT     76  // 30% (22:00-2:00)
#define BRIGHTNESS_OFF        0  // Off (2:00-6:00)

// Constructor
DisplayVfd::DisplayVfd() {
  vfd = new PT6302(VFD_CLK, VFD_RST, VFD_CS, VFD_DIN);
  currentPage = 1;
  lastPageSwitch = 0;
  priceUpdateTime = 0;
  currentBrightness = BRIGHTNESS_DAY;
}

DisplayVfd::~DisplayVfd() {
  delete vfd;
}

// Initialize VFD
void DisplayVfd::init() {
  Serial.println("[VFD] Initializing PT6302 VFD display...");

  vfd->begin();
  delay(50);

  vfd->clear();
  vfd->setBrightness(currentBrightness);

  // Show boot message
  vfd->setCursor(0);
  vfd->print("CryptoBar Retro");
  delay(2000);

  vfd->clear();
  Serial.println("[VFD] Initialization complete");
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

  vfd->setCursor(0);
  vfd->print(buf);

  Serial.printf("[VFD] Page 1 (Price): %s\n", buf);
}

// Draw change page (Page 2)
void DisplayVfd::drawChangePage() {
  char buf[17];
  const CoinInfo& coin = coinAt(g_currentCoinIndex);

  formatChange(g_lastChange24h, coin.ticker, buf, 17);

  vfd->setCursor(0);
  vfd->print(buf);

  Serial.printf("[VFD] Page 2 (Change): %s\n", buf);
}

// Scroll-up animation (0.5 second, 8 frames)
void DisplayVfd::scrollUp() {
  // Simple scroll effect: shift content up
  // Frame 1-4: Current page shifts up gradually
  // Frame 5-8: New page shifts in from bottom

  const uint8_t frameCount = 8;
  const uint8_t frameDelay = 500 / frameCount;  // 0.5 sec / 8 frames = 62ms per frame

  // For simplicity, just do a quick clear and redraw
  // True pixel-level scrolling would require PT6302 custom character support
  for (uint8_t i = 0; i < frameCount; i++) {
    vfd->clear();
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
  uint8_t targetBrightness = BRIGHTNESS_DAY;

  // Determine brightness based on time of day
  if (hour >= 6 && hour < 8) {
    targetBrightness = BRIGHTNESS_MORNING;     // 50% (6:00-8:00)
  } else if (hour >= 8 && hour < 18) {
    targetBrightness = BRIGHTNESS_DAY;         // 70% (8:00-18:00)
  } else if (hour >= 18 && hour < 22) {
    targetBrightness = BRIGHTNESS_EVENING;     // 50% (18:00-22:00)
  } else if (hour >= 22 || hour < 2) {
    targetBrightness = BRIGHTNESS_NIGHT;       // 30% (22:00-2:00)
  } else {
    targetBrightness = BRIGHTNESS_OFF;         // 0% (2:00-6:00)
  }

  // Update brightness if changed
  if (targetBrightness != currentBrightness) {
    currentBrightness = targetBrightness;
    vfd->setBrightness(currentBrightness);
    Serial.printf("[VFD] Brightness: %d%% (hour=%d)\n", (currentBrightness * 100) / 255, hour);
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
        vfd->setBrightness(255);
        vfd->setCursor(0);
        vfd->print("████████████████");
        Serial.println("[VFD] Anti-burn-in: Full bright");
        break;

      case 1:
        // Full dark
        vfd->clear();
        Serial.println("[VFD] Anti-burn-in: Full dark");
        break;

      case 2:
        // Checkerboard A
        vfd->setBrightness(255);
        vfd->setCursor(0);
        vfd->print("█ █ █ █ █ █ █ █");
        Serial.println("[VFD] Anti-burn-in: Checkerboard A");
        break;

      case 3:
        // Checkerboard B
        vfd->setBrightness(255);
        vfd->setCursor(0);
        vfd->print(" █ █ █ █ █ █ █ ");
        Serial.println("[VFD] Anti-burn-in: Checkerboard B");
        break;
    }

    return;  // Don't show normal display during anti-burn-in
  }

  // 3:04-6:00: Turn off display
  if ((hour == 3 && minute >= 4) || (hour >= 4 && hour < 6)) {
    if (currentBrightness != BRIGHTNESS_OFF) {
      vfd->clear();
      vfd->setBrightness(BRIGHTNESS_OFF);
      currentBrightness = BRIGHTNESS_OFF;
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
  vfd->clear();
  vfd->setCursor(0);
  vfd->print("Menu            ");
}

void DisplayVfd::drawCoinList() {
  const CoinInfo& coin = coinAt(g_currentCoinIndex);
  char buf[17];
  snprintf(buf, 17, "Coin: %-10s", coin.ticker);
  vfd->setCursor(0);
  vfd->print(buf);
}

void DisplayVfd::drawCurrencyList() {
  char buf[17];
  snprintf(buf, 17, "Curr: %-10s", CURRENCY_INFO[g_displayCurrency].code);
  vfd->setCursor(0);
  vfd->print(buf);
}

void DisplayVfd::drawTimezoneList() {
  // VFD Retro doesn't show time, so timezone is less relevant
  // But still allow user to set it for scheduler alignment
  vfd->clear();
  vfd->setCursor(0);
  vfd->print("Timezone        ");
}

void DisplayVfd::drawSettingsScreen(const char* key, const char* value) {
  char buf[17];
  snprintf(buf, 17, "%-6s:%-9s", key, value);
  vfd->setCursor(0);
  vfd->print(buf);
}

void DisplayVfd::drawWifiSetupScreen(const char* ssid, const char* ip) {
  vfd->clear();
  vfd->setCursor(0);
  vfd->print("WiFi: Setup     ");
  delay(2000);

  if (ssid) {
    char buf[17];
    snprintf(buf, 17, "SSID:%-11s", ssid);
    vfd->setCursor(0);
    vfd->print(buf);
  }
}

void DisplayVfd::drawOtaScreen(const char* status) {
  char buf[17];
  snprintf(buf, 17, "OTA: %-11s", status);
  vfd->setCursor(0);
  vfd->print(buf);
}

void DisplayVfd::drawErrorScreen(const char* message) {
  char buf[17];
  snprintf(buf, 17, "ERR: %-11s", message);
  vfd->setCursor(0);
  vfd->print(buf);
}

void DisplayVfd::clear() {
  vfd->clear();
}

void DisplayVfd::sleep() {
  vfd->setBrightness(0);
  vfd->clear();
}

void DisplayVfd::wake() {
  vfd->setBrightness(currentBrightness);
}

void DisplayVfd::setBrightness(uint8_t level) {
  currentBrightness = level;
  vfd->setBrightness(level);
}
