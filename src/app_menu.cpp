// CryptoBar V0.98 (Refactored: Step 5)
// app_menu.cpp - Menu navigation and settings management

#include <Arduino.h>
#include <WiFi.h>
#include "app_menu.h"
#include "app_state.h"
#include "app_wifi.h"
#include "app_time.h"
#include "app_scheduler.h"
#include "settings_store.h"
#include "led_status.h"
#include "coins.h"
#include "day_avg.h"
#include "network.h"
#include "ui.h"

// Forward declarations for functions that remain in main.cpp
extern const CoinInfo& currentCoin();
extern const char* dayAvgModeLabel(uint8_t mode);
extern void updateAvgLineReference(time_t nowUtc);
extern void updateEtCycle();
extern void bootstrapHistoryFromKrakenOHLC();
extern void updateLedForPrice(float change24h, bool priceOk);
extern unsigned long lastUpdate;  // from main.cpp

// ==================== Settings Management =====================

void loadSettings() {
  StoredSettings st;
  settingsStoreLoad(st);

 // Whether tzIndex was explicitly stored in NVS. Used for one-time Auto TZ.
  g_tzIndexKeyPresent = settingsStoreHasTzIndex();
  if (!g_bootFlagsInit) {
    g_tzIndexKeyPresentAtBoot = g_tzIndexKeyPresent;
    g_bootFlagsInit = true;
  }

  int upd   = st.updPreset;
  int bri   = st.briPreset;
  int coin  = coinIndexFromTicker(st.coinTicker.c_str());
  int tFmt  = st.timeFmt;
  int dFmt  = st.dateFmt;
  int dtSz  = st.dtSize;
  int tzIdx = st.tzIndex;
  int dayLn = st.dayAvg;
  int rf    = st.rfMode;

  int updCount   = UPDATE_PRESETS_COUNT;
  int briCount   = BRIGHTNESS_PRESETS_COUNT;

  if (upd < 0 || upd >= updCount) upd = 0;
  if (bri < 0 || bri >= briCount) bri = 1;
  if (coin < 0 || coin >= coinCount()) coin = coinIndexFromTicker(coinDefault().ticker);
  if (tFmt < 0 || tFmt > TIME_12H) tFmt = TIME_12H;
  if (dFmt < 0 || dFmt >= DATE_FORMAT_COUNT) dFmt = DATE_MM_DD_YYYY;
  if (dtSz < 0 || dtSz > 1) dtSz = 1;
  if (tzIdx < 0 || tzIdx >= TIMEZONE_COUNT) tzIdx = DEFAULT_TIMEZONE_INDEX;
  if (dayLn < 0 || dayLn > 2) dayLn = DAYAVG_ROLLING;
  if (rf < 0 || rf > 1) rf = 1;

  g_updatePresetIndex     = upd;
  g_updateIntervalMs      = UPDATE_PRESETS_MS[g_updatePresetIndex];
  g_brightnessPresetIndex = bri;
  g_ledBrightness         = BRIGHTNESS_PRESETS[g_brightnessPresetIndex];
  ledStatusSetMasterBrightness(g_ledBrightness);
  g_currentCoinIndex      = coin;
  g_timeFormat            = tFmt;
  g_dateFormatIndex       = dFmt;
  g_dtSize                = dtSz;
  g_timezoneIndex         = tzIdx;
  g_dayAvgMode          = (uint8_t)dayLn;
  g_refreshMode           = rf;

  g_displayCurrency = st.dispCur;
  if (g_displayCurrency < 0 || g_displayCurrency >= (int)CURR_COUNT) g_displayCurrency = (int)CURR_USD;
  applyTimezone();

  Serial.printf("[Settings] Loaded: coin=%s, upd=%s, LED=%s, timeFmt=%s, date=%s, dtSize=%s, tz=%s, dayAvg=%s, refresh=%s, cur=%s\n",
                currentCoin().ticker,
                UPDATE_PRESET_LABELS[g_updatePresetIndex],
                BRIGHTNESS_LABELS[g_brightnessPresetIndex],
                (g_timeFormat == TIME_24H ? "24h" : "12h"),
                DATE_FORMAT_LABELS[g_dateFormatIndex],
                DTSIZE_LABELS[g_dtSize],
                TIMEZONES[g_timezoneIndex].label,
                dayAvgModeLabel(g_dayAvgMode),
                REFRESH_MODE_LABELS[g_refreshMode],
                CURRENCY_INFO[g_displayCurrency].code);
}

