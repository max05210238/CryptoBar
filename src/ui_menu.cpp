#include <Arduino.h>

#include <GxEPD2_BW.h>

#include "coins.h"
#include "ui.h"
#include "ui_list.h"

// e-paper display object (defined in main.cpp)
extern GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display;

// Menu state (defined in main.cpp)
extern int g_menuIndex;
extern int g_menuTopIndex;

// Settings state (defined in main.cpp)
extern int  g_dateFormatIndex;
extern int  g_timeFormat;
extern int  g_dtSize;
extern int  g_timezoneIndex;
#include "day_avg.h"
extern uint8_t g_dayAvgMode;

extern int g_brightnessPresetIndex;
extern int g_updatePresetIndex;
extern int g_refreshMode;
extern int g_displayCurrency;

// Labels (defined in main.cpp)
extern const char* BRIGHTNESS_LABELS[];
extern const char* UPDATE_PRESET_LABELS[];
extern const char* DATE_FORMAT_LABELS[];
extern const char* DTSIZE_LABELS[];
extern const char* REFRESH_MODE_LABELS[];

// Helper (implemented in main.cpp)
const CoinInfo& currentCoin();

// V0.99f: Multi-currency labels (matches DisplayCurrency enum order)
static const char* CURRENCY_LABELS[] = {
  "USD", "TWD", "EUR", "GBP", "CAD", "JPY", "KRW", "SGD", "AUD"
};

static const int VISIBLE_MENU_LINES = 5;
static const int MENU_X             = 8;

void ensureMainMenuVisible() {
  if (g_menuIndex < g_menuTopIndex) {
    g_menuTopIndex = g_menuIndex;
  } else if (g_menuIndex >= g_menuTopIndex + VISIBLE_MENU_LINES) {
    g_menuTopIndex = g_menuIndex - (VISIBLE_MENU_LINES - 1);
  }
  if (g_menuTopIndex < 0) g_menuTopIndex = 0;
  if (g_menuTopIndex > MENU_COUNT - VISIBLE_MENU_LINES)
    g_menuTopIndex = max(0, MENU_COUNT - VISIBLE_MENU_LINES);
}

static void printMenuItem(int i) {
  switch (i) {
    case MENU_COIN:
      display.print("Coin: ");
      display.print(currentCoin().ticker);
      break;
    case MENU_UPDATE_INTERVAL:
      display.print("Update: ");
      display.print(UPDATE_PRESET_LABELS[g_updatePresetIndex]);
      break;
    case MENU_REFRESH_MODE:
      display.print("Refresh: ");
      display.print(REFRESH_MODE_LABELS[g_refreshMode]);
      break;
    case MENU_CURRENCY: {
      display.print("Currency: ");
      int idx = (g_displayCurrency>=0 && g_displayCurrency < (int)CURR_COUNT) ? g_displayCurrency : 0;
      // V0.99f: Display currency code only (e.g., "USD", "TWD")
      display.print(CURRENCY_INFO[idx].code);
      break;
    }
    case MENU_LED_BRIGHTNESS:
      display.print("LED: ");
      display.print(BRIGHTNESS_LABELS[g_brightnessPresetIndex]);
      break;
    case MENU_TIME_FORMAT:
      display.print("Time: ");
      display.print((g_timeFormat == TIME_24H) ? "24h" : "12h");
      break;
    case MENU_DATE_FORMAT:
      display.print("Date: ");
      display.print(DATE_FORMAT_LABELS[g_dateFormatIndex]);
      break;
    case MENU_DATETIME_SIZE:
      display.print("DT Size: ");
      display.print(DTSIZE_LABELS[(g_dtSize >= 0 && g_dtSize < 2) ? g_dtSize : 1]);
      break;
    case MENU_TIMEZONE:
      display.print("Time zone: ");
      display.print(TIMEZONES[g_timezoneIndex].label);
      break;
    case MENU_DAYAVG_LINE:
      display.print("Day avg: ");
      switch (g_dayAvgMode) {
        case DAYAVG_OFF:     display.print("Off");       break;
        case DAYAVG_ROLLING: display.print("24h mean");  break;
        case DAYAVG_CYCLE:   display.print("Cycle mean");break;
        default:             display.print("24h mean");  break;
      }
      break;

    case MENU_FIRMWARE_UPDATE:
      display.print("Firmware Update");
      break;
    case MENU_WIFI_INFO:
      display.print("WiFi Info");
      break;
    case MENU_EXIT:
      display.print("Exit");
      break;
  }
}

void drawMenuScreen(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

  ensureMainMenuVisible();

  const UiListLayout layout {
    .x = MENU_X,
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
    uiDrawListPage("Settings", layout, MENU_COUNT, g_menuTopIndex,
                   VISIBLE_MENU_LINES, g_menuIndex, printMenuItem);
  } while (display.nextPage());
}
