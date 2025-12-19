// settings_store.h
// CryptoBar V0.97: NVS (Preferences) access (coin stored as ticker string)
#pragma once

#include <Arduino.h>
#include "config.h" // DEFAULT_TIMEZONE_INDEX

struct StoredSettings {
  int updPreset = 0;
  int briPreset = 1;

 // V0.97: store coin as ticker string (e.g., "XRP") for forward-compatibility.
 // Backward-compatibility: V0.97 used an int coinIndex; migration handled in settings_store.cpp.
  String coinTicker = "";

  int timeFmt   = 1;  // V0.97: default to 12h
  int dateFmt   = 0;
 // Date/Time size:
 // 0 = Small, 1 = Large
 // V0.97: default to Large.
  int dtSize    = 1;
 // Default timezone: Seattle (UTC-08)
  int tzIndex   = (int)DEFAULT_TIMEZONE_INDEX;
  int dayAvg    = 1;
 // Refresh mode default:
 // 0 = Partial rules, 1 = Full rules
 // V0.97 freeze: default to Full refresh.
  int rfMode    = 1;
 // Display currency: CURR_USD or CURR_NTD
  int dispCur   = (int)CURR_USD;
};

// Returns true if read succeeded (even if keys missing; defaults will apply).
bool settingsStoreLoad(StoredSettings& out);

// Returns true if write succeeded.
bool settingsStoreSave(const StoredSettings& in);

// Returns true if tzIndex key exists in NVS.
// Used to decide whether we should auto-detect timezone on first setup.
bool settingsStoreHasTzIndex();

// WiFi credentials helpers (keys: w_ssid, w_pass). Returns true if read succeeded.
bool settingsStoreLoadWifi(String& outSsid, String& outPass);
bool settingsStoreSaveWifi(const String& ssid, const String& pass);
bool settingsStoreClearWifi();
