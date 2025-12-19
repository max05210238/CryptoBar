#pragma once

#include <Arduino.h>

// 統一對外提供的「抓價」與「歷史 OHLC bootstrap」API

// 抓最新價格（優先 CoinPaprika，失敗 fallback CoinGecko，再失敗 fallback Kraken）
// 回傳：true = 成功，priceUsd / change24h 已填入
bool fetchPrice(float& priceUsd, float& change24h);

// 歷史曲線 bootstrap：
// - 有 Kraken pair：用 Kraken OHLC (5min interval)
// - 否則：用 CoinGecko market_chart (days=1)
void bootstrapHistoryFromKrakenOHLC();

// USD -> TWD 匯率 (NTD)
// 回傳：true = 成功，outRate 已填入
bool fetchUsdToTwdRate(float& outRate);
