#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string.h>

#include "coins.h"
#include "chart.h"
#include "day_avg.h"
#include "network.h"

// Some values were originally from config.h; these are fallback defaults
#ifndef MARKET_GMT_OFFSET_SEC
// Default: US Eastern Time UTC-5 (DST not handled)
#define MARKET_GMT_OFFSET_SEC (-5 * 3600)
#endif

#ifndef MARKET_ANCHOR_HOUR_ET
// 7pm ET
#define MARKET_ANCHOR_HOUR_ET 19
#endif

// Helper provided by main.cpp (declaration only; definition in main.cpp)
const CoinInfo& currentCoin();

// ==================== 7pm ET cycle & chart samples =====================

void updateEtCycle() {
  time_t nowUtc = time(nullptr);
  if (nowUtc <= 0) {
    Serial.println("[Cycle] time(nullptr) failed.");
    return;
  }

 // Convert to exchange time (default: ET)
  time_t nowEt = nowUtc + MARKET_GMT_OFFSET_SEC;

  time_t dayStartEt = nowEt - (nowEt % 86400);                   // Today 00:00 ET
  time_t anchorEt   = dayStartEt + MARKET_ANCHOR_HOUR_ET * 3600; // 7pm ET

  if (nowEt < anchorEt) {
 // Before today's 7pm → use yesterday's 7pm as anchor
    anchorEt -= 86400;
  }

  time_t startUtc = anchorEt - MARKET_GMT_OFFSET_SEC;
  time_t endUtc   = startUtc + 24 * 3600;

  if (!g_cycleInit || startUtc != g_cycleStartUtc) {
    g_cycleInit        = true;
    g_cycleStartUtc    = startUtc;
    g_cycleEndUtc      = endUtc;
    g_chartSampleCount = 0;
  dayAvgRollingReset();
    Serial.printf("[Cycle] New ET day: startUtc=%ld, endUtc=%ld\n",
                  (long)g_cycleStartUtc, (long)g_cycleEndUtc);
  }
}

// Internal: add a sample to chart buffer at specified UTC time
static void addChartSampleUtc(time_t sampleUtc, float price) {
  if (!g_cycleInit) {
    updateEtCycle();
    if (!g_cycleInit) return;
  }

  if (sampleUtc < g_cycleStartUtc) return;
  if (sampleUtc > g_cycleEndUtc)   sampleUtc = g_cycleEndUtc;

  float pos = float(sampleUtc - g_cycleStartUtc) /
              float(g_cycleEndUtc - g_cycleStartUtc);
  if (pos < 0.0f) pos = 0.0f;
  if (pos > 1.0f) pos = 1.0f;

  if (g_chartSampleCount < MAX_CHART_SAMPLES) {
    g_chartSamples[g_chartSampleCount].pos   = pos;
    g_chartSamples[g_chartSampleCount].price = price;
    g_chartSampleCount++;
  } else {
 // Rolling buffer: discard oldest
    for (int i = 1; i < MAX_CHART_SAMPLES; ++i) {
      g_chartSamples[i - 1] = g_chartSamples[i];
    }
    g_chartSamples[MAX_CHART_SAMPLES - 1].pos   = pos;
    g_chartSamples[MAX_CHART_SAMPLES - 1].price = price;
  }
}

