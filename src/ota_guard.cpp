// CryptoBar V0.97 (OTA safety guard)
#include "ota_guard.h"

#include <Preferences.h>

#include <esp_ota_ops.h>
#include <esp_partition.h>

// NVS namespace + keys
static const char* kNs        = "ota_guard";
static const char* kPending   = "pending";   // bool
static const char* kPrevLabel = "prev";      // string
static const char* kNewLabel  = "next";      // string
static const char* kInProg    = "inprog";    // bool
static const char* kAttempts  = "attempts";  // u8
static const char* kLastMsg   = "lastmsg";   // string

// Policy
static const uint8_t  kMaxAttemptsBeforeRollback = 2;   // 2 failed boots -> rollback
static const uint32_t kVerifyUptimeMs            = 25000; // 25s stable -> mark valid

static bool     s_pending = false;
static uint32_t s_bootMs  = 0;
static String   s_prev;
static String   s_next;

static String runningLabel() {
  const esp_partition_t* p = esp_ota_get_running_partition();
  if (!p) return String("");
  return String(p->label);
}

static const esp_partition_t* findAppByLabel(const String& label) {
  if (label.length() == 0) return nullptr;
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label.c_str());
}

static void clearPending(Preferences& pref, const String& msg) {
  pref.putBool(kPending, false);
  pref.putBool(kInProg, false);
  pref.putUChar(kAttempts, 0);
  if (msg.length() > 0) pref.putString(kLastMsg, msg);
}

// Best-effort integration with ESP-IDF rollback (if enabled in bootloader).
static void markAppValidBestEffort(Preferences* pref, const String& tag) {
  esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
  if (err != ESP_OK) {
 // Not fatal; rollback may not be enabled in this build.
    if (pref) {
      pref->putString(kLastMsg, tag + String(" mark_valid err=") + String((int)err));
    }
  }
}

void otaGuardSetPending(const String& prevLabel, const String& newLabel) {
  Preferences pref;
  if (!pref.begin(kNs, false)) return;

  pref.putString(kPrevLabel, prevLabel);
  pref.putString(kNewLabel,  newLabel);
  pref.putUChar(kAttempts, 0);
  pref.putBool(kInProg, false);
  pref.putBool(kPending, true);
  pref.putString(kLastMsg, "pending");
  pref.end();
}

void otaGuardBootBegin() {
  s_bootMs = millis();
  s_pending = false;
  s_prev = "";
  s_next = "";

  Preferences pref;
  if (!pref.begin(kNs, false)) return;

  const bool pending = pref.getBool(kPending, false);
  if (!pending) {
    pref.end();
    return;
  }

  const String prevLabel = pref.getString(kPrevLabel, "");
  const String nextLabel = pref.getString(kNewLabel,  "");
  const bool   inprog    = pref.getBool(kInProg, false);
  uint8_t      attempts  = pref.getUChar(kAttempts, 0);

  const String run = runningLabel();
 // If the bootloader is using pending-verify state, we will mark valid later
 // via otaGuardLoop() once the app has stayed up long enough.

 // If we are not running the "next" slot anymore, consider the pending state resolved.
  if (run.length() == 0 || nextLabel.length() == 0 || run != nextLabel) {
    clearPending(pref, "cleared (not on next slot)");
    pref.end();
    return;
  }

 // We are running the newly flashed slot.
  s_pending = true;
  s_prev = prevLabel;
  s_next = nextLabel;

 // If the previous boot never cleared in-progress, count it as a failure.
  if (inprog) {
    if (attempts < 255) attempts++;
  } else {
 // First boot attempt after update
    if (attempts < 255) attempts++;
  }

  pref.putUChar(kAttempts, attempts);
  pref.putBool(kInProg, true);
  pref.putString(kLastMsg, String("boot attempt ") + String(attempts));

 // Roll back after N failed boots.
  if (inprog && attempts >= kMaxAttemptsBeforeRollback) {
    const esp_partition_t* prev = findAppByLabel(prevLabel);
    if (prev) {
      const esp_err_t err = esp_ota_set_boot_partition(prev);
      if (err == ESP_OK) {
        clearPending(pref, String("rollback to ") + prevLabel);
 // Also try to let bootloader know this app is not valid (best-effort).
 // If rollback is not enabled, this returns an error and is ignored.
        esp_ota_mark_app_invalid_rollback_and_reboot();
        pref.end();
        delay(120);
        ESP.restart();
        return;
      } else {
        pref.putString(kLastMsg, String("rollback failed err=") + String((int)err));
      }
    } else {
      pref.putString(kLastMsg, "rollback failed: prev slot not found");
    }
  }

  pref.end();
}

void otaGuardLoop() {
  if (!s_pending) return;
  if ((uint32_t)(millis() - s_bootMs) < kVerifyUptimeMs) return;

  Preferences pref;
  if (!pref.begin(kNs, false)) return;

 // Still pending AND running the intended slot? then mark valid.
  const bool pending = pref.getBool(kPending, false);
  if (!pending) {
    pref.end();
    s_pending = false;
    return;
  }

  const String run = runningLabel();
  const String nextLabel = pref.getString(kNewLabel, "");
  if (run.length() > 0 && nextLabel.length() > 0 && run == nextLabel) {
 // Mark valid for both our NVS guard and (if enabled) ESP-IDF rollback.
    markAppValidBestEffort(&pref, String("verified ") + run);
    clearPending(pref, String("verified ") + run);
  }

  pref.end();
  s_pending = false;
}

bool otaGuardIsPending() {
  return s_pending;
}

String otaGuardPendingSummary() {
  if (!s_pending) return String("(none)");
  return String("pending: ") + s_prev + " -> " + s_next;
}
