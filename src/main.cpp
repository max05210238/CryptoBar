// CryptoBar V0.99j (Price Precision Fix)
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <esp_sntp.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

#include "app_state.h"
#include "app_wifi.h"
#include "app_time.h"
#include "app_scheduler.h"
#include "app_input.h"
#include "app_menu.h"
#include "settings_store.h"
#include "led_status.h"
#include "coins.h"

#include "encoder_pcnt.h"
#include "wifi_portal.h"
#include "maint_mode.h"
#include "maint_boot.h"
#include "ota_guard.h"
#include <driver/gpio.h>
#include <driver/pcnt.h>

#include "config.h"
#include "chart.h"
#include "day_avg.h"
#include "network.h"
#include "ui.h"

#include <string.h> // for strcmp

// ==================== Safe defaults (prevent compilation failure if config.h is missing) =====================

// (WiFi SSID/PW are no longer hardcoded; they will be provisioned via AP portal and saved to NVS.)
#ifndef MARKET_GMT_OFFSET_SEC
// Default: US Eastern Time UTC-5 (DST not handled)
#define MARKET_GMT_OFFSET_SEC (-5 * 3600)
#endif

#ifndef MARKET_ANCHOR_HOUR_ET
// 7pm ET
#define MARKET_ANCHOR_HOUR_ET 19
#endif

// ==================== e-paper display =====================

GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(
  GxEPD2_290_BS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ==================== Helper Functions =====================

// Get current selected coin
const CoinInfo& currentCoin() {
  int n = coinCount();
  if (g_currentCoinIndex < 0) g_currentCoinIndex = 0;
  if (g_currentCoinIndex >= n) g_currentCoinIndex = n - 1;
  return coinAt(g_currentCoinIndex);
}


const char* dayAvgModeLabel(uint8_t mode) {
  switch (mode) {
    case DAYAVG_OFF:     return "Off";
    case DAYAVG_ROLLING: return "24h mean";
    case DAYAVG_CYCLE:   return "Cycle mean";
    default:             return "24h mean";
  }
}

void updateAvgLineReference(time_t nowUtc) {
  if (g_dayAvgMode == DAYAVG_OFF) {
    g_prevDayRefValid = false;
    return;
  }

  double mean = 0.0;
  bool ok = false;

  if (g_dayAvgMode == DAYAVG_CYCLE) {
    ok = dayAvgCycleMean(mean);
  } else {
 // Rolling 24h mean
    ok = dayAvgRollingGet(nowUtc, mean);
  }

  if (ok) {
    g_prevDayRefPrice = mean;
    g_prevDayRefValid = true;
  } else {
    g_prevDayRefValid = false;
  }
}

// LED functions moved to led_status.cpp (setLed*, updateLedForPrice, ledAnimLoop)
// WiFi functions moved to app_wifi.cpp (loadWifiCreds, connectWiFiSta, etc.)
// Time/scheduler functions moved to app_time.cpp & app_scheduler.cpp

// ==================== WiFi Setup UI =====================

void showWifiSetupRequired(unsigned long splashStartMs, bool enforceSplashDelay = true) {
 // Ensure the splash is visible for at least 3 seconds (first boot only).
  if (enforceSplashDelay) {
    unsigned long elapsed = millis() - splashStartMs;
    if (elapsed < 3000) delay(3000 - elapsed);
  }

 // Show a fixed "Preparing AP" screen for 10 seconds (total time),
 // then switch to the WiFi portal instructions screen.
  const uint32_t PREP_MS = 10000;
  uint32_t t0 = millis();

  drawWifiPreparingApScreen(CRYPTOBAR_VERSION);

  wifiPortalSetDefaultCoinTicker(coinAt(g_currentCoinIndex).ticker);
  

  wifiPortalSetDefaultAdvanced(

    g_refreshMode,

    g_updatePresetIndex,

    g_brightnessPresetIndex,

    g_timeFormat,

    g_dateFormatIndex,

    g_dtSize,

    g_displayCurrency,

    g_timezoneIndex,

    g_dayAvgMode

  );

  wifiPortalStart();
uint32_t elapsed = millis() - t0;
  if (elapsed < PREP_MS) delay(PREP_MS - elapsed);

 // AP IP is fixed by softAPConfig(), so we can show it directly.
  String apIp = "192.168.4.1";

  setLedPurple();
  drawWifiPortalScreen(CRYPTOBAR_VERSION, wifiPortalApSsid().c_str(), apIp.c_str());

  g_appState = APP_STATE_NEED_WIFI;
}

// ==================== Normal Operation Startup =====================

static void startNormalOperation(bool enforceSplashDelay, uint32_t splashStartMs) {
 // Stop portal/AP if it was running
  wifiPortalStop();
  WiFi.mode(WIFI_STA);
 // : disable WiFi power-save to reduce disconnects.
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  g_uiMode = UI_MODE_NORMAL;
  g_appState = APP_STATE_RUNNING;

 // V0.97: One-time best-effort timezone auto-detect on fresh devices.
 // This does NOT change the NTP sync logic; it only sets g_localUtcOffsetSec for display.
  autoDetectTimezoneIfNeeded();

  setupTime();


 // V0.99f: Fetch FX rates for all currencies (except USD) after WiFi+NTP ready
  if (g_displayCurrency != (int)CURR_USD && WiFi.status() == WL_CONNECTED) {
    if (fetchExchangeRates()) {
      g_fxValid = true;
      Serial.printf("[FX] Multi-currency rates fetched (display: %s)\n",
                    CURRENCY_INFO[g_displayCurrency].code);
    } else {
      g_fxValid = false;
      Serial.println("[FX] Multi-currency fetch failed");
    }
    g_nextFxUpdateUtc = time(nullptr) + 3600;  // 1 hour (V0.99f: reduced API load)
  }
  updateEtCycle();
  bootstrapHistoryFromKrakenOHLC();

  double price  = 0.0;
  double change = 0.0;

  g_lastPriceOk = fetchPrice(price, change);
  if (g_lastPriceOk) {
    g_lastPriceUsd  = price;
    g_lastChange24h = change;

    time_t nowUtc = time(nullptr);
    if (nowUtc > 100000) {
      dayAvgRollingAdd(nowUtc, price);
    }
    updateAvgLineReference(nowUtc);


    addChartSampleForNow(price);
  } else {
    g_prevDayRefValid = false;
    g_lastPriceUsd = 0.0f;
    g_lastChange24h = 0.0f;
  }

  updateLedForPrice(g_lastChange24h, g_lastPriceOk);

 // ensure the LED animator task has up-to-date context before any long draw/refresh.
  ledAnimLoop(g_appState == APP_STATE_RUNNING, g_lastPriceOk);

  if (enforceSplashDelay) {
    uint32_t elapsed = millis() - splashStartMs;
    if (elapsed < 3000) delay(3000 - elapsed);
  }

  drawMainScreen(g_lastPriceUsd, g_lastChange24h, true);

  g_partialRefreshCount = 0;
  lastUpdate = millis();
}

// ==================== Settings & Menu handlers moved to app_menu.cpp =====================

// ==================== Maintenance mode (firmware update AP) =====================

static void enterMaintenanceMode(bool fromBoot) {
  Serial.println("[MAINT] Enter");

 // When coming from normal operation, tear down any provisioning portal and STA
 // cleanly before switching to the Maintenance AP.
 // When coming from a cold boot (reboot-into-maint), avoid an extra WIFI_OFF
 // transition here — maintModeEnter() will configure AP mode itself.
  if (!fromBoot) {
 // Ensure provisioning portal is fully stopped before starting our own server.
    wifiPortalStop();
    delay(50);

 // Tear down STA cleanly before switching to AP-only.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(80);
  }

  maintModeEnter(CRYPTOBAR_VERSION);
  g_appState = APP_STATE_MAINT;
  g_uiMode   = UI_MODE_MAINT;
  setLedPurple();
  drawFirmwareUpdateApScreen(CRYPTOBAR_VERSION, maintModeApSsid().c_str(), maintModeApIp().c_str());
}

// : Request maintenance mode via reboot (more stable than switching modes at runtime).
static void requestMaintenanceModeReboot() {
  Serial.println("[MAINT] Request (reboot into update AP)");
  maintBootRequest();
  delay(80);
  ESP.restart();
}


static void exitMaintenanceModeToNormal() {
  Serial.println("[MAINT] Exit requested (reboot to normal mode)");
 // Fully reboot to guarantee clean WiFi/netif teardown.
 // (Avoids occasional: wifi_init_default: netstack cb reg failed)
  maintModeExit(true);
}

// ==================== Input handlers moved to app_input.cpp =====================

// ==================== setup & loop =====================

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(10);
  }

  Serial.println();
  Serial.println("=== CryptoBar ===");
  Serial.printf("[Version] %s\n", CRYPTOBAR_VERSION);

 // OTA safety guard: if a freshly-updated firmware keeps rebooting before it
 // stabilizes, automatically roll back to the previous slot.
  otaGuardBootBegin();

  loadSettings();

 // LEDs (external + onboard)
  ledStatusBegin(NEOPIXEL_PIN, NEOPIXEL_COUNT, BOARD_RGB_PIN, BOARD_RGB_COUNT);
  ledStatusSetMasterBrightness(g_ledBrightness);
  setLedBlue();

 // keep LED breathe animating even while the main loop is blocked (e-paper refresh)
 // and sync phase across devices (when NTP time becomes valid).
  ledAnimStartTask(30, 0);

  SPI.begin(EPD_SCK, -1, EPD_MOSI);
  display.init(115200);
  display.setRotation(1);

 // ==================== Boot welcome screen =====================
 // Show version to user first; screen stays visible while WiFi/NTP connection takes time
  drawSplashScreen(CRYPTOBAR_VERSION);
  uint32_t splashStartMs = millis();

  // Encoder button pin (CLK/DT pins are configured inside encoderPcntBegin)
  pinMode(ENC_SW_PIN,  INPUT_PULLUP);
  g_lastEncSw = digitalRead(ENC_SW_PIN);

 // Use PCNT hardware for encoder rotation (no GPIO interrupt, won't be blocked by screen/network)
  encoderPcntBegin(ENC_CLK_PIN, ENC_DT_PIN);

 // : enter maintenance mode via reboot flag (requested from UI).
  if (maintBootConsumeRequested()) {
    Serial.println("[MAINT] Boot request detected");
    drawFirmwareUpdateApScreen(CRYPTOBAR_VERSION, "Starting Update AP...", "");
    enterMaintenanceMode(true);
    return;
  }

