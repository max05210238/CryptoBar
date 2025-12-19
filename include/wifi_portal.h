#pragma once
#include <Arduino.h>

// Minimal AP provisioning portal.
// - starts AP + web page on 192.168.4.1
// - collects WiFi credentials (and optionally a few settings)

struct WifiPortalSubmission {
 // WiFi
  String  ssid;
  String  pass;

 // Core settings
  String  coinTicker;
  int     updPreset   = -1; // optional (if provided by portal)
  int     rfMode      = -1; // 0=Partial, 1=Full
  int     briPreset   = -1; // optional
  int     timeFmt     = -1; // 0=24h, 1=12h
  int     dateFmt     = -1; // 0=MM/DD, 1=DD/MM, 2=YYYY-MM-DD
  int     dtSize     = -1; // 0=Small, 1=Large
  int     dispCur     = -1; // 0=USD, 1=NTD
  int     tzIndex     = -1; // optional
  int     dayAvg      = -1; // 0=Off, 1=Rolling 24h mean
};

// Start portal with default SSID "CryptoBar_XXXX" (MAC last 4).
void wifiPortalStart();
// Start portal with a custom SSID (optional). If nullptr/empty, uses default.
void wifiPortalStart(const char* apSsid);

void wifiPortalStop();
void wifiPortalLoop();

bool wifiPortalIsRunning();
const String& wifiPortalApSsid();

bool wifiPortalHasSubmission();

// Preferred: get all fields.
bool wifiPortalTakeSubmission(WifiPortalSubmission& out);

// Legacy: WiFi + coin only (kept for compatibility).
bool wifiPortalTakeSubmission(String& outSsid, String& outPass, String& outCoinTicker);

// Default selection for coin dropdown.
void wifiPortalSetDefaultCoinTicker(const String& ticker);
void wifiPortalSetDefaultCoinTicker(const char* ticker);

// Pre-select portal dropdown values (pass -1 to default to "Keep current").
void wifiPortalSetDefaultAdvanced(int rfMode, int updPreset, int briPreset,
                                int timeFmt, int dateFmt, int dtSize,
                                int dispCur, int tzIndex, int dayAvg);