void saveSettings() {
  StoredSettings st;
  st.updPreset = g_updatePresetIndex;
  st.briPreset = g_brightnessPresetIndex;
  st.coinTicker = String(currentCoin().ticker);
  st.timeFmt   = g_timeFormat;
  st.dateFmt   = g_dateFormatIndex;
  st.dtSize    = g_dtSize;
  st.tzIndex   = g_timezoneIndex;
  st.dayAvg    = (int)g_dayAvgMode;
  st.rfMode    = g_refreshMode;
  st.dispCur   = g_displayCurrency;

  settingsStoreSave(st);

  Serial.printf("[Settings] Saved: coin=%s, upd=%s, LED=%s, timeFmt=%s, date=%s, dtSize=%s, tz=%s, dayAvg=%s, refresh=%s, cur=%s\n",
                currentCoin().ticker,
                UPDATE_PRESET_LABELS[g_updatePresetIndex],
                BRIGHTNESS_LABELS[g_brightnessPresetIndex],
                (g_timeFormat == TIME_24H ? "24h" : "12h"),
                DATE_FORMAT_LABELS[g_dateFormatIndex],
                DTSIZE_LABELS[g_dtSize],
                TIMEZONES[g_timezoneIndex].label,
                dayAvgModeLabel(g_dayAvgMode),
                REFRESH_MODE_LABELS[g_refreshMode],
                CURRENCY_INFO[g_displayCurrency].code);
}

// ==================== Menu Navigation =====================

void leaveMenu() {
  g_uiMode = UI_MODE_NORMAL;
  Serial.println("[Menu] Exit to main");
  drawMainScreen(g_lastPriceUsd, g_lastChange24h, true);
  g_partialRefreshCount = 0;
  lastUpdate = millis();
}

void enterMenu() {
  g_uiMode       = UI_MODE_MENU;
  g_menuIndex    = 0;
  g_menuTopIndex = 0;
  Serial.println("[Menu] Enter");
  drawMenuScreen(true);
}

void enterTimezoneSubmenu() {
  g_uiMode         = UI_MODE_TZ_SUB;
 // g_tzMenuIndex is a DISPLAY position (UTC-ordered), not the raw tzIndex.
  g_tzMenuIndex    = tzDisplayPosFromTzIndex(g_timezoneIndex);
  g_tzMenuTopIndex = max(0, g_tzMenuIndex - 2);
  Serial.println("[Menu] Enter TZ submenu");
  drawTimezoneMenu(true);
}

// ==================== Menu Item Handlers =====================

void handleTimezoneSelect() {
 // g_tzMenuIndex is a DISPLAY position (UTC-ordered)
  g_timezoneIndex = tzIndexFromDisplayPos(g_tzMenuIndex);
  applyTimezone();
  saveSettings();
  Serial.printf("[Menu] Timezone -> %s (UTC%+d)\n", TIMEZONES[g_timezoneIndex].label, (int)TIMEZONES[g_timezoneIndex].utcOffsetHours);

  g_uiMode = UI_MODE_MENU;
  drawMenuScreen(true);
}

void handleCoinSelect() {
  g_currentCoinIndex = g_coinMenuIndex;
  saveSettings();
  Serial.printf("[Menu] Coin -> %s (index=%d)\n",
                currentCoin().ticker, g_currentCoinIndex);

 // Reset chart / cycle and re-bootstrap history
  g_chartSampleCount = 0;
  g_cycleInit        = false;
  updateEtCycle();
  bootstrapHistoryFromKrakenOHLC();

 // Force an immediate price refresh on coin change (best-effort)
  float price  = 0.0f;
  float change = 0.0f;
  if (fetchPrice(price, change)) {
    g_lastPriceUsd  = price;
    g_lastChange24h = change;
    g_lastPriceOk   = true;
    updateLedForPrice(g_lastChange24h, g_lastPriceOk);
  } else {
    g_lastPriceOk = false;
    updateLedForPrice(0.0f, false);
  }

  g_uiMode = UI_MODE_MENU;
  drawMenuScreen(true);
}

