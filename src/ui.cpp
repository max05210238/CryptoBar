#include <Arduino.h>
#include <time.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

#include "SpaceGroteskBold24pt7b.h"

#include "config.h"
#include "app_state.h"
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
// Note: g_refreshMode, g_displayCurrency, g_usdToTwd, g_fxValid are declared in app_state.h
extern int g_refreshMode;
extern const char* REFRESH_MODE_LABELS[];

// Previous day average reference line
extern double g_prevDayRefPrice;
extern bool   g_prevDayRefValid;

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

// V0.99m: Left black panel: price API + currency + coin symbol + 24h change + history API
static void drawSymbolPanel(const char* symbol, float change24h) {
  int panelWidth = SYMBOL_PANEL_WIDTH;

  display.fillRect(0, 0, panelWidth, display.height(), GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);

  const GFXfont* bigFont   = &FreeSansBold18pt7b;
  const GFXfont* smallFont = &FreeSansBold9pt7b;

  // V0.99m: API source labels (dynamic, updated by network.cpp)
  const char* priceApiLabel = g_currentPriceApi;      // Real-time price data source
  const char* historyApiLabel = g_currentHistoryApi;  // Historical data source

  // Get bounds for all elements

  // Price API label bounds (extra small font - default 6x8)
  display.setFont();  // Use default 6x8 font for extra small text
  display.setTextSize(1);
  int16_t apiX1, apiY1;
  uint16_t apiW, apiH;
  display.getTextBounds(priceApiLabel, 0, 0, &apiX1, &apiY1, &apiW, &apiH);

  // Currency code bounds (small font)
  const CurrencyInfo& curr = CURRENCY_INFO[g_displayCurrency];
  const char* currCode = curr.code;  // Just the code, no symbol

  display.setFont(smallFont);
  int16_t currX1, currY1;
  uint16_t currW, currH;
  display.getTextBounds(currCode, 0, 0, &currX1, &currY1, &currW, &currH);

  // Coin symbol bounds (big font)
  display.setFont(bigFont);
  int16_t sx1, sy1;
  uint16_t sW, sH;
  display.getTextBounds(symbol, 0, 0, &sx1, &sy1, &sW, &sH);

  // Change percentage bounds (small font)
  char changeBuf[24];
  snprintf(changeBuf, sizeof(changeBuf), "%+.2f%%", change24h);

  display.setFont(smallFont);
  int16_t cx1, cy1;
  uint16_t cW, cH;
  display.getTextBounds(changeBuf, 0, 0, &cx1, &cy1, &cW, &cH);

  // History API label bounds (extra small font - default 6x8)
  display.setFont();
  display.setTextSize(1);
  int16_t histX1, histY1;
  uint16_t histW, histH;
  display.getTextBounds(historyApiLabel, 0, 0, &histX1, &histY1, &histW, &histH);

  // Calculate vertical layout with BTC centered in black panel (128px height)
  // Spacing: topApiGap=22px, normalGap=7px, bottomApiGap=4px
  const int topApiGap = 22;   // Gap between Paprika and USD
  const int bottomApiGap = 4; // Smaller gap before bottom API label
  const int normalGap = 7;    // Gap between main elements

  // Center BTC symbol at display midpoint
  // BTC baseline calculation: center - sy1 - sH/2 (to position visual center at midpoint)
  int16_t btcCenter = display.height() / 2;  // 64 for 128px display
  int16_t sy = btcCenter - sy1 - sH / 2;
  int16_t sx = (panelWidth - sW) / 2;

  // Calculate baselines using spacing formula:
  // Upper element: current_baseline - (current_height + gap)
  // Lower element: current_baseline + (gap + element_height)

  // USD baseline (above BTC): BTC_baseline - (BTC_height + gap)
  int16_t currY = sy - sH - normalGap;
  int16_t currX = (panelWidth - currW) / 2;

  // Paprika baseline (above USD): USD_baseline - (USD_height + gap)
  int16_t apiY = currY - currH - topApiGap;
  int16_t apiX = (panelWidth - apiW) / 2;

  // Change% baseline (below BTC): BTC_baseline + (gap + change_height)
  int16_t cy = sy + normalGap + cH;
  int16_t cx = (panelWidth - cW) / 2;

  // CoinGecko baseline (below %): change_baseline + (change_height + gap)
  int16_t histY = cy + cH + bottomApiGap;
  int16_t histX = (panelWidth - histW) / 2;

  // Draw price API label (top, extra small)
  display.setFont();
  display.setTextSize(1);
  display.setCursor(apiX, apiY);
  display.print(priceApiLabel);

  // Draw currency code
  display.setFont(smallFont);
  display.setCursor(currX, currY);
  display.print(currCode);

  // Draw coin symbol (centered in black panel)
  display.setFont(bigFont);
  display.setCursor(sx, sy);
  display.print(symbol);

  // Draw change percentage
  display.setFont(smallFont);
  display.setCursor(cx, cy);
  display.print(changeBuf);

  // Draw history API label (bottom, extra small)
  display.setFont();
  display.setTextSize(1);
  display.setCursor(histX, histY);
  display.print(historyApiLabel);

  display.setTextColor(GxEPD_BLACK);
}

