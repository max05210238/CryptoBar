#include <Arduino.h>

#include <GxEPD2_BW.h>

#include "coins.h"
#include "ui.h"
#include "ui_list.h"

// e-paper display object (defined in main.cpp)
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// Coin menu state (defined in main.cpp)
extern int g_coinMenuIndex;
extern int g_coinMenuTopIndex;

static const int VISIBLE_COIN_LINES = 5;
static const int COIN_MENU_X        = 8;

void ensureCoinMenuVisible() {
  if (g_coinMenuIndex < g_coinMenuTopIndex) {
    g_coinMenuTopIndex = g_coinMenuIndex;
  } else if (g_coinMenuIndex >= g_coinMenuTopIndex + VISIBLE_COIN_LINES) {
    g_coinMenuTopIndex = g_coinMenuIndex - (VISIBLE_COIN_LINES - 1);
  }

  int n = coinCount();
  if (g_coinMenuTopIndex < 0) g_coinMenuTopIndex = 0;
  if (g_coinMenuTopIndex > n - VISIBLE_COIN_LINES) {
    g_coinMenuTopIndex = max(0, n - VISIBLE_COIN_LINES);
  }
}

static void printCoinItem(int i) {
  const CoinInfo& c = coinAt(i);
  display.print("#");
  display.print((int)c.marketRank);
  display.print(" ");
  display.print(c.ticker);
}

void drawCoinMenu(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

  ensureCoinMenuVisible();

  const UiListLayout layout {
    .x = COIN_MENU_X,
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
    uiDrawListPage("Coin", layout, coinCount(), g_coinMenuTopIndex,
                   VISIBLE_COIN_LINES, g_coinMenuIndex, printCoinItem);
  } while (display.nextPage());
}
