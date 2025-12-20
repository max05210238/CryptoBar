#include <Arduino.h>
#include <time.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

#include "SpaceGroteskBold24pt7b.h"

#include "coins.h"
#include "chart.h"
#include "ui.h"

// ===== Global objects and variables from main.cpp (extern declarations) =====

// e-paper display object
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// Layout: left symbol panel width
extern const int SYMBOL_PANEL_WIDTH;

// Settings state
extern int   g_dateFormatIndex;
extern int   g_timeFormat;
extern int   g_dtSize; // 0=Small, 1=Large

// Large mode: shift right-side content down slightly for comfortable date/time spacing.
// Note: This offset affects dt/price/chart together to maintain consistent spacing.
static inline int16_t largeContentYOffset() {
  return (g_dtSize == 1) ? 8 : 0;
}
extern int   g_timezoneIndex;
#include "day_avg.h"
extern uint8_t g_dayAvgMode;

// ✅ NEW: refresh mode
extern int g_refreshMode;
extern int g_displayCurrency;
extern float g_usdToTwd;
extern bool g_fxValid;
extern const char* REFRESH_MODE_LABELS[];

// Previous day average reference line
extern float g_prevDayRefPrice;
extern bool  g_prevDayRefValid;

// Chart samples
extern ChartSample g_chartSamples[MAX_CHART_SAMPLES];
extern int         g_chartSampleCount;

// Menu / timezone submenu index
extern int g_menuIndex;
extern int g_menuTopIndex;
extern int g_tzMenuIndex;
extern int g_tzMenuTopIndex;

// Other settings labels / indices
extern int   g_brightnessPresetIndex;
extern int   g_updatePresetIndex;
extern int   g_currentCoinIndex;
extern const char* BRIGHTNESS_LABELS[];
extern const char* UPDATE_PRESET_LABELS[];
extern const char* DATE_FORMAT_LABELS[];

// Helper functions implemented in main.cpp
bool getLocalTimeLocal(struct tm* out);
const CoinInfo& currentCoin();

// ===== Constants used in this file =====

// ===== Date/Time formatting =====

static void formatDateString(char* buf, size_t bufSize, const struct tm& local) {
  switch (g_dateFormatIndex) {
    case DATE_DD_MM_YYYY:
      strftime(buf, bufSize, "%d/%m/%Y", &local);
      break;
    case DATE_YYYY_MM_DD:
      strftime(buf, bufSize, "%Y-%m-%d", &local);
      break;
    case DATE_MM_DD_YYYY:
    default:
      strftime(buf, bufSize, "%m/%d/%Y", &local);
      break;
  }
}

static void formatTimeString(char* timeBuf, size_t timeSize,
                             char* ampmBuf, size_t ampmSize,
                             const struct tm& local) {
  if (g_timeFormat == TIME_24H) {
    strftime(timeBuf, timeSize, "%H:%M", &local);
    ampmBuf[0] = '\0';
  } else {
    int hour = local.tm_hour;
    bool isPM = (hour >= 12);
    int displayHour = hour % 12;
    if (displayHour == 0) displayHour = 12;
    snprintf(timeBuf, timeSize, "%2d:%02d", displayHour, local.tm_min);
    snprintf(ampmBuf, ampmSize, "%s", isPM ? "PM" : "AM");
  }
}

