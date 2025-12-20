// CryptoBar V0.99f (Multi-Currency Support)
// ui_currency.cpp - Currency selection submenu

#include <Arduino.h>

#include <GxEPD2_BW.h>

#include "config.h"
#include "ui.h"
#include "ui_list.h"

// e-paper display object (defined in main.cpp)
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// Currency menu state (defined in app_state.cpp)
extern int g_currencyMenuIndex;
extern int g_currencyMenuTopIndex;

static const int VISIBLE_CURRENCY_LINES = 5;
static const int CURRENCY_MENU_X        = 8;

void ensureCurrencyMenuVisible() {
  if (g_currencyMenuIndex < g_currencyMenuTopIndex) {
    g_currencyMenuTopIndex = g_currencyMenuIndex;
  } else if (g_currencyMenuIndex >= g_currencyMenuTopIndex + VISIBLE_CURRENCY_LINES) {
    g_currencyMenuTopIndex = g_currencyMenuIndex - (VISIBLE_CURRENCY_LINES - 1);
  }

  int n = (int)CURR_COUNT;
  if (g_currencyMenuTopIndex < 0) g_currencyMenuTopIndex = 0;
  if (g_currencyMenuTopIndex > n - VISIBLE_CURRENCY_LINES) {
    g_currencyMenuTopIndex = max(0, n - VISIBLE_CURRENCY_LINES);
  }
}

static void printCurrencyItem(int i) {
  if (i >= 0 && i < (int)CURR_COUNT) {
    const CurrencyInfo& curr = CURRENCY_INFO[i];
    // Display currency code only (e.g., "USD", "JPY")
    display.print(curr.code);
  }
}

void drawCurrencyMenu(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

  ensureCurrencyMenuVisible();

  const UiListLayout layout {
    .x = CURRENCY_MENU_X,
    .titleY = 30,
    .listYStart = 54,
    .lineStep = 18,
    .trackX = (int)display.width() - 5,
    .trackTop = 40,
    .trackBottom = (int)display.height() - 10
  };

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    uiDrawListPage("Currency", layout, (int)CURR_COUNT, g_currencyMenuTopIndex,
                   VISIBLE_CURRENCY_LINES, g_currencyMenuIndex, printCurrencyItem);
  } while (display.nextPage());
}
