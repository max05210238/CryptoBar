#include <Arduino.h>

#include <GxEPD2_BW.h>

#include "config.h"
#include "ui.h"
#include "ui_list.h"

// e-paper display object (defined in main.cpp)
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// Timezone menu state (defined in main.cpp)
extern int g_tzMenuIndex;
extern int g_tzMenuTopIndex;

static const int VISIBLE_TZ_LINES = 5;
static const int TZ_MENU_X        = 8;

void ensureTzMenuVisible() {
  if (g_tzMenuIndex < g_tzMenuTopIndex) {
    g_tzMenuTopIndex = g_tzMenuIndex;
  } else if (g_tzMenuIndex >= g_tzMenuTopIndex + VISIBLE_TZ_LINES) {
    g_tzMenuTopIndex = g_tzMenuIndex - (VISIBLE_TZ_LINES - 1);
  }
  if (g_tzMenuTopIndex < 0) g_tzMenuTopIndex = 0;
  if (g_tzMenuTopIndex > TIMEZONE_COUNT - VISIBLE_TZ_LINES)
    g_tzMenuTopIndex = max(0, TIMEZONE_COUNT - VISIBLE_TZ_LINES);
}

static void printTimezoneItem(int i) {
  int tzIdx = tzIndexFromDisplayPos(i);
  display.print(TIMEZONES[tzIdx].label);
}

void drawTimezoneMenu(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

  ensureTzMenuVisible();

  const UiListLayout layout {
    .x = TZ_MENU_X,
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
    uiDrawListPage("Time zone", layout, TIMEZONE_COUNT, g_tzMenuTopIndex,
                   VISIBLE_TZ_LINES, g_tzMenuIndex, printTimezoneItem);
  } while (display.nextPage());
}