// Small date/time display (top-left small text)
// Note: Small/Large both respect date/time format settings; differ only in font and position
static void drawHeaderDateTimeSmall() {
  char dateBuf[20] = "--/--/----";
  char timeBuf[16] = "--:--";
  char ampmBuf[4]  = "";
  char fullTimeBuf[24] = "--:--";   // time + optional AM/PM

  struct tm local;
  if (getLocalTimeLocal(&local)) {
    formatDateString(dateBuf, sizeof(dateBuf), local);
    formatTimeString(timeBuf, sizeof(timeBuf), ampmBuf, sizeof(ampmBuf), local);
  }

 // Combine time string with AM/PM
  if (ampmBuf[0]) {
    snprintf(fullTimeBuf, sizeof(fullTimeBuf), "%s %s", timeBuf, ampmBuf);
  } else {
    snprintf(fullTimeBuf, sizeof(fullTimeBuf), "%s", timeBuf);
  }

  int panelLeft  = SYMBOL_PANEL_WIDTH;
  int panelRight = display.width();

  display.setFont();       // default 6x8
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);

  int16_t x1, y1;
  uint16_t w, h;

  const int16_t yBase       = 2;   // yBase=2 as previously specified
  const int16_t leftMargin  = 2;
  const int16_t rightMargin = 2;

 // Date: top-left
  display.getTextBounds(dateBuf, 0, 0, &x1, &y1, &w, &h);
  int16_t xDate = panelLeft + leftMargin;
  display.setCursor(xDate, yBase);
  display.print(dateBuf);

 // Time: top-right (includes AM/PM)
  display.getTextBounds(fullTimeBuf, 0, 0, &x1, &y1, &w, &h);
  int16_t xTime = panelRight - rightMargin - w;
  display.setCursor(xTime, yBase);
  display.print(fullTimeBuf);
}

// Centered large date/time (Large mode)
// Goal: equal spacing between date/time ↔ price ↔ chart; date/time can be slightly compressed near top edge of white panel
static void drawHeaderDateTimeLarge() {
  const int16_t yOff = largeContentYOffset();
  char dateBuf[20] = "--/--/----";
  char timeBuf[16] = "--:--";
  char ampmBuf[4]  = "";
  char fullTimeBuf[24] = "--:--";
  char dtBuf[64] = "--/--/---- --:--";

  struct tm local;
  if (getLocalTimeLocal(&local)) {
    formatDateString(dateBuf, sizeof(dateBuf), local);
    formatTimeString(timeBuf, sizeof(timeBuf), ampmBuf, sizeof(ampmBuf), local);
  }

  if (ampmBuf[0]) {
    snprintf(fullTimeBuf, sizeof(fullTimeBuf), "%s %s", timeBuf, ampmBuf);
  } else {
    snprintf(fullTimeBuf, sizeof(fullTimeBuf), "%s", timeBuf);
  }
  snprintf(dtBuf, sizeof(dtBuf), "%s %s", dateBuf, fullTimeBuf);

  const int panelLeft  = SYMBOL_PANEL_WIDTH;
  const int panelRight = display.width();
  const int panelWidth = panelRight - panelLeft;

 // Large mode: fixed 9pt font (12pt would be too large)
  const GFXfont* dtFont = &FreeSansBold9pt7b;
  display.setFont(dtFont);
  display.setTextColor(GxEPD_BLACK);

  // V0.99c: Get date/time text bounds once (will reuse later, avoiding duplicate call)
  int16_t dtX1, dtY1;
  uint16_t dtW, dtH;
  display.getTextBounds(dtBuf, 0, 0, &dtX1, &dtY1, &dtW, &dtH);

 // 9pt still needs full centering; if string becomes too wide (rare), fall back to side margins
  const int minSideMargin = 2;
  if ((int)dtW > (panelWidth - minSideMargin * 2)) {
 // Don't reduce font size; use available width as centering baseline (prevent overflow from white panel)
 // (x will be recalculated later)
  }

 // These two values must match the layout in drawPriceCenter / drawHistoryChart
  const int16_t PRICE_Y_BASE = 52 + yOff;
  const int16_t CHART_TOP    = 70 + yOff;

 // Calculate price text area height (use large font bbox only, avoid modifying drawPriceCenter itself)
  display.setFont(&FreeSansBold18pt7b);
  int16_t px1, py1;
  uint16_t pw, ph;
 // Use full-width characters to estimate price font height, making dt↔price↔chart spacing visually accurate
  display.getTextBounds("88888", 0, 0, &px1, &py1, &pw, &ph);

  int16_t priceTop    = PRICE_Y_BASE + py1;
  int16_t priceBottom = priceTop + (int16_t)ph;

 // Restore to 9pt font for date/time rendering (V0.99c: reuse cached dtBuf bounds from line 186)
  display.setFont(dtFont);

  int16_t gap = CHART_TOP - priceBottom;  // Price area bottom → chart top
  if (gap < 2) gap = 2;

 // Distance from date/time bottom → price top should equal gap
  int16_t dtBottom = priceTop - gap;

 // V0.99c: Reuse cached dtX1, dtY1, dtW, dtH from line 190 (removed redundant getTextBounds call)
  int16_t dtBaseline = dtBottom - (dtY1 + (int16_t)dtH);

 // Allow slight compression at top edge, but stay within bounds
  const int16_t minTopMargin = 2;
  int16_t dtTop = dtBaseline + dtY1;
  if (dtTop < minTopMargin) {
    dtBaseline += (minTopMargin - dtTop);
  }

 // Horizontal centering (centered within white panel area only)
  int16_t x = panelLeft + (panelWidth - (int)dtW) / 2 - dtX1;
  display.setCursor(x, dtBaseline);
  display.print(dtBuf);
}

