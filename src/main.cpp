// CryptoBar V0.97 (Maintenance boot flag + WiFi sleep off)
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

// ==================== 安全預設（避免 config.h 缺值時編譯失敗） =====================

// (WiFi SSID/PW are no longer hardcoded; they will be provisioned via AP portal and saved to NVS.)
#ifndef MARKET_GMT_OFFSET_SEC
// 預設為美東 UTC-5（未處理 DST）
#define MARKET_GMT_OFFSET_SEC (-5 * 3600)
#endif

#ifndef MARKET_ANCHOR_HOUR_ET
// 7pm ET
#define MARKET_ANCHOR_HOUR_ET 19
#endif

// ==================== Helper functions for scheduler =====================

static inline uint32_t updateIntervalSec() {
  uint32_t sec = (uint32_t)(g_updateIntervalMs / 1000UL);
  if (sec < 30) sec = 30;
  return sec;
}

static inline bool isTimeValidNow() {
  time_t t = time(nullptr);
  return (t >= TIME_VALID_MIN_UTC);
}

static inline time_t alignNextTickUtc(time_t nowUtc, uint32_t intervalSec) {
  if (intervalSec < 1) intervalSec = 1;
 // Next multiple of intervalSec (epoch-aligned)
  return ((nowUtc / (time_t)intervalSec) + 1) * (time_t)intervalSec;
}

static inline uint32_t macLast16bits() {
 // e.g. "AA:BB:CC:DD:EE:FF" -> take last 2 bytes (EEFF)
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  if (mac.length() < 4) return 0;
  String last4 = mac.substring(mac.length() - 4);
  char buf[5];
  last4.toCharArray(buf, sizeof(buf));
  return (uint32_t)strtoul(buf, nullptr, 16);
}

static inline void initFetchJitterIfNeeded() {
 // : Disable per-device fetch jitter to maximize multi-unit sync.
  g_fetchJitterSec = 0;
}
static void tickSchedulerReset(const char* reason) {
  time_t nowUtc = time(nullptr);
  if (nowUtc < TIME_VALID_MIN_UTC) {
    g_timeValid = false;
    g_nextUpdateUtc = 0;
    g_prefetchValid = false;
    g_prefetchForUtc = 0;
    g_nextFxUpdateUtc = 0;
    Serial.printf("[Sched] Reset(%s): time not valid\n", reason ? reason : "");
    return;
  }

  g_timeValid = true;
  uint32_t sec = updateIntervalSec();
  g_nextUpdateUtc = alignNextTickUtc(nowUtc, sec);
  g_nextFxUpdateUtc = alignNextTickUtc(nowUtc, 300);

 // New schedule → discard any pending prefetch.
  g_prefetchValid = false;
  g_prefetchForUtc = 0;
  Serial.printf("[Sched] Reset(%s): int=%lus nextUpdateUtc=%ld (in %ld s)\n",
                reason ? reason : "", (unsigned long)sec,
                (long)g_nextUpdateUtc, (long)(g_nextUpdateUtc - nowUtc));
}

static void loadWifiCreds() {
  settingsStoreLoadWifi(g_wifiSsid, g_wifiPass);

  g_wifiSsid.trim();
  g_wifiPass.trim();
  g_hasWifiCreds = (g_wifiSsid.length() > 0);
  Serial.printf("[WiFi] NVS creds: %s\n", g_hasWifiCreds ? "FOUND" : "MISSING");
}

static void saveWifiCreds(const String& ssid, const String& pass) {
  settingsStoreSaveWifi(ssid, pass);
}

static void clearWifiCreds() {
  settingsStoreClearWifi();

  g_wifiSsid = "";
  g_wifiPass = "";
  g_hasWifiCreds = false;
  Serial.println("[WiFi] Cleared saved credentials (factory reset)");
}


static int rssiToBars(int rssi) {
  if (rssi >= -55) return 5;
  if (rssi >= -62) return 4;
  if (rssi >= -69) return 3;
  if (rssi >= -76) return 2;
  if (rssi >= -83) return 1;
  return 0;
}

// ==================== e-paper display =====================

GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(
  GxEPD2_290_BS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ==================== 小工具函式 =====================

// 目前幣別
const CoinInfo& currentCoin() {
  int n = coinCount();
  if (g_currentCoinIndex < 0) g_currentCoinIndex = 0;
  if (g_currentCoinIndex >= n) g_currentCoinIndex = n - 1;
  return coinAt(g_currentCoinIndex);
}


static const char* dayAvgModeLabel(uint8_t mode) {
  switch (mode) {
    case DAYAVG_OFF:     return "Off";
    case DAYAVG_ROLLING: return "24h mean";
    case DAYAVG_CYCLE:   return "Cycle mean";
    default:             return "24h mean";
  }
}

static void updateAvgLineReference(time_t nowUtc) {
  if (g_dayAvgMode == DAYAVG_OFF) {
    g_prevDayRefValid = false;
    return;
  }

  float mean = 0.0f;
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

// 本地時間（使用 g_localUtcOffsetSec）
bool getLocalTimeLocal(struct tm* out) {
  time_t nowUtc = time(nullptr);
  if (nowUtc < 100000) return false;

  time_t localSec = nowUtc + g_localUtcOffsetSec;
  struct tm* t = gmtime(&localSec);
  if (!t) return false;

 *out = *t;
  return true;
}

// LED functions moved to led_status.cpp (setLed*, updateLedForPrice, ledAnimLoop)

// ==================== WiFi & Time =====================

bool connectWiFiSta(const char* ssid, const char* pass, uint32_t timeoutMs = 30000) {
  if (!ssid || !ssid[0]) return false;

  WiFi.mode(WIFI_STA);
 // : disable WiFi power-save to reduce connection instability.
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  initFetchJitterIfNeeded();
  if (g_fetchJitterSec == 0) {
    Serial.printf("[Sched] Fetch jitter DISABLED (MAC tail=0x%04lX)\n", (unsigned long)macLast16bits());
  } else {
    Serial.printf("[Sched] Fetch jitter=%lu s (MAC tail=0x%04lX)\n", (unsigned long)g_fetchJitterSec, (unsigned long)macLast16bits());
  }
  WiFi.disconnect(true);
  delay(100);

  Serial.printf("[WiFi] Connecting to %s\n", ssid);
  WiFi.begin(ssid, (pass ? pass : ""));
  setLedBlue();

  uint32_t start = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
    Serial.print(".");
    dots++;
    if (dots % 40 == 0) Serial.println();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected, IP: ");
    Serial.println(WiFi.localIP());

 // Mark that we have successfully connected at least once.
    g_wifiEverConnected = true;
    g_nextRuntimeReconnectMs = 0;
    g_runtimeReconnectBatch = 0;

    return true;
  }

  Serial.println("[WiFi] Connect FAILED (timeout)");
  WiFi.disconnect(true);
  return false;
}

// ==================== WiFi connect retries (V0.97) =====================
// 需求：如果 NVS 有 SSID/PW，第一次連線 timeout 後不要立刻開 AP；改為多次嘗試。
// - 每次嘗試前更新畫面：Connecting... (i/N)
// - 全部失敗後才進入 AP provisioning
static bool connectWiFiStaWithRetries(const char* ssid, const char* pass,
                                     int maxAttempts,
                                     uint32_t perAttemptTimeoutMs,
                                     bool showUi)
{
  if (!ssid || !ssid[0]) return false;
  if (maxAttempts < 1) maxAttempts = 1;

  for (int i = 1; i <= maxAttempts; i++) {
    if (showUi) {
 // 在 SSID 後面附上 (i/N) 讓 UI 顯示嘗試次數（不改 UI 函式簽名）
      String label = String(ssid) + " (" + String(i) + "/" + String(maxAttempts) + ")";
      drawWifiConnectingScreen(CRYPTOBAR_VERSION, label.c_str());
    }

    Serial.printf("[WiFi] STA attempt %d/%d (timeout=%lums)\n",
                  i, maxAttempts, (unsigned long)perAttemptTimeoutMs);

    if (connectWiFiSta(ssid, pass, perAttemptTimeoutMs)) {
      return true;
    }

 // 小延遲，避免太快重複 begin() 造成掃描/連線不穩
    delay(300);
  }

  return false;
}

// Forward declarations (needed because auto TZ helpers are defined before these functions).
void applyTimezone();
void saveSettings();

// ==================== Auto-detect timezone (V0.97) =====================
// Goal:
// - Keep existing time sync (NTP) logic unchanged.
// - When tzIndex key is missing in NVS (fresh device / after clear), attempt to detect
// the integer UTC offset once after WiFi connects, then save tzIndex.
// - This is best-effort; if it fails we keep DEFAULT_TIMEZONE_INDEX (Seattle).

static int findTzIndexByOffsetHours(int8_t offsetHours) {
  for (int i = 0; i < (int)TIMEZONE_COUNT; ++i) {
    if (TIMEZONES[i].utcOffsetHours == offsetHours) return i;
  }
  return -1;
}

static bool fetchUtcOffsetHoursFromWorldTimeApi(int8_t& outOffsetHours) {
  if (WiFi.status() != WL_CONNECTED) return false;

  const char* urls[] = {
    "http://worldtimeapi.org/api/ip",
    "https://worldtimeapi.org/api/ip"
  };

  for (size_t u = 0; u < sizeof(urls) / sizeof(urls[0]); ++u) {
    HTTPClient http;
    http.begin(urls[u]);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(8000);

    Serial.print("[TZ][Auto] GET ");
    Serial.println(urls[u]);

    int code = http.GET();
    if (code != 200) {
      Serial.printf("[TZ][Auto] HTTP error: %d\n", code);
      http.end();
      continue;
    }

    String payload = http.getString();
    http.end();

 // Parse only the utc_offset field to keep memory low.
    StaticJsonDocument<96> filter;
    filter["utc_offset"] = true;

    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (err) {
      Serial.printf("[TZ][Auto] JSON parse error: %s\n", err.c_str());
      continue;
    }

    const char* off = doc["utc_offset"] | "";
    if (!off || off[0] == '\0') {
      Serial.println("[TZ][Auto] utc_offset missing");
      continue;
    }

 // Expected format: "+08:00" or "-05:00".
    char sign = off[0];
    int hh = atoi(off + 1);
    int mm = 0;
    const char* colon = strchr(off, ':');
    if (colon && colon[1] && colon[2]) {
      mm = atoi(colon + 1);
    }

 // V0.97 integer-only timezone support.
    if (mm != 0) {
      Serial.printf("[TZ][Auto] Non-integer offset not supported: %s\n", off);
      return false;
    }

    if (sign == '-') hh = -hh;
    if (hh < -12 || hh > 14) {
      Serial.printf("[TZ][Auto] Offset out of range: %d (raw=%s)\n", hh, off);
      return false;
    }

    outOffsetHours = (int8_t)hh;
    return true;
  }

  return false;
}

static void autoDetectTimezoneIfNeeded() {
  if (g_tzAutoAttempted) return;
  g_tzAutoAttempted = true;

  if (g_tzIndexKeyPresent) {
    Serial.println("[TZ][Auto] tzIndex already set in NVS; skip");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TZ][Auto] WiFi not connected; skip");
    return;
  }

  Serial.println("[TZ][Auto] Detecting timezone offset (best-effort)...");

  int8_t offHours = 0;
  if (!fetchUtcOffsetHoursFromWorldTimeApi(offHours)) {
    Serial.println("[TZ][Auto] Detect failed; keep default");
    return;
  }

  int tzIdx = findTzIndexByOffsetHours(offHours);
  if (tzIdx < 0) {
    Serial.printf("[TZ][Auto] No matching tzIndex for UTC%+d; keep default\n", (int)offHours);
    return;
  }

  g_timezoneIndex = tzIdx;
  applyTimezone();
  saveSettings();
  g_tzIndexKeyPresent = true;
  g_tzIndexKeyPresentAtBoot = true;

  Serial.printf("[TZ][Auto] Applied: %s (UTC%+d)\n",
                TIMEZONES[g_timezoneIndex].label,
                (int)TIMEZONES[g_timezoneIndex].utcOffsetHours);
}



