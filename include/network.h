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

// V0.99f: Fetch exchange rates for all supported currencies
// Updates g_usdToRate[] array
// Returns: true = success (at least 50% of rates fetched)
bool fetchExchangeRates();

// USD -> TWD exchange rate (backward compatibility wrapper)
// Returns: true = success, outRate is populated
bool fetchUsdToTwdRate(float& outRate);
