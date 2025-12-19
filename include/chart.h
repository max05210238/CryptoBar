#pragma once

#include <Arduino.h>
#include <time.h>

// 單一圖表 sample：在當天 7pm ET cycle 中的相對位置 + 價格
struct ChartSample {
  float pos;    // 0.0–1.0, 一天內相對位置（7pm ET → 7pm ET）
  float price;
};

// 一天最多 sample 數（跟你原本一樣 300）
constexpr int MAX_CHART_SAMPLES = 300;

// 這些全域變數在 main.cpp 裡「定義」，在其他 .cpp 只用 extern 引用
extern ChartSample g_chartSamples[MAX_CHART_SAMPLES];
extern int         g_chartSampleCount;

// 7pm ET cycle 狀態（同樣在 main.cpp 定義）
extern bool   g_cycleInit;
extern time_t g_cycleStartUtc;
extern time_t g_cycleEndUtc;

// 對外提供的 API（實作在 network.cpp）
// - 重新計算今天的 7pm ET cycle
// - 用「現在時間」加進一個 sample
void updateEtCycle();
void addChartSampleForNow(float price);