// Public API: add a sample using current time
// Fixed: instead of adding a point every 30s, use 5-minute buckets (matching Kraken OHLC interval=5)
// - Within the same 5-minute bucket, only update the last price
// - Only append a new sample when crossing bucket boundaries
void addChartSampleForNow(float price) {
  time_t nowUtc = time(nullptr);
  if (nowUtc <= 0) {
    Serial.println("[Chart] time(nullptr) failed in addChartSampleForNow().");
    return;
  }

  updateEtCycle();
  if (!g_cycleInit) return;

  const int BUCKET_SEC = 5 * 60; // 5 minutes
  int bucket = (int)((nowUtc - g_cycleStartUtc) / BUCKET_SEC);
  if (bucket < 0) return;

 // Reset bucket memory when cycle rolls over to new day
  static time_t s_lastCycleStart = 0;
  static int    s_lastBucket     = -1;
  if (s_lastCycleStart != g_cycleStartUtc) {
    s_lastCycleStart = g_cycleStartUtc;
    s_lastBucket = -1;
  }

 // Same bucket: update last point, don't add new one (prevents filling 300 samples too quickly)
  if (bucket == s_lastBucket && g_chartSampleCount > 0) {
    g_chartSamples[g_chartSampleCount - 1].price = price;
    return;
  }

 // New bucket: add new point (using bucket start time)
  s_lastBucket = bucket;
  time_t bucketUtc = g_cycleStartUtc + (time_t)bucket * BUCKET_SEC;
  addChartSampleUtc(bucketUtc, price);
}

// ==================== Price fetching =====================

static bool fetchPriceFromPaprika(float& priceUsd, float& change24h) {
  const CoinInfo& coin = currentCoin();

  if (!coin.paprikaId || coin.paprikaId[0] == '\0') {
    Serial.println("[CP] No paprikaId configured for this coin.");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CP] WiFi not connected.");
    return false;
  }

  HTTPClient http;
  // V0.99b: Avoid String concatenation (heap fragmentation)
  char url[128];
  snprintf(url, sizeof(url), "https://api.coinpaprika.com/v1/tickers/%s", coin.paprikaId);

  Serial.print("[CP] GET ");
  Serial.println(url);

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[CP] HTTP error: %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

 // Parse only the fields we need to keep memory usage low.
  StaticJsonDocument<128> filter;
  filter["quotes"]["USD"]["price"] = true;
  filter["quotes"]["USD"]["percent_change_24h"] = true;

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (err) {
    Serial.printf("[CP] JSON parse error: %s\n", err.c_str());
    return false;
  }

  JsonObject usd = doc["quotes"]["USD"].as<JsonObject>();
  if (usd.isNull()) {
    Serial.println("[CP] quotes.USD missing.");
    return false;
  }

  priceUsd  = usd["price"].as<float>();
  change24h = usd["percent_change_24h"].as<float>();

  Serial.printf("[CP] %s: $%.6f (24h: %.2f%%)\n",
                coin.ticker, priceUsd, change24h);

  return (priceUsd > 0.0f);
}

static bool fetchPriceFromKraken(float& priceUsd, float& change24h) {
  const CoinInfo& coin = currentCoin();

  if (!coin.krakenPair || coin.krakenPair[0] == '\0') {
    Serial.println("[Kraken] No krakenPair configured for this coin.");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Kraken] WiFi not connected.");
    return false;
  }

  HTTPClient http;
  // V0.99b: Avoid String concatenation (heap fragmentation)
  char url[128];
  snprintf(url, sizeof(url), "https://api.kraken.com/0/public/Ticker?pair=%s", coin.krakenPair);

  Serial.print("[Kraken] GET ");
  Serial.println(url);

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[Kraken] HTTP error: %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[Kraken] JSON parse error: %s\n", err.c_str());
    return false;
  }

  if (doc["error"].size() > 0) {
    Serial.print("[Kraken] API error: ");
    serializeJson(doc["error"], Serial);
    Serial.println();
    return false;
  }

  JsonObject result = doc["result"];
  if (result.isNull()) {
    Serial.println("[Kraken] result is null.");
    return false;
  }

  JsonObject ticker;
  for (JsonPair kv : result) {
    ticker = kv.value().as<JsonObject>();
    break;   // Only grab the first key
  }
  if (ticker.isNull()) {
    Serial.println("[Kraken] ticker missing.");
    return false;
  }

  const char* lastStr = ticker["c"][0] | "0";
  const char* openStr = ticker["o"]    | "0";

  priceUsd = atof(lastStr);
  float open = atof(openStr);
  if (open <= 0.0f) {
    change24h = 0.0f;
  } else {
    change24h = (priceUsd - open) / open * 100.0f;
  }

  Serial.printf("[Kraken] %s: $%.6f (24h: %.2f%%)\n",
                coin.ticker, priceUsd, change24h);

  return true;
}