// V0.99p: Length-based decimal precision (max 10 chars: 9 digits + 1 decimal point)
// Try 4 decimals first, degrade to 2/0 if total length exceeds display limit
// Trailing zeros are preserved (e.g., "1.8600" indicates API precision limit)
static int detectDecimalPlaces(double price, int maxDecimals) {
  // Calculate integer part digit count
  // For price < 1.0, integer part is "0" (1 digit)
  int intDigits;
  if (price < 1.0) {
    intDigits = 1;  // "0.xxxx"
  } else {
    intDigits = (int)floor(log10(price)) + 1;
  }

  // Try decimal places in descending order: 4 → 2 → 0
  // Total length = integer digits + decimal point (if any) + decimal digits
  const int decimals[] = {4, 2, 0};
  for (int i = 0; i < 3; ++i) {
    int dec = decimals[i];
    if (dec > maxDecimals) continue;  // Skip if exceeds currency's max decimals

    int totalLen = intDigits + (dec > 0 ? 1 : 0) + dec;
    if (totalLen <= 10) {
      return dec;
    }
  }

  return 0;  // Fallback: no decimals (will trigger font downgrade if still too long)
}

// V0.99f: Center price display with multi-currency support (number only)
static void drawPriceCenter(double priceUsd) {
  int panelLeft  = SYMBOL_PANEL_WIDTH;
  int panelWidth = display.width() - panelLeft;

  // Convert for display if needed
  double fx = 1.0;
  if (g_displayCurrency != (int)CURR_USD && g_displayCurrency < (int)CURR_COUNT) {
    fx = g_usdToRate[g_displayCurrency];
  }
  double price = priceUsd * fx;

  // Get currency metadata
  const CurrencyInfo& curr = CURRENCY_INFO[g_displayCurrency];

  // V0.99p: Length-based decimal precision (auto-adjust for display width)
  // All currencies use same logic: 4 → 2 → 0 decimals based on total length
  // CoinGecko precision=full provides 14+ decimals, display max 4 for readability
  int maxDecimals = 4;  // All currencies (including JPY/KRW) use length-based logic
  int actualDecimals = detectDecimalPlaces(price, maxDecimals);

  // Start with 18pt font (may downgrade to 12pt if needed)
  const GFXfont* numFont = &FreeSansBold18pt7b;
  display.setFont(numFont);
  display.setTextColor(GxEPD_BLACK);

  // Available width for price number (no currency symbol, so more space)
  int maxNumberW = panelWidth - 8;  // 4px margin on each side
  if (maxNumberW < 10) maxNumberW = 10;

  // V0.99p: Trailing zeros preserved (indicates API precision)
  // Adaptive decimals: start at actualDecimals, reduce until it fits
  char numBuf[32];
  int16_t x1, y1;
  uint16_t wNum = 0, hNum = 0;
  bool needDowngrade = false;

  for (int dec = actualDecimals; dec >= 0; --dec) {
    snprintf(numBuf, sizeof(numBuf), "%.*f", dec, price);
    display.getTextBounds(numBuf, 0, 0, &x1, &y1, &wNum, &hNum);
    if ((int)wNum <= maxNumberW) break;

    // If we've reached 0 decimals and still doesn't fit, need to downgrade font
    if (dec == 0 && (int)wNum > maxNumberW) {
      needDowngrade = true;
    }
  }

  // Downgrade to 12pt font if necessary (e.g., BTC at $100,000+)
  if (needDowngrade) {
    numFont = &FreeSansBold12pt7b;
    display.setFont(numFont);

    // Retry formatting with smaller font
    for (int dec = actualDecimals; dec >= 0; --dec) {
      snprintf(numBuf, sizeof(numBuf), "%.*f", dec, price);
      display.getTextBounds(numBuf, 0, 0, &x1, &y1, &wNum, &hNum);
      if ((int)wNum <= maxNumberW) break;
    }
    Serial.printf("[UI] Price downgraded to 12pt font: %s\n", numBuf);
  }

  // Center the number horizontally
  int16_t xStart = panelLeft + (panelWidth - (int)wNum) / 2;
  int16_t yBase  = 52 + largeContentYOffset();

  // Draw price number only (no currency symbol)
  display.setFont(numFont);
  display.setCursor(xStart, yBase);
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

    double minP = g_chartSamples[0].price * fx;
  double maxP = g_chartSamples[0].price * fx;
  for (int i = 1; i < g_chartSampleCount; ++i) {
        double p0 = g_chartSamples[i].price * fx;
    if (p0 < minP) minP = p0;
    if (p0 > maxP) maxP = p0;
  }

  if (g_dayAvgMode != DAYAVG_OFF && g_prevDayRefValid) {
        double p = g_prevDayRefPrice * fx;
    if (p < minP) minP = p;
    if (p > maxP) maxP = p;
  }

  if (minP == maxP) {
    maxP = minP + 1.0;
  }

  int chartHeight = chartBottom - chartTop;
  int chartWidth  = panelRight - panelLeft - 4;

  int yDayAvg = -1;
  if (g_dayAvgMode != DAYAVG_OFF && g_prevDayRefValid) {
        double norm = ((g_prevDayRefPrice * fx) - minP) / (maxP - minP);
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
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
        double p   = g_chartSamples[i].price * fx;

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
void drawWifiPreparingApScreen(const char* version, bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

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
void drawWifiPortalScreen(const char* version, const char* apSsid, const char* apIp, bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

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
void drawFirmwareUpdateApScreen(const char* version, const char* apSsid, const char* apIp, bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

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

void drawWifiConnectingScreen(const char* version, const char* ssid, bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

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

void drawWifiConnectFailedScreen(const char* version, bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

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
void drawMainScreen(double priceUsd, double change24h, bool fullRefresh) {
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

// V0.99q: Time-only refresh (full-area partial refresh)
// Updates the entire white panel with partial refresh, but only the time actually changes
// GxEPD2 will only refresh pixels that differ from the previous state
// This avoids artifacts around the price area and ensures clean time updates
void drawMainScreenTimeOnly(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    // Partial window: entire right-side panel (not just time area)
    // This prevents partial refresh artifacts bleeding into price/chart areas
    display.setPartialWindow(SYMBOL_PANEL_WIDTH, 0,
                             display.width() - SYMBOL_PANEL_WIDTH,
                             display.height());
  }

  display.firstPage();
  do {
    // Clear and redraw the entire white panel
    // GxEPD2 compares old vs new pixels and only refreshes what changed (time area)
    display.fillScreen(GxEPD_WHITE);

    // Redraw all content with current data
    // Only time will change, price/chart data remains the same
    drawHeaderDateTime();           // Time updates (changes)
    drawPriceCenter(g_lastPriceUsd);    // Price stays same (no refresh)
    drawHistoryChart();             // Chart stays same (no refresh)

  } while (display.nextPage());
}

// Draw scrollbar (shared by menu / tz menu)
// Settings main menu: keep cursor in visible range
// Settings main menu screen
// Timezone submenu: keep cursor in visible range
// Timezone submenu screen
