#pragma once

#include <Arduino.h>
#include <time.h>

// Day-average line modes (persisted in settings_store.dayAvg)
// 0 = Off
// 1 = Rolling 24h mean (default)
// 2 = ET 7pm cycle mean

enum DayAvgMode : uint8_t {
  DAYAVG_OFF     = 0,
  DAYAVG_ROLLING = 1,
  DAYAVG_CYCLE   = 2,
};

// Rolling 24h mean state (5-min buckets)
void dayAvgRollingReset();
void dayAvgRollingAdd(time_t sampleUtc, float price);
bool dayAvgRollingGet(time_t nowUtc, float& outMean);
int  dayAvgRollingCount();

// Cycle mean computed from current chart buffer (7pm ET cycle)
bool dayAvgCycleMean(float& outMean);