static bool fetchPriceFromCoingecko(float& priceUsd, float& change24h) {
  const CoinInfo& coin = currentCoin();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CG] WiFi not connected.");
    return false;
  }

 // Use registry-provided CoinGecko id when available.
  if (!coin.geckoId || coin.geckoId[0] == '\0') {
    Serial.println("[CG] No geckoId configured for this coin.");
    return false;
  }

  HTTPClient http;
  // V0.99b: Avoid String concatenation (heap fragmentation)
  char url[192];
  snprintf(url, sizeof(url),
           "https://api.coingecko.com/api/v3/simple/price?ids=%s&vs_currencies=usd&include_24hr_change=true",
           coin.geckoId);

  Serial.print("[CG] GET ");
  Serial.println(url);

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[CG] HTTP error: %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[CG] JSON parse error: %s\n", err.c_str());
    return false;
  }

  // V0.99b: Use coin.geckoId directly instead of String temporary
  JsonObject coinObj = doc[coin.geckoId];
  if (coinObj.isNull()) {
    Serial.println("[CG] Coin id missing.");
    return false;
  }

  priceUsd  = coinObj["usd"].as<float>();
  change24h = coinObj["usd_24h_change"].as<float>();

  Serial.printf("[CG] %s: $%.6f (24h: %.2f%%)\n",
                coin.ticker, priceUsd, change24h);

  return true;
}

bool fetchPrice(float& priceUsd, float& change24h) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Price] WiFi not connected.");
    return false;
  }

  if (fetchPriceFromPaprika(priceUsd, change24h)) {
    return true;
  }
  Serial.println("[Price] CoinPaprika failed, falling back to CoinGecko...");

  if (fetchPriceFromCoingecko(priceUsd, change24h)) {
    return true;
  }

  Serial.println("[Price] CoinGecko failed, falling back to Kraken...");

  if (fetchPriceFromKraken(priceUsd, change24h)) {
    return true;
  }

  Serial.println("[Price] All providers failed.");
  return false;
}

// ==================== Historical OHLC bootstrap =====================