// Header wrapper: switch between Small / Large based on dtSize
static void drawHeaderDateTime() {
  if (g_dtSize == 1) {
    drawHeaderDateTimeLarge();
  } else {
    drawHeaderDateTimeSmall();
  }
}

// Left black panel: coin symbol + 24h change
static void drawSymbolPanel(const char* symbol, float change24h) {
  int panelWidth = SYMBOL_PANEL_WIDTH;

  display.fillRect(0, 0, panelWidth, display.height(), GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);

  const GFXfont* bigFont   = &FreeSansBold18pt7b;
  const GFXfont* smallFont = &FreeSansBold9pt7b;

  // Coin symbol bounds
  display.setFont(bigFont);
  int16_t sx1, sy1;
  uint16_t sW, sH;
  display.getTextBounds(symbol, 0, 0, &sx1, &sy1, &sW, &sH);

  // Change percentage bounds
  char changeBuf[24];
  snprintf(changeBuf, sizeof(changeBuf), "%+.2f%%", change24h);

  display.setFont(smallFont);
  int16_t cx1, cy1;
  uint16_t cW, cH;
  display.getTextBounds(changeBuf, 0, 0, &cx1, &cy1, &cW, &cH);

  // Currency symbol bounds (V0.99f)
  const CurrencyInfo& curr = CURRENCY_INFO[g_displayCurrency];
  char currBuf[16];
  snprintf(currBuf, sizeof(currBuf), "%s-%s", curr.symbol, curr.code);

  int16_t currX1, currY1;
  uint16_t currW, currH;
  display.getTextBounds(currBuf, 0, 0, &currX1, &currY1, &currW, &currH);

  // Calculate vertical layout: [Symbol] [Change] [Currency]
  int totalH = sH + 4 + cH + 4 + currH;
  int topY   = (display.height() - totalH) / 2;

  // Draw coin symbol
  int16_t sy = topY + sH;
  int16_t sx = (panelWidth - sW) / 2;
  display.setFont(bigFont);
  display.setCursor(sx, sy);
  display.print(symbol);

  // Draw change percentage
  int16_t cy = sy + 4 + cH;
  int16_t cx = (panelWidth - cW) / 2;
  display.setFont(smallFont);
  display.setCursor(cx, cy);
  display.print(changeBuf);

  // Draw currency symbol (V0.99f)
  int16_t currY = cy + 4 + currH;
  int16_t currX = (panelWidth - currW) / 2;
  display.setCursor(currX, currY);
  display.print(currBuf);

  display.setTextColor(GxEPD_BLACK);
}

