// CryptoBar V0.98-rc3 (Refactored: Step 3)
// app_scheduler.cpp - Tick-aligned update scheduler
#include "app_scheduler.h"
#include "app_state.h"

uint32_t updateIntervalSec() {
  uint32_t sec = (uint32_t)(g_updateIntervalMs / 1000UL);
  if (sec < 30) sec = 30;
  return sec;
}

bool isTimeValidNow() {
  time_t t = time(nullptr);
  return (t >= TIME_VALID_MIN_UTC);
}

time_t alignNextTickUtc(time_t nowUtc, uint32_t intervalSec) {
  if (intervalSec < 1) intervalSec = 1;
  // Next multiple of intervalSec (epoch-aligned)
  return ((nowUtc / (time_t)intervalSec) + 1) * (time_t)intervalSec;
}

void tickSchedulerReset(const char* reason) {
  time_t nowUtc = time(nullptr);
  if (nowUtc < TIME_VALID_MIN_UTC) {
    g_timeValid = false;
    g_nextUpdateUtc = 0;
    g_prefetchValid = false;
    g_prefetchForUtc = 0;
    g_nextFxUpdateUtc = 0;
    Serial.printf("[Sched] Reset(%s): time not valid\n", reason ? reason : "");
    return;
  }

  g_timeValid = true;
  uint32_t sec = updateIntervalSec();
  g_nextUpdateUtc = alignNextTickUtc(nowUtc, sec);
  g_nextFxUpdateUtc = alignNextTickUtc(nowUtc, 300);

  // New schedule → discard any pending prefetch
  g_prefetchValid = false;
  g_prefetchForUtc = 0;
  Serial.printf("[Sched] Reset(%s): int=%lus nextUpdateUtc=%ld (in %ld s)\n",
                reason ? reason : "", (unsigned long)sec,
                (long)g_nextUpdateUtc, (long)(g_nextUpdateUtc - nowUtc));
}

void logTickInfo(const char* tag, uint32_t intervalSec) {
  time_t nowUtc = time(nullptr);
  if (intervalSec < 1) intervalSec = 1;
  time_t tickUtc = nowUtc - (nowUtc % (time_t)intervalSec);
  time_t nextUtc = tickUtc + (time_t)intervalSec;
  Serial.printf("[Tick][%s] nowUtc=%ld tickUtc=%ld delta=%ld int=%lus next=%ld\n",
                tag, (long)nowUtc, (long)tickUtc, (long)(nowUtc - tickUtc),
                (unsigned long)intervalSec, (long)nextUtc);
}