static bool bootstrapHistoryFromCoingeckoMarketChart() {
  const CoinInfo& coin = currentCoin();

  if (!coin.geckoId || coin.geckoId[0] == '\0') {
    Serial.println("[History][CG] No geckoId configured for this coin.");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[History][CG] WiFi not connected.");
    return false;
  }

 // Ensure 7pm ET cycle exists.
  updateEtCycle();
  if (!g_cycleInit) {
    Serial.println("[History][CG] cycle not initialized.");
    return false;
  }

  time_t nowUtc = time(nullptr);
  if (nowUtc <= 0) {
    Serial.println("[History][CG] time(nullptr) failed.");
    return false;
  }

  time_t windowStartUtc = g_cycleStartUtc;
  time_t windowEndUtc   = g_cycleEndUtc;
  if (nowUtc < windowEndUtc) windowEndUtc = nowUtc;

  HTTPClient http;
  // V0.99b: Avoid String concatenation (heap fragmentation)
  char url[192];
  snprintf(url, sizeof(url),
           "https://api.coingecko.com/api/v3/coins/%s/market_chart?vs_currency=usd&days=1",
           coin.geckoId);

  Serial.print("[History][CG] GET ");
  Serial.println(url);

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[History][CG] HTTP error: %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

 // Only keep the 'prices' array to reduce memory usage.
  StaticJsonDocument<64> filter;
  filter["prices"] = true;

 // V0.99b: Reduced from 24576 to 16384 (33% reduction) to save heap
 // Filter extracts only "prices" array, reducing memory footprint
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (err) {
    if (err == DeserializationError::NoMemory) {
      Serial.printf("[History][CG] Insufficient memory for parsing (need >16KB)\n");
      Serial.printf("[History][CG] Free heap: %u bytes\n", ESP.getFreeHeap());
    } else {
      Serial.printf("[History][CG] JSON parse error: %s\n", err.c_str());
    }
    return false;
  }

  JsonArray prices = doc["prices"].as<JsonArray>();
  if (prices.isNull()) {
    Serial.println("[History][CG] prices missing.");
    return false;
  }

  g_chartSampleCount = 0;
  dayAvgRollingReset();
  int kept = 0;

  for (JsonVariant v : prices) {
    JsonArray row = v.as<JsonArray>();
    if (row.isNull() || row.size() < 2) continue;

 // CoinGecko timestamps are milliseconds.
    long long tMs = row[0].as<long long>();
    time_t tUtc = (time_t)(tMs / 1000LL);
    float price = row[1].as<float>();
    if (price <= 0.0f) continue;

 // Rolling 24h mean uses full last-day window (seeded from CoinGecko)
    dayAvgRollingAdd(tUtc, price);
    if (tUtc < windowStartUtc || tUtc > windowEndUtc) continue;
    addChartSampleUtc(tUtc, price);
    kept++;
  }

  Serial.printf("[History][CG] Kept %d samples into chart.\n", kept);
  return (kept > 0);
}