// Load WiFi credentials from NVS (no more hardcoded defaults)
loadWifiCreds();

if (!g_hasWifiCreds) {
  Serial.println("[WiFi] No credentials saved. Waiting for provisioning...");
  showWifiSetupRequired(splashStartMs);
  return;
}

if (!connectWiFiStaWithRetries(g_wifiSsid.c_str(), g_wifiPass.c_str(),
                              5, 12000, true)) {
  Serial.println("[WiFi] Failed to connect with saved credentials (all attempts).");
  showWifiSetupRequired(splashStartMs);
  return;
}

startNormalOperation(true, splashStartMs);
}

void loop() {
  unsigned long now = millis();

 // V0.99i: Track consecutive identical price updates to detect stale API data
  static double s_lastFetchedPrice = 0.0;
  static int    s_duplicatePriceCount = 0;

 // If we are in an OTA pending state (freshly updated firmware), mark it as
 // "valid" after it has been running stably for a short time.
  otaGuardLoop();

 // Keep external NeoPixel alive + keep onboard WS2812 forced-off.
  ledStatusService();

 // V0.97: hardware PCNT rotation decode (clears counts even during provisioning)
  encoderPcntPoll(g_appState == APP_STATE_RUNNING, &g_encStepAccum, &g_encMux);

 // button
  pollEncoder();

 // ===== Maintenance mode (firmware update AP) =====
  if (g_appState == APP_STATE_MAINT) {
    maintModeLoop();
    if (maintModeConsumeExitRequest()) {
      exitMaintenanceModeToNormal();
    }
    delay(5);
    return;
  }

 // ===== WiFi provisioning portal =====
  if (g_appState == APP_STATE_NEED_WIFI) {
    wifiPortalLoop();

    WifiPortalSubmission sub;
    if (wifiPortalTakeSubmission(sub)) {
 // Save to NVS
      saveWifiCreds(sub.ssid, sub.pass);
      g_wifiSsid = sub.ssid;
      g_wifiPass = sub.pass;
      g_wifiSsid.trim();
      g_wifiPass.trim();
      g_hasWifiCreds = (g_wifiSsid.length() > 0);

      Serial.printf("[WiFi] Provisioned SSID: %s\n", g_wifiSsid.c_str());

      bool anySettingChanged = false;

 // Optional: coin selection from portal dropdown
      String submittedCoin = sub.coinTicker;
      submittedCoin.trim();
      if (submittedCoin.length() > 0) {
        int ci = coinIndexFromTicker(submittedCoin.c_str());
        if (ci >= 0 && ci < coinCount()) {
          g_currentCoinIndex = ci;
          anySettingChanged = true;
          Serial.printf("[Settings] Provisioned coin: %s\n", coinAt(g_currentCoinIndex).ticker);
        }
      }

 // Optional: Date/Time Size (dtSize)
      if (sub.dtSize == 0 || sub.dtSize == 1) {
        g_dtSize = sub.dtSize;
        anySettingChanged = true;
        Serial.printf("[Settings] Provisioned dtSize: %s\n", DTSIZE_LABELS[g_dtSize]);
      }

      if (anySettingChanged) {
        saveSettings();
      }



 // Start STA connect (keep AP up)
      g_appState = APP_STATE_CONNECTING;
      g_staConnectStartMs = millis();

      WiFi.mode(WIFI_AP_STA);
 // : disable WiFi power-save to keep STA stable during provisioning.
      WiFi.setSleep(false);
      WiFi.setAutoReconnect(true);
      WiFi.disconnect(false);  // disconnect STA only (keep radio on)
      delay(50);
      WiFi.begin(g_wifiSsid.c_str(), g_wifiPass.c_str());
      setLedBlue();
      drawWifiConnectingScreen(CRYPTOBAR_VERSION, g_wifiSsid.c_str());
    }

    delay(5);
    return;
  }


  if (g_appState == APP_STATE_CONNECTING) {
    wifiPortalLoop();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[WiFi] Connected, IP: ");
      Serial.println(WiFi.localIP());

      g_wifiEverConnected = true;
      g_nextRuntimeReconnectMs = 0;
      g_runtimeReconnectBatch = 0;

 // Done provisioning: shut down AP/portal and enter normal operation
      startNormalOperation(false, 0);
      return;
    }

    if ((millis() - g_staConnectStartMs) > STA_CONNECT_TIMEOUT_MS) {
      Serial.println("[WiFi] Connect FAILED (timeout) - restarting portal");
      setLedRed();
      drawWifiConnectFailedScreen(CRYPTOBAR_VERSION);

      WiFi.disconnect(false);
      delay(800);

 // Restart AP portal and show instructions again
      wifiPortalStop();
      delay(150);

      drawWifiPreparingApScreen(CRYPTOBAR_VERSION);
      wifiPortalSetDefaultCoinTicker(coinAt(g_currentCoinIndex).ticker);
  

  wifiPortalSetDefaultAdvanced(

    g_refreshMode,

    g_updatePresetIndex,

    g_brightnessPresetIndex,

    g_timeFormat,

    g_dateFormatIndex,

    g_dtSize,

    g_displayCurrency,

    g_timezoneIndex,

    g_dayAvgMode

  );

  wifiPortalStart();
String apIp = WiFi.softAPIP().toString();
      setLedPurple();
      drawWifiPortalScreen(CRYPTOBAR_VERSION, wifiPortalApSsid().c_str(), apIp.c_str());
      g_appState = APP_STATE_NEED_WIFI;
      return;
    }

    delay(10);
    return;
  }

 // ===== consume encoder steps captured by ISR =====
  int steps = 0;
  portENTER_CRITICAL(&g_encMux);
  steps = g_encStepAccum;
  g_encStepAccum = 0;
  portEXIT_CRITICAL(&g_encMux);

  if (steps != 0) {
    if (g_uiMode == UI_MODE_MENU) {
      g_menuIndex += steps;
      while (g_menuIndex < 0) g_menuIndex += MENU_COUNT;
      while (g_menuIndex >= MENU_COUNT) g_menuIndex -= MENU_COUNT;
      ensureMainMenuVisible();
      g_menuDirty = true;
    } else if (g_uiMode == UI_MODE_TZ_SUB) {
      g_tzMenuIndex += steps;
      while (g_tzMenuIndex < 0) g_tzMenuIndex += TIMEZONE_COUNT;
      while (g_tzMenuIndex >= TIMEZONE_COUNT) g_tzMenuIndex -= TIMEZONE_COUNT;
      ensureTzMenuVisible();
      g_tzDirty = true;
    } else if (g_uiMode == UI_MODE_COIN_SUB) {
      g_coinMenuIndex += steps;
      int n = coinCount();
      while (g_coinMenuIndex < 0) g_coinMenuIndex += n;
      while (g_coinMenuIndex >= n) g_coinMenuIndex -= n;
      ensureCoinMenuVisible();
      g_coinDirty = true;
    } else if (g_uiMode == UI_MODE_CURRENCY_SUB) {
      g_currencyMenuIndex += steps;
      while (g_currencyMenuIndex < 0) g_currencyMenuIndex += (int)CURR_COUNT;
      while (g_currencyMenuIndex >= (int)CURR_COUNT) g_currencyMenuIndex -= (int)CURR_COUNT;
      ensureCurrencyMenuVisible();
      g_currencyDirty = true;
    }
  }

 // ===== throttle menu redraw =====
  uint32_t ms = millis();
  if (ms - g_lastUiDrawMs >= UI_DRAW_MIN_MS) {
    if (g_uiMode == UI_MODE_MENU && g_menuDirty) {
      drawMenuScreen(false);
      g_menuDirty = false;
      g_lastUiDrawMs = ms;
    } else if (g_uiMode == UI_MODE_TZ_SUB && g_tzDirty) {
      drawTimezoneMenu(false);
      g_tzDirty = false;
      g_lastUiDrawMs = ms;
    } else if (g_uiMode == UI_MODE_COIN_SUB && g_coinDirty) {
      drawCoinMenu(false);
      g_coinDirty = false;
      g_lastUiDrawMs = ms;
    } else if (g_uiMode == UI_MODE_CURRENCY_SUB && g_currencyDirty) {
      drawCurrencyMenu(false);
      g_currencyDirty = false;
      g_lastUiDrawMs = ms;
    }
  }
 // ==================== Runtime WiFi drop handling (V0.97) ====================
 // If WiFi drops during normal use, DO NOT auto-start AP.
 // We retry STA in small batches with a backoff. AP can be started manually via long-press while offline.
  if (WiFi.status() != WL_CONNECTED) {
 // If somehow we never had a successful connection (e.g., boot edge-case), keep the original behavior.
    if (!g_wifiEverConnected) {
      if (!connectWiFiStaWithRetries(g_wifiSsid.c_str(), g_wifiPass.c_str(), 5, 12000, true)) {
        Serial.println("[WiFi] Failed to connect with saved credentials (all attempts).");
        showWifiSetupRequired(0, false);
        return;
      }
    } else {
      uint32_t nowMs = millis();
 // Only attempt a reconnect when the backoff window expires.
      if (g_nextRuntimeReconnectMs == 0 || nowMs >= g_nextRuntimeReconnectMs) {
        bool showUi = (g_uiMode == UI_MODE_NORMAL);
        if (showUi) {
          String label = String(g_wifiSsid) + " (reconnect)";
          drawWifiConnectingScreen(CRYPTOBAR_VERSION, label.c_str());
        }
        Serial.printf("[WiFi] Runtime reconnect batch %u (attempts=%u, timeout=%lums)\n",
                      (unsigned)g_runtimeReconnectBatch + 1, (unsigned)RUNTIME_RECONNECT_ATTEMPTS,
                      (unsigned long)RUNTIME_RECONNECT_TIMEOUT_MS);
        bool ok = connectWiFiStaWithRetries(g_wifiSsid.c_str(), g_wifiPass.c_str(),
                                           RUNTIME_RECONNECT_ATTEMPTS, RUNTIME_RECONNECT_TIMEOUT_MS, showUi);
        if (!ok) {
          g_runtimeReconnectBatch++;
          g_nextRuntimeReconnectMs = nowMs + RUNTIME_RECONNECT_BACKOFF_MS;
          Serial.printf("[WiFi] Runtime reconnect failed. Next retry in %lus (no AP auto-start).\n",
                      (unsigned long)(RUNTIME_RECONNECT_BACKOFF_MS / 1000UL));
          if (showUi) {
            setLedRed();
            String label = String("Offline, retry in ") + String(RUNTIME_RECONNECT_BACKOFF_MS / 1000UL) + "s";
            drawWifiConnectingScreen(CRYPTOBAR_VERSION, label.c_str());
          }
 // Skip network work this loop, but keep LED animation alive.
          ledAnimLoop(g_appState == APP_STATE_RUNNING, g_lastPriceOk);
          delay(1);
          return;
        }
 // Success: counters are reset inside connectWiFiSta() success path
      } else {
 // Still in backoff window — skip network work this loop.
        ledAnimLoop(g_appState == APP_STATE_RUNNING, g_lastPriceOk);
        delay(1);
        return;
      }
    }
  }

 // ===== periodic updates (tick-aligned once time is valid) =====
  uint32_t intervalSec = updateIntervalSec();
  time_t nowUtc = time(nullptr);

 // V0.97) Handle SNTP time-sync notification (lwIP callback)
  if (g_ntpEventPending) {
    int64_t utc = 0;
    portENTER_CRITICAL(&g_ntpMux);
    utc = g_ntpEventUtc;
    g_ntpEventPending = false;
    g_ntpEventUtc = 0;
    portEXIT_CRITICAL(&g_ntpMux);

    if ((time_t)utc >= TIME_VALID_MIN_UTC) {      // Log in device-selected timezone (manual UTC offset) for consistency with main clock display.
      time_t localSec = (time_t)utc + g_localUtcOffsetSec;
      struct tm local;
      if (gmtime_r(&localSec, &local)) {
        Serial.printf("[Time][SNTP] Sync event: %02d:%02d  %02d/%02d/%04d\n",
                      local.tm_hour, local.tm_min,
                      local.tm_mon + 1, local.tm_mday, local.tm_year + 1900);
      } else {
        Serial.printf("[Time][SNTP] Sync event: utc=%lld\n", (long long)utc);
      }
 // Re-align update ticks after time correction so multiple stacked units stay in phase.
      tickSchedulerReset("SNTP");
      ntpResyncReset("SNTP");
      nowUtc = time(nullptr); // refresh after any smooth correction
    }
  }

 // --- Prefetch-to-tick () ---
 // If we're within a small lead window before the next scheduled tick, fetch
 // the price early. When the tick triggers, reuse the prefetched result so
 // multiple units "start" the screen refresh closer together.
  if (nowUtc >= TIME_VALID_MIN_UTC && g_timeValid && g_nextUpdateUtc > 0) {
 // New tick target? drop stale prefetch.
    if (g_prefetchValid && g_prefetchForUtc != g_nextUpdateUtc) {
      g_prefetchValid = false;
      g_prefetchForUtc = 0;
    }

    time_t prefetchAtUtc = g_nextUpdateUtc - (time_t)PREFETCH_FIXED_LEAD_SEC;
    time_t earliestPrefetchAt = g_nextUpdateUtc - (time_t)PREFETCH_WINDOW_SEC;
    time_t latestPrefetchAt   = g_nextUpdateUtc - (time_t)PREFETCH_MIN_LEAD_SEC;
    if (prefetchAtUtc < earliestPrefetchAt) prefetchAtUtc = earliestPrefetchAt;
    if (prefetchAtUtc > latestPrefetchAt)   prefetchAtUtc = latestPrefetchAt;
    if (!g_prefetchValid && nowUtc >= prefetchAtUtc && nowUtc < g_nextUpdateUtc && WiFi.status() == WL_CONNECTED) {
      float p = 0.0f;
      float c = 0.0f;
      Serial.printf("[Prefetch] Tick=%ld (in %ld s) prefetchAt=%ld (lead=%lu)\n",
                    (long)g_nextUpdateUtc,
                    (long)(g_nextUpdateUtc - nowUtc),
                    (long)prefetchAtUtc,
                    (unsigned long)((g_nextUpdateUtc > prefetchAtUtc) ? (g_nextUpdateUtc - prefetchAtUtc) : 0));
      bool ok = fetchPrice(p, c);
      if (ok) {
        g_prefetchPrice = p;
        g_prefetchChange = c;
        g_prefetchForUtc = g_nextUpdateUtc;
        g_prefetchValid = true;
        Serial.println("[Prefetch] OK");
      } else {
        Serial.println("[Prefetch] FAILED");
      }
    }
  }

  bool doUpdate = false;

  if (nowUtc >= TIME_VALID_MIN_UTC) {
    if (!g_timeValid || g_nextUpdateUtc == 0) {
      tickSchedulerReset("AUTO");
    }
    if (g_timeValid && g_nextUpdateUtc > 0 && nowUtc >= g_nextUpdateUtc) {
      doUpdate = true;
    }
  } else {
 // Fallback (time not synced yet)
    if (now - lastUpdate >= g_updateIntervalMs) {
      doUpdate = true;
    }
  }

 // -------------------- Periodic FX update (Multi-currency) --------------------
  // V0.99f: Update FX rates hourly for all non-USD currencies
  if (!doUpdate && g_timeValid && WiFi.status() == WL_CONNECTED && g_displayCurrency != (int)CURR_USD) {
    time_t nowFxUtc = time(nullptr);
    if (g_nextFxUpdateUtc == 0 || nowFxUtc >= g_nextFxUpdateUtc) {
      if (fetchExchangeRates()) {
        g_fxValid = true;
        Serial.printf("[FX] Multi-currency rates updated (display: %s, rate: %.4f)\n",
                      CURRENCY_INFO[g_displayCurrency].code, g_usdToRate[g_displayCurrency]);
      } else {
        g_fxValid = false;
        Serial.println("[FX] Multi-currency update failed");
      }
      g_nextFxUpdateUtc = nowFxUtc + 3600; // 1 hour (V0.99f: reduced API load)
    }
  }