// V0.99f: Center price display with multi-currency support
static void drawPriceCenter(float priceUsd) {
  int panelLeft  = SYMBOL_PANEL_WIDTH;
  int panelWidth = display.width() - panelLeft;

  // Convert for display if needed
  float fx = 1.0f;
  if (g_displayCurrency != (int)CURR_USD && g_displayCurrency < (int)CURR_COUNT) {
    fx = g_usdToRate[g_displayCurrency];
  }
  float price = priceUsd * fx;

  // Get currency metadata
  const CurrencyInfo& curr = CURRENCY_INFO[g_displayCurrency];

  // Determine max decimals based on currency type
  int maxDecimals = curr.noDecimals ? 0 : 4;

  // Big font for number (may downgrade to 12pt if needed)
  const GFXfont* numFont = &FreeSansBold18pt7b;
  display.setFont(numFont);
  display.setTextColor(GxEPD_BLACK);

  // Reserve a prefix slot equal to '$' width (big font)
  int16_t x1, y1;
  uint16_t wDollar, hDollar;
  display.getTextBounds("$", 0, 0, &x1, &y1, &wDollar, &hDollar);

  const int gap = 4;
  int maxNumberW = panelWidth - (int)wDollar - gap;
  if (maxNumberW < 10) maxNumberW = 10;

  // Adaptive decimals: start at maxDecimals, reduce until it fits
  char numBuf[32];
  uint16_t wNum = 0, hNum = 0;
  bool needDowngrade = false;

  for (int dec = maxDecimals; dec >= 0; --dec) {
    snprintf(numBuf, sizeof(numBuf), "%.*f", dec, price);
    display.getTextBounds(numBuf, 0, 0, &x1, &y1, &wNum, &hNum);
    if ((int)wNum <= maxNumberW) break;

    // If we've reached 0 decimals and still doesn't fit, need to downgrade font
    if (dec == 0 && (int)wNum > maxNumberW) {
      needDowngrade = true;
    }
  }

  // V0.99f: Downgrade to 12pt font only if necessary (>9 digits with 0 decimals)
  if (needDowngrade) {
    numFont = &FreeSansBold12pt7b;
    display.setFont(numFont);

    // Retry formatting with smaller font
    for (int dec = maxDecimals; dec >= 0; --dec) {
      snprintf(numBuf, sizeof(numBuf), "%.*f", dec, price);
      display.getTextBounds(numBuf, 0, 0, &x1, &y1, &wNum, &hNum);
      if ((int)wNum <= maxNumberW) break;
    }
    Serial.printf("[UI] Price downgraded to 12pt font: %s\n", numBuf);
  }

  // Reset to 18pt for symbol measurement
  display.setFont(&FreeSansBold18pt7b);

  int totalW = (int)wDollar + gap + (int)wNum;
  int16_t xStart = panelLeft + (panelWidth - totalW) / 2;
  int16_t yBase  = 52 + largeContentYOffset();

  // Draw currency symbol/prefix
  if (curr.twoCharSym) {
    // Two-character symbols: NT, C$, S$, A$ (use 12pt, compressed)
    display.setFont(&FreeSansBold12pt7b);

    const char* sym = curr.symbol;
    char ch1[2] = {sym[0], '\0'};
    char ch2[2] = {sym[1], '\0'};

    uint16_t w1, w2, h1, h2;
    display.getTextBounds(ch1, 0, 0, &x1, &y1, &w1, &h1);
    display.getTextBounds(ch2, 0, 0, &x1, &y1, &w2, &h2);

    int overlap = 2;
    int symW = (int)w1 + (int)w2 - overlap;
    if (symW > (int)wDollar) overlap += (symW - (int)wDollar);
    if (overlap > (int)w1 - 1) overlap = (int)w1 - 1;
    symW = (int)w1 + (int)w2 - overlap;

    int16_t symX = xStart + ((int)wDollar - symW) / 2;
    display.setCursor(symX, yBase);
    display.print(ch1);
    display.setCursor(symX + (int)w1 - overlap, yBase);
    display.print(ch2);

  } else {
    // Single-character symbols: $, €, £, ¥, ₩ (use 18pt)
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(xStart, yBase);
    display.print(curr.symbol);
  }

  // Draw number
  display.setFont(numFont);  // Use potentially downgraded font
  display.setCursor(xStart + (int)wDollar + gap, yBase);
  display.print(numBuf);
}

