// CryptoBar V0.99s (VFD Display Support)
// ui_update.cpp - Update interval selection submenu

#include <Arduino.h>

#include <GxEPD2_BW.h>

#include "app_state.h"
#include "ui.h"
#include "ui_list.h"

// e-paper display object (defined in main.cpp)
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// Update menu state (defined in app_state.cpp)
extern int g_updateMenuIndex;
extern int g_updateMenuTopIndex;

static const int VISIBLE_UPDATE_LINES = 5;
static const int UPDATE_MENU_X        = 8;

void ensureUpdateMenuVisible() {
  if (g_updateMenuIndex < g_updateMenuTopIndex) {
    g_updateMenuTopIndex = g_updateMenuIndex;
  } else if (g_updateMenuIndex >= g_updateMenuTopIndex + VISIBLE_UPDATE_LINES) {
    g_updateMenuTopIndex = g_updateMenuIndex - (VISIBLE_UPDATE_LINES - 1);
  }

  int n = UPDATE_PRESETS_COUNT;
  if (g_updateMenuTopIndex < 0) g_updateMenuTopIndex = 0;
  if (g_updateMenuTopIndex > n - VISIBLE_UPDATE_LINES) {
    g_updateMenuTopIndex = max(0, n - VISIBLE_UPDATE_LINES);
  }
}

static void printUpdateItem(int i) {
  if (i >= 0 && i < UPDATE_PRESETS_COUNT) {
    // Display update interval label (e.g., "1m", "3m", "5m", "10m")
    display.print(UPDATE_PRESET_LABELS[i]);
  }
}

void drawUpdateMenu(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

  ensureUpdateMenuVisible();

  const UiListLayout layout {
    .x = UPDATE_MENU_X,
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
    uiDrawListPage("Update Interval", layout, UPDATE_PRESETS_COUNT, g_updateMenuTopIndex,
                   VISIBLE_UPDATE_LINES, g_updateMenuIndex, printUpdateItem);
  } while (display.nextPage());
}