static void showWifiSetupRequired(unsigned long splashStartMs, bool enforceSplashDelay = true) {
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

static void logTickInfo(const char* tag, uint32_t intervalSec) {
  time_t nowUtc = time(nullptr);
  if (intervalSec < 1) intervalSec = 1;
  time_t tickUtc = nowUtc - (nowUtc % (time_t)intervalSec);
  time_t nextUtc = tickUtc + (time_t)intervalSec;
  Serial.printf("[Tick][%s] nowUtc=%ld tickUtc=%ld delta=%ld int=%lus next=%ld\n",
                tag, (long)nowUtc, (long)tickUtc, (long)(nowUtc - tickUtc),
                (unsigned long)intervalSec, (long)nextUtc);
}

// ==================== Periodic NTP resync (V0.97) =====================

static void ntpTimeSyncCb(struct timeval* tv) {
  int64_t utc = 0;
  if (tv) utc = (int64_t)tv->tv_sec;
  else    utc = (int64_t)time(nullptr);

  portENTER_CRITICAL(&g_ntpMux);
  g_ntpEventUtc = utc;
  g_ntpEventPending = true;
  portEXIT_CRITICAL(&g_ntpMux);
}

// Apply SNTP config (mode/callback/interval). Call once after initial NTP sync,
// and again after any configTime() restart.
static void applyPeriodicNtpConfig(bool logLine)
{
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_set_time_sync_notification_cb(ntpTimeSyncCb);
    sntp_set_sync_interval((uint32_t)NTP_RESYNC_INTERVAL_SEC * 1000UL);

    if (logLine) {
        Serial.printf("[Time] SNTP periodic enabled: interval=%lus mode=SMOOTH\n",
                      (unsigned long)NTP_RESYNC_INTERVAL_SEC);
    }
}

static void ntpResyncReset(const char* reason)
{
    time_t nowUtc = time(nullptr);
    if (nowUtc < TIME_VALID_MIN_UTC) {
        g_nextNtpResyncUtc = 0;
        return;
    }
    g_nextNtpResyncUtc = alignNextTickUtc(nowUtc, (uint32_t)NTP_RESYNC_INTERVAL_SEC);
    Serial.printf("[Time][SNTP] Next resync: %ld (in %ld s) reason=%s\n",
                  (long)g_nextNtpResyncUtc,
                  (long)(g_nextNtpResyncUtc - nowUtc),
                  reason ? reason : "?");
}

static void requestNtpResync(const char* reason)
{
    Serial.printf("[Time][SNTP] Resync requested (%s)\n", reason ? reason : "?");
 // Restart SNTP via configTime (Arduino core stops/starts SNTP inside)
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
 // Re-apply interval/callback/mode in case configTime() reset them
    applyPeriodicNtpConfig(false);
}


void setupTime() {
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);

  Serial.println("[Time] Syncing NTP...");
  time_t nowUtc = 0;
  int retry = 0;
  while (nowUtc < TIME_VALID_MIN_UTC && retry < 40) {
    nowUtc = time(nullptr);
    Serial.print(".");
    delay(500);
    retry++;
  }
  Serial.println();

  if (nowUtc >= TIME_VALID_MIN_UTC) {
    struct tm local;
    if (getLocalTimeLocal(&local)) {
      Serial.printf("[Time] NTP OK: %02d:%02d  %02d/%02d/%04d\n",
                    local.tm_hour, local.tm_min,
                    local.tm_mon + 1, local.tm_mday, local.tm_year + 1900);
    } else {
      Serial.println("[Time] local conversion failed.");
    }

 // Enable tick-aligned scheduler as soon as time is valid.
  applyPeriodicNtpConfig(true);
    ntpResyncReset("NTP");
  tickSchedulerReset("NTP");
    logTickInfo("NTP", updateIntervalSec());
  } else {
    Serial.println("[Time] NTP FAILED");
    tickSchedulerReset("NTP FAIL");
  }
}

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


 // V0.97: If display currency is NTD, fetch FX once after WiFi+NTP is ready.
  if (g_displayCurrency == (int)CURR_NTD && WiFi.status() == WL_CONNECTED) {
    float rate = 0.0f;
    if (fetchUsdToTwdRate(rate)) {
      g_usdToTwd = rate;
      g_fxValid  = true;
      Serial.printf("[FX] USD->TWD=%.4f\n", g_usdToTwd);
    } else {
      g_fxValid = false;
      Serial.println("[FX] USD->TWD fetch failed");
    }
    g_nextFxUpdateUtc = time(nullptr) + 300;  // 5 min
  }
  updateEtCycle();
  bootstrapHistoryFromKrakenOHLC();

  float price  = 0.0f;
  float change = 0.0f;

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

