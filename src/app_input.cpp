// CryptoBar V0.98-rc4 (Refactored: Step 4)
// app_input.cpp - Input handling (encoder/button events)

#include <Arduino.h>
#include <WiFi.h>
#include "app_input.h"
#include "app_state.h"
#include "app_wifi.h"
#include "app_time.h"
#include "day_avg.h"
#include "coins.h"
#include "led_status.h"
#include "maint_mode.h"
#include "maint_boot.h"
#include "wifi_portal.h"
#include "ui.h"

// Forward declarations for UI functions that remain in main.cpp
extern void enterMenu();
extern void handleMenuSelect();
extern void handleTimezoneSelect();
extern void handleCoinSelect();
extern void leaveMenu();
extern void showWifiSetupRequired(uint32_t splashStartMs, bool rebootOnExit = true);
extern void saveSettings();

// ==================== Button Event Handlers =====================

void handleShortPress() {
  if (g_appState == APP_STATE_NEED_WIFI) return;

 // Firmware update confirm: short press = cancel
  if (g_uiMode == UI_MODE_FW_CONFIRM) {
    g_uiMode = UI_MODE_MENU;
    drawMenuScreen(true);
    return;
  }

 // Maintenance mode: short press = reboot back to normal
  if (g_uiMode == UI_MODE_MAINT) {
    Serial.println("[MAINT] Exit (reboot)");
    maintModeExit(true);
    return;
  }

  if (g_uiMode == UI_MODE_WIFI_INFO) {
    g_uiMode = UI_MODE_MENU;
    drawMenuScreen(true);
    return;
  }

  if (g_uiMode == UI_MODE_NORMAL) {
    enterMenu();
  } else if (g_uiMode == UI_MODE_MENU) {
    handleMenuSelect();
  } else if (g_uiMode == UI_MODE_TZ_SUB) {
    handleTimezoneSelect();
  } else if (g_uiMode == UI_MODE_COIN_SUB) {
    handleCoinSelect();
  }
}

void handleLongPress() {
 // Firmware update: long press confirms entry
  if (g_uiMode == UI_MODE_FW_CONFIRM) {
 // More robust: reboot into maintenance mode using an NVS boot flag.
 // (Avoids switching WiFi modes while other work may be in progress.)
    drawFirmwareUpdateApScreen(CRYPTOBAR_VERSION, "Rebooting to Update AP...", "");
    Serial.println("[MAINT] Request (reboot into update AP)");
    maintBootRequest();
    delay(80);
    ESP.restart();
  }

 // Maintenance mode: long press also exits
  if (g_uiMode == UI_MODE_MAINT) {
    Serial.println("[MAINT] Exit (reboot)");
    maintModeExit(true);
    return;
  }

 // V0.97: If WiFi dropped during normal use, do NOT auto-start AP.
 // User can long-press to manually start AP provisioning when offline.
  if (g_appState == APP_STATE_RUNNING && WiFi.status() != WL_CONNECTED && g_hasWifiCreds && g_uiMode == UI_MODE_NORMAL) {
    Serial.println("[WiFi] Manual AP provisioning requested (long press)");
    showWifiSetupRequired(0, false);
    return;
  }
  if (g_appState == APP_STATE_NEED_WIFI) return;

  if (g_uiMode == UI_MODE_MENU || g_uiMode == UI_MODE_TZ_SUB || g_uiMode == UI_MODE_COIN_SUB || g_uiMode == UI_MODE_WIFI_INFO) {
    leaveMenu();
  }
}

void handleFactoryReset() {
  Serial.println("[Menu] Factory reset - restore defaults.");

 // Defaults (match initial globals)
  g_updatePresetIndex      = 0;   // 30s
  g_updateIntervalMs       = UPDATE_PRESETS_MS[g_updatePresetIndex];
  g_brightnessPresetIndex  = 1;   // Med
  g_ledBrightness          = BRIGHTNESS_PRESETS[g_brightnessPresetIndex];
  ledStatusSetMasterBrightness(g_ledBrightness);
  g_currentCoinIndex       = coinIndexFromTicker(coinDefault().ticker);
  g_timeFormat             = TIME_12H;
  g_dateFormatIndex        = DATE_MM_DD_YYYY;
  g_timezoneIndex          = DEFAULT_TIMEZONE_INDEX;
  g_dayAvgMode           = DAYAVG_ROLLING;
  g_refreshMode            = 1;   // Full (default)

  applyTimezone();
  saveSettings();

 // Also clear saved WiFi creds so the device truly returns to "provisioning required"
  clearWifiCreds();

 // Stop any WiFi/AP activity and reboot cleanly
  wifiPortalStop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  ESP.restart();
}

// ==================== Encoder Polling =====================

void pollEncoder() {
  bool sw  = digitalRead(ENC_SW_PIN);
  unsigned long now = millis();

  if (sw != g_lastEncSw) {
    g_lastEncSw = sw;
    if (sw == LOW) {
      g_encPressStart = now;
    } else {
      unsigned long dur = now - g_encPressStart;
      if (dur >= 12000) {
        handleFactoryReset();
      } else if (dur >= 800) {
        handleLongPress();
      } else if (dur >= 30) {
        handleShortPress();
      }
    }
  }
}
