#pragma once

#include <Arduino.h>

// Unified API for price fetching and historical OHLC bootstrap

// Fetch latest price (priority: CoinPaprika, fallback to CoinGecko, fallback to Kraken)
// Returns: true = success, priceUsd / change24h are populated
bool fetchPrice(float& priceUsd, float& change24h);

// Historical chart bootstrap:
// - If Kraken pair configured: use Kraken OHLC (5min interval)
// - Otherwise: use CoinGecko market_chart (days=1)
void bootstrapHistoryFromKrakenOHLC();

// USD -> TWD exchange rate (NTD)
// Returns: true = success, outRate is populated
bool fetchUsdToTwdRate(float& outRate);