// ==================== 設定 load/save =====================

void applyTimezone() {
  if (g_timezoneIndex < 0 || g_timezoneIndex >= TIMEZONE_COUNT)
    g_timezoneIndex = DEFAULT_TIMEZONE_INDEX;

  g_localUtcOffsetSec = (int32_t)TIMEZONES[g_timezoneIndex].utcOffsetHours * 3600;
}


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
                REFRESH_MODE_LABELS[g_refreshMode], (g_displayCurrency==CURR_NTD?"NTD":"USD"));
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
                REFRESH_MODE_LABELS[g_refreshMode], (g_displayCurrency==CURR_NTD?"NTD":"USD"));
}

// LED logic moved to led_status.cpp (V0.97)

// ==================== Menu handler =====================


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

void handleMenuSelect();

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

 // ✅ 新增：Refresh mode（你的 ui.cpp 既然有顯示，就要能切）
 // 如果你 ui.h 的 enum 名稱不是 MENU_REFRESH_MODE，
 // 你專案裡現在用什麼名字，這裡就要跟著一樣。
    
    case MENU_REFRESH_MODE: {
      g_refreshMode = (g_refreshMode + 1) % 2;
      Serial.printf("[Menu] Refresh mode -> %s\n", REFRESH_MODE_LABELS[g_refreshMode]);
      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_CURRENCY: {
      g_displayCurrency = (g_displayCurrency + 1) % (int)CURR_COUNT;
      if (g_displayCurrency < 0 || g_displayCurrency >= (int)CURR_COUNT) g_displayCurrency = (int)CURR_USD;
      Serial.printf("[Menu] Currency -> %s\n", (g_displayCurrency == (int)CURR_NTD) ? "NTD" : "USD");

 // If switching to NTD, refresh FX soon.
      if (g_displayCurrency == (int)CURR_NTD) g_nextFxUpdateUtc = 0;

      saveSettings();
      drawMenuScreen(false);
      break;
    }

    case MENU_DAYAVG_LINE:

 {
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
    requestMaintenanceModeReboot();
    return;
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

// ==================== Encoder (button only) =====================

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
 // 先讓使用者看到版本；同時後續 WiFi/NTP 會花時間，螢幕會停留在這張圖
  drawSplashScreen(CRYPTOBAR_VERSION);
  uint32_t splashStartMs = millis();

  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN,  INPUT_PULLUP);
  pinMode(ENC_SW_PIN,  INPUT_PULLUP);
  g_lastEncSw = digitalRead(ENC_SW_PIN);

 // ✅ rotation 用中斷抓，不怕被畫面/網路卡住
 // V0.97: use PCNT hardware for encoder rotation (no GPIO interrupt)
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

 // -------------------- Periodic FX update (USD->TWD) --------------------
if (!doUpdate && g_timeValid && WiFi.status() == WL_CONNECTED && g_displayCurrency == (int)CURR_NTD) {
  time_t nowFxUtc = time(nullptr);
  if (g_nextFxUpdateUtc == 0 || nowFxUtc >= g_nextFxUpdateUtc) {
    float rate = 0.0f;
    if (fetchUsdToTwdRate(rate)) {
      g_usdToTwd = rate;
      g_fxValid  = true;
      Serial.printf("[FX] USD->TWD=%.4f\n", g_usdToTwd);
} else {
      g_fxValid = false;
      Serial.println("[FX] USD->TWD fetch failed");
    }
    g_nextFxUpdateUtc = nowFxUtc + 300; // 5 min
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

    float price  = 0.0f;
    float change = 0.0f;
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
      g_lastPriceUsd  = price;
      g_lastChange24h = change;
      if (nowUtc >= TIME_VALID_MIN_UTC) {
        dayAvgRollingAdd(nowUtc, price);
      }
      updateAvgLineReference(nowUtc);

      addChartSampleForNow(price);
      updateLedForPrice(g_lastChange24h, g_lastPriceOk);

      if (g_uiMode == UI_MODE_NORMAL) {
 // ✅ refresh mode 規則：
 // - Partial：照舊（累積 PARTIAL_REFRESH_LIMIT 後才 full）
 // - Full：主畫面每次都 full
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
      fadeLedTo(100, 0, 120);  // API fail → 紫
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

  delay(1); // 比 delay(10) 更滑
}
