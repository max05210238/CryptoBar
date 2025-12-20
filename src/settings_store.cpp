// settings_store.cpp
// CryptoBar V0.97: move NVS (Preferences) access out of main.cpp
// - coin is stored as ticker string under key "coin"
// - migrates from legacy V0.97 key "coinIndex" (0=XRP,1=BTC,2=ETH)

#include <Arduino.h>
#include <Preferences.h>

#include "settings_store.h"
#include "config.h" // DEFAULT_TIMEZONE_INDEX, TIMEZONE_COUNT

static const char* kNvsNamespace = "cryptobar";

static String legacyTickerFromIndex(int idx) {
  switch (idx) {
    case 0: return "XRP";
    case 1: return "BTC";
    case 2: return "ETH";
    default: return "";
  }
}

bool settingsStoreLoad(StoredSettings& out) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true)) {
    return false;
  }

  out.updPreset = prefs.getInt("updPreset", out.updPreset);
  out.briPreset = prefs.getInt("briPreset", out.briPreset);

 // Coin selection:
 // - If key "coin" exists: use it (ticker string)
 // - Else if legacy key "coinIndex" exists: migrate
 // - Else: default to BTC
  String coin;
  if (prefs.isKey("coin")) {
    coin = prefs.getString("coin", "");
  } else if (prefs.isKey("coinIndex")) {
    int legacyIdx = prefs.getInt("coinIndex", 0);
    coin = legacyTickerFromIndex(legacyIdx);
  } else {
    coin = "BTC";
  }
  if (coin.length() == 0) coin = "BTC";
  out.coinTicker = coin;

  out.timeFmt   = prefs.getInt("timeFmt",   out.timeFmt);
  out.dateFmt   = prefs.getInt("dateFmt",   out.dateFmt);
  out.dtSize    = prefs.getInt("dtSize",    out.dtSize);
  if (out.dtSize < 0) out.dtSize = 1;
  if (out.dtSize > 1) out.dtSize = 1;
 // If tzIndex key is missing (fresh device / after clear), default to Seattle.
  out.tzIndex   = prefs.getInt("tzIndex",   (int)DEFAULT_TIMEZONE_INDEX);
 // Safety: clamp to valid range in case NVS contains an out-of-range value.
  if (out.tzIndex < 0) out.tzIndex = (int)DEFAULT_TIMEZONE_INDEX;
  if (out.tzIndex >= (int)TIMEZONE_COUNT) out.tzIndex = (int)DEFAULT_TIMEZONE_INDEX;
  out.dayAvg    = prefs.getInt("dayAvg",    out.dayAvg);
  out.rfMode    = prefs.getInt("rfMode",    out.rfMode);

  // V0.99f: Multi-currency support - validate range
  out.dispCur   = prefs.getInt("dispCur",   (int)CURR_USD);
  if (out.dispCur < 0 || out.dispCur >= (int)CURR_COUNT) {
    out.dispCur = (int)CURR_USD;
  }

  prefs.end();
  return true;
}

bool settingsStoreSave(const StoredSettings& in) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {
    return false;
  }

  bool ok = true;
  ok &= prefs.putInt("updPreset", in.updPreset) > 0;
  ok &= prefs.putInt("briPreset", in.briPreset) > 0;

 // Store new key
  ok &= prefs.putString("coin", in.coinTicker) > 0;

  ok &= prefs.putInt("timeFmt",   in.timeFmt) > 0;
  ok &= prefs.putInt("dateFmt",   in.dateFmt) > 0;
  ok &= prefs.putInt("dtSize",    in.dtSize) > 0;
  ok &= prefs.putInt("tzIndex",   in.tzIndex) > 0;
  ok &= prefs.putInt("dayAvg",    in.dayAvg) > 0;
  ok &= prefs.putInt("rfMode",    in.rfMode) > 0;

  ok &= prefs.putInt("dispCur",  in.dispCur) > 0;
 // Best-effort cleanup legacy key (not fatal if it fails).
  if (prefs.isKey("coinIndex")) {
    prefs.remove("coinIndex");
  }

  prefs.end();
  return ok;
}

bool settingsStoreLoadWifi(String& outSsid, String& outPass) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true)) {
    return false;
  }

  outSsid = prefs.getString("w_ssid", "");
  outPass = prefs.getString("w_pass", "");

  prefs.end();
  return true;
}

bool settingsStoreSaveWifi(const String& ssid, const String& pass) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {
    return false;
  }
  bool ok = true;
  ok &= prefs.putString("w_ssid", ssid) > 0;
  ok &= prefs.putString("w_pass", pass) > 0;
  prefs.end();
  return ok;
}

bool settingsStoreClearWifi() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {
    return false;
  }
  bool ok = true;
  if (prefs.isKey("w_ssid")) ok &= prefs.remove("w_ssid");
  if (prefs.isKey("w_pass")) ok &= prefs.remove("w_pass");
  prefs.end();
  return ok;
}

bool settingsStoreHasTzIndex() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true)) {
    return false;
  }
  bool has = prefs.isKey("tzIndex");
  prefs.end();
  return has;
}
