#include "day_avg.h"
#include "chart.h"

static constexpr int   kMaxSamples   = 320;   // > 288 (24h @ 5-min)
static constexpr int   kBucketSec    = 300;   // 5 minutes
static constexpr time_t kWindowSec   = 24 * 3600;

struct MeanSample {
  time_t tUtc;   // bucket start (UTC seconds)
  double price;
};

static MeanSample s_buf[kMaxSamples];
static int    s_head  = 0;   // index of oldest
static int    s_count = 0;   // number of valid samples
static double s_sum   = 0.0;

static inline int idxAt(int offset) {
  return (s_head + offset) % kMaxSamples;
}

void dayAvgRollingReset() {
  s_head  = 0;
  s_count = 0;
  s_sum   = 0.0;
}

static void popOldest() {
  if (s_count <= 0) return;
  s_sum -= s_buf[s_head].price;
  s_head = (s_head + 1) % kMaxSamples;
  s_count--;
}

static void pushNewest(time_t bucketUtc, double price) {
 // If full, drop oldest.
  if (s_count >= kMaxSamples) {
    popOldest();
  }
  int i = idxAt(s_count);
  s_buf[i].tUtc = bucketUtc;
  s_buf[i].price = price;
  s_sum += price;
  s_count++;
}

void dayAvgRollingAdd(time_t sampleUtc, double price) {
  if (sampleUtc <= 0) return;

 // Normalize to 5-min bucket.
  time_t bucketUtc = sampleUtc - (sampleUtc % kBucketSec);

  if (s_count <= 0) {
    pushNewest(bucketUtc, price);
    return;
  }

  int lastIdx = idxAt(s_count - 1);
  time_t lastBucket = s_buf[lastIdx].tUtc;

  if (bucketUtc == lastBucket) {
 // Replace latest bucket price.
    s_sum -= s_buf[lastIdx].price;
    s_buf[lastIdx].price = price;
    s_sum += price;
    return;
  }

  if (bucketUtc < lastBucket) {
 // Time went backwards (NTP jump). Reset to avoid skew.
    dayAvgRollingReset();
    pushNewest(bucketUtc, price);
    return;
  }

  pushNewest(bucketUtc, price);
}

static void trimTo24h(time_t nowUtc) {
  if (nowUtc <= 0 || s_count <= 0) return;
  time_t cutoff = nowUtc - kWindowSec;
  while (s_count > 0) {
    time_t t0 = s_buf[s_head].tUtc;
    if (t0 >= cutoff) break;
    popOldest();
  }
}

bool dayAvgRollingGet(time_t nowUtc, double& outMean) {
  trimTo24h(nowUtc);
  if (s_count <= 0) return false;
  outMean = s_sum / (double)s_count;
  return true;
}

int dayAvgRollingCount() {
  return s_count;
}

bool dayAvgCycleMean(double& outMean) {
  if (g_chartSampleCount <= 0) return false;
  double acc = 0.0;
  for (int i = 0; i < g_chartSampleCount; i++) {
    acc += g_chartSamples[i].price;
  }
  outMean = acc / (double)g_chartSampleCount;
  return true;
}
