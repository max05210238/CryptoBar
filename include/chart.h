#pragma once

#include <Arduino.h>
#include <time.h>

// Single chart sample: relative position within today's 7pm ET cycle + price
struct ChartSample {
  float  pos;    // 0.0–1.0, relative position within day (7pm ET → 7pm ET)
  double price;  // Price in USD (double for precision)
};

// Maximum samples per day (same as original: 300)
constexpr int MAX_CHART_SAMPLES = 300;

// These global variables are defined in main.cpp; other .cpp files use extern references
extern ChartSample g_chartSamples[MAX_CHART_SAMPLES];
extern int         g_chartSampleCount;

// 7pm ET cycle state (also defined in main.cpp)
extern bool   g_cycleInit;
extern time_t g_cycleStartUtc;
extern time_t g_cycleEndUtc;

// Public API (implemented in network.cpp)
// - Recalculate today's 7pm ET cycle
// - Add a sample using current time
void updateEtCycle();
void addChartSampleForNow(double price);
