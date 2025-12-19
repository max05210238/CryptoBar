// CryptoBar V0.98 (Refactored: Step 3)
// app_scheduler.h - Tick-aligned update scheduler
#pragma once

#include <Arduino.h>
#include <time.h>

// Get update interval in seconds (from g_updateIntervalMs)
uint32_t updateIntervalSec();

// Check if system time is valid (after NTP sync)
bool isTimeValidNow();

// Align time to next tick boundary (epoch-aligned)
time_t alignNextTickUtc(time_t nowUtc, uint32_t intervalSec);

// Reset tick scheduler (recalculate next update time)
void tickSchedulerReset(const char* reason);

// Log tick timing information (for debugging multi-unit sync)
void logTickInfo(const char* tag, uint32_t intervalSec);