// Main chart (including previous day average reference line)
static void drawHistoryChart() {
 // V0.97: Chart always stays in USD scale.
 // (Only the big price display converts to NTD; the chart has no unit labels.)
  const float fx = 1.0f;

  int panelLeft   = SYMBOL_PANEL_WIDTH;
  int panelRight  = display.width();
  int chartTop    = 70 + largeContentYOffset();
  int chartBottom = display.height() - 6;

  const int MIN_POINTS_FOR_CHART = 4;
  if (g_chartSampleCount < MIN_POINTS_FOR_CHART) {
    display.setFont();
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(panelLeft + 4, chartBottom - 4);
    display.print("Collecting data...");
    return;
  }

    float minP = g_chartSamples[0].price * fx;
  float maxP = g_chartSamples[0].price * fx;
  for (int i = 1; i < g_chartSampleCount; ++i) {
        float p0 = g_chartSamples[i].price * fx;
    if (p0 < minP) minP = p0;
    if (p0 > maxP) maxP = p0;
  }

  if (g_dayAvgMode != DAYAVG_OFF && g_prevDayRefValid) {
        float p = g_prevDayRefPrice * fx;
    if (p < minP) minP = p;
    if (p > maxP) maxP = p;
  }

  if (minP == maxP) {
    maxP = minP + 1.0f;
  }

  int chartHeight = chartBottom - chartTop;
  int chartWidth  = panelRight - panelLeft - 4;

  int yDayAvg = -1;
  if (g_dayAvgMode != DAYAVG_OFF && g_prevDayRefValid) {
        float norm = ((g_prevDayRefPrice * fx) - minP) / (maxP - minP);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    yDayAvg = chartBottom - int(norm * chartHeight);
  }

  if (yDayAvg >= chartTop && yDayAvg <= chartBottom) {
    int xStart = panelLeft + 2;
    int xEnd   = panelRight - 2;
    int dashLen = 4;
    int gapLen  = 4;
    int x = xStart;
    while (x < xEnd) {
      int x2 = x + dashLen;
      if (x2 > xEnd) x2 = xEnd;
      display.drawLine(x, yDayAvg, x2, yDayAvg, GxEPD_BLACK);
      x = x2 + gapLen;
    }
  }

  int prevX = 0, prevY = 0;
  for (int i = 0; i < g_chartSampleCount; ++i) {
    float pos = g_chartSamples[i].pos;
        float p   = g_chartSamples[i].price * fx;

    if (pos < 0.0f) pos = 0.0f;
    if (pos > 1.0f) pos = 1.0f;

    int x = panelLeft + 2 + int(pos * (chartWidth - 1));
    if (x < panelLeft + 2)  x = panelLeft + 2;
    if (x > panelRight - 2) x = panelRight - 2;

    float norm = (p - minP) / (maxP - minP);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    int y = chartBottom - int(norm * chartHeight);

    if (i > 0) {
      display.drawLine(prevX, prevY, x, y, GxEPD_BLACK);
    }
    prevX = x;
    prevY = y;
  }
}

// Boot splash screen
void drawSplashScreen(const char* version) {
  const char* title = "CryptoBar";

  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

 // --- Center title (V0.99e: Space Grotesk Medium 24pt) ---
    display.setFont(&SpaceGrotesk_Medium24pt7b);
    display.setTextColor(GxEPD_BLACK);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);

    int16_t x = (display.width() - (int)w) / 2 - x1;
    int16_t y = (display.height() - (int)h) / 2 - y1;
    display.setCursor(x, y);
    display.print(title);

 // --- Bottom-right version ---
    if (version && version[0]) {
      display.setFont(&FreeSansBold9pt7b);
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);

      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}


// WiFi provisioning / status screens
// Preparing AP screen (AP startup can take ~30s on some boards/firmware)
void drawWifiPreparingApScreen(const char* version) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("WiFi Setup");

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(8, 52);
    display.print("Preparing AP...");
    display.setCursor(8, 72);
    display.print("Please wait (10s)");

    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}

// WiFi AP portal instructions screen
void drawWifiPortalScreen(const char* version, const char* apSsid, const char* apIp) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

 // Title
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("WiFi Setup");

 // Steps (compact to avoid clipping on 2.9" display)
    display.setFont(&FreeSansBold9pt7b);
    int y = 44;
    const int step = 15;

    display.setCursor(8, y);
    display.print("1) Connect phone to:");
    y += step;
    display.setCursor(8, y);
    display.print(apSsid ? apSsid : "");
    y += step;

    display.setCursor(8, y);
    display.print("2) Open browser to:");
    y += step;
    display.setCursor(8, y);
    display.print(apIp ? apIp : "");
    y += step;

    display.setCursor(8, y);
    display.print("3) Enter SSID+PW, Save");

 // Bottom-right version
    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}