void bootstrapHistoryFromKrakenOHLC() {
  const CoinInfo& coin = currentCoin();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[History] WiFi not connected, skip.");
    return;
  }

 // Ensure 7pm ET cycle is established first
  updateEtCycle();
  if (!g_cycleInit) {
    Serial.println("[History] cycle not initialized, abort.");
    return;
  }

  time_t nowUtc = time(nullptr);
  if (nowUtc <= 0) {
    Serial.println("[History] time(nullptr) failed.");
    return;
  }

 // UTC window for this 7pm ET cycle
  time_t windowStartUtc = g_cycleStartUtc;
  time_t windowEndUtc   = g_cycleEndUtc;
  if (nowUtc < windowEndUtc) {
    windowEndUtc = nowUtc;
  }

  Serial.printf("[History] cycle UTC window: %ld .. %ld\n",
                (long)windowStartUtc, (long)windowEndUtc);

  if (!coin.krakenPair || coin.krakenPair[0] == '\0') {
    Serial.println("[History] No Kraken pair configured, trying CoinGecko...");
    if (!bootstrapHistoryFromCoingeckoMarketChart()) {
      Serial.println("[History] CoinGecko history failed.");
    }
    return;
  }

 // Seed rolling 24h mean buffer (independent of chart window)
  dayAvgRollingReset();
  time_t sinceUtc = windowStartUtc;
  if (nowUtc > 100000) {
    time_t rollSince = nowUtc - (24 * 3600);
    if (rollSince < sinceUtc) sinceUtc = rollSince;
  }

  HTTPClient http;
  // V0.99b: Avoid String concatenation (heap fragmentation)
  // Only fetch data after this cycle's start timestamp to avoid oversized JSON
  char url[192];
  snprintf(url, sizeof(url),
           "https://api.kraken.com/0/public/OHLC?pair=%s&interval=5&since=%ld",
           coin.krakenPair, (long)sinceUtc);

  Serial.print("[History] GET ");
  Serial.println(url);

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[History] HTTP error: %d\n", code);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("[History] Payload length: %d bytes\n", payload.length());

 // V0.99b: Reduced from 49152 to 32768 (33% reduction) to save heap
 // With since= parameter, payload is filtered to current cycle only
  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    if (err == DeserializationError::NoMemory) {
      Serial.printf("[History] Insufficient memory for OHLC parsing (need >32KB)\n");
      Serial.printf("[History] Free heap: %u bytes\n", ESP.getFreeHeap());
    } else {
      Serial.printf("[History] JSON parse error: %s\n", err.c_str());
    }
    return;
  }

  if (doc["error"].size() > 0) {
    Serial.print("[History] API error: ");
    serializeJson(doc["error"], Serial);
    Serial.println();
    return;
  }

  JsonObject result = doc["result"];
  if (result.isNull()) {
    Serial.println("[History] result is null.");
    return;
  }

 // Kraken returns dynamic keys (pair name) + a 'last' field.
  JsonArray ohlcArr;
  for (JsonPair kv : result) {
    if (strcmp(kv.key().c_str(), "last") == 0) continue;
    ohlcArr = kv.value().as<JsonArray>();
    break;
  }
  if (ohlcArr.isNull()) {
    Serial.println("[History] OHLC array missing or wrong type.");
    Serial.println("[History] Trying CoinGecko...");
    if (!bootstrapHistoryFromCoingeckoMarketChart()) {
      Serial.println("[History] CoinGecko history failed.");
    }
    return;
  }

  Serial.printf("[History] OHLC raw size: %u\n", (unsigned)ohlcArr.size());

  g_chartSampleCount = 0;
  dayAvgRollingReset();
  long minT = LONG_MAX;
  long maxT = LONG_MIN;
  int  kept = 0;

  for (JsonVariant v : ohlcArr) {
    JsonArray row = v.as<JsonArray>();
    if (row.isNull() || row.size() < 5) continue;

    long tUtc = row[0].as<long>();
    const char* closeStr = row[4] | "0";

    if (tUtc < minT) minT = tUtc;
    if (tUtc > maxT) maxT = tUtc;

 // Only add points within this cycle to the chart
    if (tUtc < windowStartUtc || tUtc > windowEndUtc) continue;

    float closePrice = atof(closeStr);
    dayAvgRollingAdd((time_t)tUtc, closePrice);
    addChartSampleUtc((time_t)tUtc, closePrice);
    kept++;
  }

  Serial.printf("[History] OHLC timestamp range UTC: %ld .. %ld\n",
                (minT == LONG_MAX ? 0 : minT),
                (maxT == LONG_MIN ? 0 : maxT));
  Serial.printf("[History] Kept %d samples into chart.\n", kept);
  Serial.printf("[History] g_chartSampleCount = %d\n", g_chartSampleCount);
}


// ------------------------------
// FX: USD -> TWD (NTD)
// ------------------------------
bool fetchUsdToTwdRate(float& outRate) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[FX] WiFi not connected; skip");
    return false;
  }

  const char* urls[] = {
    "https://open.er-api.com/v6/latest/USD",
    "https://api.frankfurter.app/latest?from=USD&to=TWD",
    "https://api.exchangerate.host/latest?base=USD&symbols=TWD",
  };

  for (size_t i = 0; i < sizeof(urls) / sizeof(urls[0]); i++) {
    const char* url = urls[i];
    HTTPClient http;
    http.setTimeout(6000);
    http.begin(url);

    Serial.printf("[FX] GET %s\n", url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
      Serial.printf("[FX] HTTP %d\n", httpCode);
      http.end();
      continue;
    }

    String payload = http.getString();
    http.end();

 // Only extract the rates.TWD field
    StaticJsonDocument<64> filter;
    filter["rates"]["TWD"] = true;

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (err) {
      Serial.printf("[FX] JSON error: %s\n", err.c_str());
      continue;
    }

    float rate = doc["rates"]["TWD"] | 0.0f;
    if (rate > 0.1f && rate < 500.0f) {
      outRate = rate;
      Serial.printf("[FX] USD->TWD: %.4f\n", outRate);
      return true;
    }

    Serial.println("[FX] Missing/invalid rates.TWD");
  }

  Serial.println("[FX] All endpoints failed");
  return false;
}