void handleMenuSelect() {
  switch (g_menuIndex) {
    case MENU_COIN: {
 // Enter coin submenu
      g_uiMode           = UI_MODE_COIN_SUB;
      g_coinMenuIndex    = g_currentCoinIndex;
      g_coinMenuTopIndex = 0;
      g_coinDirty        = true;
      Serial.println("[Menu] Enter COIN submenu");
      drawCoinMenu(true);
      break;
    }

    case MENU_UPDATE_INTERVAL: {
      int presetCount = UPDATE_PRESETS_COUNT;
      g_updatePresetIndex = (g_updatePresetIndex + 1) % presetCount;
      g_updateIntervalMs  = UPDATE_PRESETS_MS[g_updatePresetIndex];
      Serial.printf("[Menu] Update interval -> %s\n",
                    UPDATE_PRESET_LABELS[g_updatePresetIndex]);
      saveSettings();
      tickSchedulerReset("MENU_UPD");
      lastUpdate = millis();
      drawMenuScreen(false);
      break;
    }

    case MENU_LED_BRIGHTNESS: {
      int presetCount = BRIGHTNESS_PRESETS_COUNT;
      g_brightnessPresetIndex = (g_brightnessPresetIndex + 1) % presetCount;
      g_ledBrightness         = BRIGHTNESS_PRESETS[g_brightnessPresetIndex];
      ledStatusSetMasterBrightness(g_ledBrightness);
      Serial.printf("[Menu] LED brightness -> %s\n",
                    BRIGHTNESS_LABELS[g_brightnessPresetIndex]);
      saveSettings();
      uint8_t r=0,g=0,b=0;
      ledStatusGetLogicalRgb(&r, &g, &b);
      fadeLedTo(r, g, b, 5, 10);
      drawMenuScreen(false);
      break;
    }

    case MENU_TIME_FORMAT: {
      g_timeFormat = (g_timeFormat == TIME_24H) ? TIME_12H : TIME_24H;
      Serial.printf("[Menu] Time format -> %s\n",
                    (g_timeFormat == TIME_24H ? "24h" : "12h"));
      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_DATE_FORMAT: {
      g_dateFormatIndex = (g_dateFormatIndex + 1) % DATE_FORMAT_COUNT;
      Serial.printf("[Menu] Date format -> %s\n",
                    DATE_FORMAT_LABELS[g_dateFormatIndex]);
      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_DATETIME_SIZE: {
      g_dtSize = (g_dtSize == 0) ? 1 : 0;
      Serial.printf("[Menu] Date/Time size -> %s\n",
                    DTSIZE_LABELS[(g_dtSize >= 0 && g_dtSize < 2) ? g_dtSize : 1]);
      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_TIMEZONE: {
      enterTimezoneSubmenu();
      break;
    }

    case MENU_REFRESH_MODE: {
      g_refreshMode = (g_refreshMode + 1) % 2;
      Serial.printf("[Menu] Refresh mode -> %s\n", REFRESH_MODE_LABELS[g_refreshMode]);
      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_CURRENCY: {
      // V0.99f: Cycle through all supported currencies
      g_displayCurrency = (g_displayCurrency + 1) % (int)CURR_COUNT;
      if (g_displayCurrency < 0 || g_displayCurrency >= (int)CURR_COUNT) g_displayCurrency = (int)CURR_USD;
      Serial.printf("[Menu] Currency -> %s\n", CURRENCY_INFO[g_displayCurrency].code);

      // If switching to non-USD currency, refresh FX soon
      if (g_displayCurrency != (int)CURR_USD) g_nextFxUpdateUtc = 0;

      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_DAYAVG_LINE: {
      g_dayAvgMode = (uint8_t)((g_dayAvgMode + 1) % 3);
      Serial.printf("[Menu] Day avg -> %s\n", dayAvgModeLabel(g_dayAvgMode));
 // Update reference immediately so the dashed line reflects the new mode.
      updateAvgLineReference(time(nullptr));
      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_FIRMWARE_UPDATE: {
 // Two-step entry: show confirm screen, then long-press to enter maintenance AP.
      g_uiMode = UI_MODE_FW_CONFIRM;
      Serial.println("[Menu] Firmware update (confirm)");
      drawFirmwareUpdateConfirmScreen(CRYPTOBAR_VERSION);
      break;
    }

    case MENU_WIFI_INFO: {
      bool connected = (WiFi.status() == WL_CONNECTED);
      String mac = WiFi.macAddress();
      String ip  = connected ? WiFi.localIP().toString() : String("");
      int bars   = connected ? rssiToBars(WiFi.RSSI()) : 0;
      int ch     = connected ? WiFi.channel() : 0;

      g_uiMode = UI_MODE_WIFI_INFO;
      drawWifiInfoScreen(CRYPTOBAR_VERSION, mac.c_str(), ip.c_str(), bars, ch, connected);
      break;
    }

    case MENU_EXIT:
      leaveMenu();
      break;
  }
}