// Firmware update confirm screen (enter maintenance AP)
void drawFirmwareUpdateConfirmScreen(const char* version) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("Firmware Update");

    display.setFont(&FreeSansBold9pt7b);
    int y = 52;
    display.setCursor(8, y);
    display.print("Hold button to enter");
    y += 18;
    display.setCursor(8, y);
    display.print("maintenance mode");
    y += 18;
    display.setCursor(8, y);
    display.print("(short press to cancel)");

 // Bottom-right version
    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}

// Firmware update / maintenance AP instructions
void drawFirmwareUpdateApScreen(const char* version, const char* apSsid, const char* apIp) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

 // Title
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("Firmware Update");

 // Steps (compact to avoid clipping on 2.9" display)
    display.setFont(&FreeSansBold9pt7b);
    int y = 44;
    const int step = 15;

    display.setCursor(8, y);
    display.print("1) Connect phone to:");
    y += step;
    display.setCursor(8, y);
    display.print(apSsid ? apSsid : "");
    y += step;

    display.setCursor(8, y);
    display.print("2) Open browser to:");
    y += step;
    display.setCursor(8, y);
    display.print(apIp ? apIp : "");
    y += step;

    display.setCursor(8, y);
    display.print("3) Upload new firmware");
    y += step;
    display.setCursor(8, y);
    display.print("(Maintenance UI)");

 // Bottom-right version
    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}

void drawWifiConnectingScreen(const char* version, const char* ssid) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("Connecting WiFi");

    display.setFont(&FreeSansBold9pt7b);
    int y = 52;
    display.setCursor(8, y);
    display.print("SSID:");
    y += 18;
    display.setCursor(8, y);
    display.print(ssid && ssid[0] ? ssid : "(unknown)");
    y += 22;

    display.setCursor(8, y);
    display.print("Please wait...");

 // Bottom-right version
    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}

void drawWifiConnectFailedScreen(const char* version) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("WiFi Failed");

    display.setFont(&FreeSansBold9pt7b);
    int y = 52;
    display.setCursor(8, y);
    display.print("Could not connect.");
    y += 18;
    display.setCursor(8, y);
    display.print("Restarting portal...");

 // Bottom-right version
    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}

void drawWifiInfoScreen(const char* version, const char* mac, const char* staIp,
                        int signalBars, int channel, bool connected) {
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 24);
    display.print("WiFi Info");

    display.setFont(&FreeSansBold9pt7b);
    int y = 50;

    display.setCursor(8, y);
    display.print("Status: ");
    display.print(connected ? "Connected" : "Disconnected");
    y += 18;

    display.setCursor(8, y);
    display.print("MAC: ");
    display.print(mac && mac[0] ? mac : "-");
    y += 18;

    display.setCursor(8, y);
    display.print("STA IP: ");
    display.print(staIp && staIp[0] ? staIp : "-");
    y += 18;

    display.setCursor(8, y);
    display.print("Signal: ");
    if (connected) {
      display.print(signalBars);
      display.print("/5");
    } else {
      display.print("-");
    }
    y += 18;

    display.setCursor(8, y);
    display.print("Channel: ");
    if (connected) display.print(channel);
    else display.print("-");
    y += 22;

    display.setCursor(8, y);
    display.print("Short press: Back");
    y += 18;
    display.setCursor(8, y);
    display.print("Long press: Exit");

 // Bottom-right version
    if (version && version[0]) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(version, 0, 0, &x1, &y1, &w, &h);
      const int16_t margin = 4;
      int16_t vx = (display.width() - margin) - (x1 + (int)w);
      int16_t vy = (display.height() - margin) - (y1 + (int)h);
      display.setCursor(vx, vy);
      display.print(version);
    }

  } while (display.nextPage());
}



// Main screen (full / partial refresh)
void drawMainScreen(float priceUsd, float change24h, bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(SYMBOL_PANEL_WIDTH, 0,
                             display.width() - SYMBOL_PANEL_WIDTH,
                             display.height());
  }

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    drawSymbolPanel(currentCoin().ticker, change24h);
    drawHeaderDateTime();
    drawPriceCenter(priceUsd);
    drawHistoryChart();

  } while (display.nextPage());
}

// Draw scrollbar (shared by menu / tz menu)
// Settings main menu: keep cursor in visible range
// Settings main menu screen
// Timezone submenu: keep cursor in visible range
// Timezone submenu screen
