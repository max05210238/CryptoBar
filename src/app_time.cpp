// CryptoBar V0.98 (Refactored: Step 3)
// app_time.cpp - Time synchronization and timezone management
#include "app_time.h"
#include "app_state.h"
#include "app_scheduler.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_sntp.h>

// Forward declaration
void saveSettings();

// ==================== Local Time =====================

bool getLocalTimeLocal(struct tm* out) {
  time_t nowUtc = time(nullptr);
  if (nowUtc < 100000) return false;

  time_t localSec = nowUtc + g_localUtcOffsetSec;
  struct tm* t = gmtime(&localSec);
  if (!t) return false;

  *out = *t;
  return true;
}

// ==================== Timezone =====================

void applyTimezone() {
  if (g_timezoneIndex < 0 || g_timezoneIndex >= TIMEZONE_COUNT)
    g_timezoneIndex = DEFAULT_TIMEZONE_INDEX;

  g_localUtcOffsetSec = (int32_t)TIMEZONES[g_timezoneIndex].utcOffsetHours * 3600;
}

// ==================== Auto-detect timezone =====================

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

    // Parse only the utc_offset field to keep memory low
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

    // Expected format: "+08:00" or "-05:00"
    char sign = off[0];
    int hh = atoi(off + 1);
    int mm = 0;
    const char* colon = strchr(off, ':');
    if (colon && colon[1] && colon[2]) {
      mm = atoi(colon + 1);
    }

    // V0.97 integer-only timezone support
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

void autoDetectTimezoneIfNeeded() {
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

// ==================== NTP Time Synchronization =====================

void ntpTimeSyncCb(struct timeval* tv) {
  int64_t utc = 0;
  if (tv) utc = (int64_t)tv->tv_sec;
  else    utc = (int64_t)time(nullptr);

  portENTER_CRITICAL(&g_ntpMux);
  g_ntpEventUtc = utc;
  g_ntpEventPending = true;
  portEXIT_CRITICAL(&g_ntpMux);
}

void applyPeriodicNtpConfig(bool logLine) {
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
  sntp_set_time_sync_notification_cb(ntpTimeSyncCb);
  sntp_set_sync_interval((uint32_t)NTP_RESYNC_INTERVAL_SEC * 1000UL);

  if (logLine) {
    Serial.printf("[Time] SNTP periodic enabled: interval=%lus mode=SMOOTH\n",
                  (unsigned long)NTP_RESYNC_INTERVAL_SEC);
  }
}

void ntpResyncReset(const char* reason) {
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

void requestNtpResync(const char* reason) {
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

    // Enable tick-aligned scheduler as soon as time is valid
    applyPeriodicNtpConfig(true);
    ntpResyncReset("NTP");
    tickSchedulerReset("NTP");
    logTickInfo("NTP", updateIntervalSec());
  } else {
    Serial.println("[Time] NTP FAILED");
    tickSchedulerReset("NTP FAIL");
  }
}