if (doUpdate) {
    lastUpdate = now;

    time_t thisTickUtc = 0;  // the boundary this update corresponds to (UTC)

 // Advance schedule before long network work to avoid double-trigger
    if (nowUtc >= TIME_VALID_MIN_UTC) {
      logTickInfo("UPDATE", intervalSec);
      if (g_nextUpdateUtc == 0) {
        g_nextUpdateUtc = alignNextTickUtc(nowUtc, intervalSec);
      }
      thisTickUtc = g_nextUpdateUtc;
      int guard = 0;
      while (nowUtc >= g_nextUpdateUtc && guard < 8) {
        g_nextUpdateUtc += (time_t)intervalSec;
        guard++;
      }
      if (guard >= 8) {
        g_nextUpdateUtc = alignNextTickUtc(nowUtc, intervalSec);
      }
    }

    double price  = 0.0;
    double change = 0.0;
    bool ok = false;
 // Use prefetched data if it matches the tick we're committing now.
    if (thisTickUtc != 0 && g_prefetchValid && g_prefetchForUtc == thisTickUtc) {
      price  = g_prefetchPrice;
      change = g_prefetchChange;
      ok = true;
      g_prefetchValid = false;
      g_prefetchForUtc = 0;
      Serial.println("[Prefetch] Using cached result for tick");
    } else {
      ok = fetchPrice(price, change);
    }

    g_lastPriceOk = ok;

    if (ok) {
      // V0.99i: Track duplicate prices for diagnostics (log only, always refresh display)
      const double PRICE_EPSILON = 0.0001;  // tolerance for floating-point comparison

      if (s_lastFetchedPrice > 0.0 && fabs(price - s_lastFetchedPrice) < PRICE_EPSILON) {
        s_duplicatePriceCount++;
        Serial.printf("[Price] Duplicate #%d: $%.6f (no change)\n", s_duplicatePriceCount, price);

        // Warning: if price hasn't changed for 3+ consecutive updates, API might be stale
        if (s_duplicatePriceCount == 3) {
          Serial.println("[Price] WARNING: Price unchanged for 3 updates - possible stale API data");
        }
      } else {
        if (s_duplicatePriceCount > 0) {
          Serial.printf("[Price] CHANGED after %d duplicates: $%.6f -> $%.6f\n",
                        s_duplicatePriceCount, s_lastFetchedPrice, price);
        }
        s_duplicatePriceCount = 0;
      }
      s_lastFetchedPrice = price;

      g_lastPriceUsd  = price;
      g_lastChange24h = change;
      if (nowUtc >= TIME_VALID_MIN_UTC) {
        dayAvgRollingAdd(nowUtc, price);
      }
      updateAvgLineReference(nowUtc);

      addChartSampleForNow(price);
      updateLedForPrice(g_lastChange24h, g_lastPriceOk);

      if (g_uiMode == UI_MODE_NORMAL) {
 // Refresh mode rules:
 // - Partial mode: accumulate partials, full refresh after PARTIAL_REFRESH_LIMIT
 // - Full mode: always do full refresh on main screen
        bool doFull = false;
        if (g_refreshMode == 1) {
          doFull = true;
        } else {
          doFull = (g_partialRefreshCount >= PARTIAL_REFRESH_LIMIT);
        }

        drawMainScreen(g_lastPriceUsd, g_lastChange24h, doFull);

        if (g_refreshMode == 0) {
          if (doFull) g_partialRefreshCount = 0;
          else        g_partialRefreshCount++;
        } else {
          g_partialRefreshCount = 0;
        }
      }
    } else {
      fadeLedTo(100, 0, 120);  // API fail → purple
    }
  }


 // V0.97) Force a resync request on a shared 10-minute boundary
 // (fallback for cases where the core's internal SNTP interval doesn't fire reliably)
 // Do this AFTER any price/network update to avoid overlapping network work.
  if (nowUtc >= TIME_VALID_MIN_UTC) {
    time_t now2 = time(nullptr);
    if (g_nextNtpResyncUtc != 0 && now2 >= g_nextNtpResyncUtc) {
      if (WiFi.status() == WL_CONNECTED) {
        requestNtpResync("SCHED");
        ntpResyncReset("SCHED");
      } else {
 // If offline, keep trying (do not advance schedule) so it re-syncs immediately once WiFi returns.
        Serial.println("[Time][SNTP] Resync due, but WiFi is offline.");
 // Backoff to avoid spamming the serial log; retry soon.
        g_nextNtpResyncUtc = now2 + 30;
      }
    }
  }

  ledAnimLoop(g_appState == APP_STATE_RUNNING, g_lastPriceOk);

  delay(1); // Smoother than delay(10)
}
