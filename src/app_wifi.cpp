// CryptoBar V0.98-rc2 (Refactored: Step 2)
// app_wifi.cpp - WiFi connection and credential management
#include "app_wifi.h"
#include "app_state.h"
#include "settings_store.h"
#include "led_status.h"
#include "ui.h"
#include <WiFi.h>

// ==================== WiFi Credentials Management =====================

void loadWifiCreds() {
  settingsStoreLoadWifi(g_wifiSsid, g_wifiPass);

  g_wifiSsid.trim();
  g_wifiPass.trim();
  g_hasWifiCreds = (g_wifiSsid.length() > 0);
  Serial.printf("[WiFi] NVS creds: %s\n", g_hasWifiCreds ? "FOUND" : "MISSING");
}

void saveWifiCreds(const String& ssid, const String& pass) {
  settingsStoreSaveWifi(ssid, pass);
}

void clearWifiCreds() {
  settingsStoreClearWifi();

  g_wifiSsid = "";
  g_wifiPass = "";
  g_hasWifiCreds = false;
  Serial.println("[WiFi] Cleared saved credentials (factory reset)");
}

// ==================== WiFi Utilities =====================

int rssiToBars(int rssi) {
  if (rssi >= -55) return 5;
  if (rssi >= -62) return 4;
  if (rssi >= -69) return 3;
  if (rssi >= -76) return 2;
  if (rssi >= -83) return 1;
  return 0;
}

// ==================== WiFi Connection =====================

// Helper function for MAC-based jitter (preserved from original)
static inline uint32_t macLast16bits() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  if (mac.length() < 4) return 0;
  String last4 = mac.substring(mac.length() - 4);
  char buf[5];
  last4.toCharArray(buf, sizeof(buf));
  return (uint32_t)strtoul(buf, nullptr, 16);
}

static inline void initFetchJitterIfNeeded() {
  // Disable per-device fetch jitter to maximize multi-unit sync
  g_fetchJitterSec = 0;
}

bool connectWiFiSta(const char* ssid, const char* pass, uint32_t timeoutMs) {
  if (!ssid || !ssid[0]) return false;

  WiFi.mode(WIFI_STA);
  // Disable WiFi power-save to reduce connection instability
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

    // Mark that we have successfully connected at least once
    g_wifiEverConnected = true;
    g_nextRuntimeReconnectMs = 0;
    g_runtimeReconnectBatch = 0;

    return true;
  }

  Serial.println("[WiFi] Connect FAILED (timeout)");
  WiFi.disconnect(true);
  return false;
}

bool connectWiFiStaWithRetries(const char* ssid, const char* pass,
                               int maxAttempts,
                               uint32_t perAttemptTimeoutMs,
                               bool showUi)
{
  if (!ssid || !ssid[0]) return false;
  if (maxAttempts < 1) maxAttempts = 1;

  for (int i = 1; i <= maxAttempts; i++) {
    if (showUi) {
      // Append (i/N) to SSID label to show attempt count
      String label = String(ssid) + " (" + String(i) + "/" + String(maxAttempts) + ")";
      drawWifiConnectingScreen(CRYPTOBAR_VERSION, label.c_str());
    }

    Serial.printf("[WiFi] STA attempt %d/%d (timeout=%lums)\n",
                  i, maxAttempts, (unsigned long)perAttemptTimeoutMs);

    if (connectWiFiSta(ssid, pass, perAttemptTimeoutMs)) {
      return true;
    }

    // Small delay to avoid rapid reconnection attempts
    delay(300);
  }

  return false;
}
