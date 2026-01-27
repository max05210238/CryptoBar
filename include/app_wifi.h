// CryptoBar V0.99t (Non-blocking Network)
// app_wifi.h - WiFi connection and credential management
#pragma once

#include <Arduino.h>

// Set WiFi hostname (call before WiFi.begin())
// Generates "CryptoBar_XXXX" using last 4 hex digits of MAC
void setWifiHostname();

// Load WiFi credentials from NVS
void loadWifiCreds();

// Save WiFi credentials to NVS
void saveWifiCreds(const String& ssid, const String& pass);

// Clear WiFi credentials from NVS
void clearWifiCreds();

// Convert RSSI to signal bars (0-5)
int rssiToBars(int rssi);

// Connect to WiFi in STA mode with timeout
// Returns true if connected successfully
bool connectWiFiSta(const char* ssid, const char* pass, uint32_t timeoutMs = 30000);

// Connect to WiFi with multiple retry attempts
// Shows UI progress if showUi is true
bool connectWiFiStaWithRetries(const char* ssid, const char* pass,
                               int maxAttempts,
                               uint32_t perAttemptTimeoutMs,
                               bool showUi);
