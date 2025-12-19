// CryptoBar V0.98 (Refactored: Step 3)
// app_time.h - Time synchronization and timezone management
#pragma once

#include <Arduino.h>
#include <time.h>

// Setup NTP time synchronization
void setupTime();

// Apply timezone offset to g_localUtcOffsetSec
void applyTimezone();

// Get local time using g_localUtcOffsetSec
bool getLocalTimeLocal(struct tm* out);

// Auto-detect timezone from WorldTimeAPI (one-time, best-effort)
void autoDetectTimezoneIfNeeded();

// ==================== NTP Resync Functions =====================

// NTP time sync callback (called by SNTP library)
void ntpTimeSyncCb(struct timeval* tv);

// Apply periodic NTP configuration (mode/callback/interval)
void applyPeriodicNtpConfig(bool logLine);

// Reset NTP resync scheduler
void ntpResyncReset(const char* reason);

// Request immediate NTP resync
void requestNtpResync(const char* reason);
