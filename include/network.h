#pragma once

#include <Arduino.h>

// Unified API for price fetching and historical OHLC bootstrap

// V0.99g: Fetch latest price (4-layer fallback: Paprika → Binance → CoinGecko → Kraken)
// Returns: true = success, priceUsd / change24h are populated
bool fetchPrice(float& priceUsd, float& change24h);

// V0.99g: Historical chart bootstrap (3-layer fallback):
// - If Kraken pair configured: use Kraken OHLC (5min interval)
//   - Fallback: Binance klines → CoinGecko market_chart
// - Otherwise: Binance klines → CoinGecko market_chart (days=1)
void bootstrapHistoryFromKrakenOHLC();

// V0.99f: Fetch exchange rates for all supported currencies
// Updates g_usdToRate[] array
// Returns: true = success (at least 50% of rates fetched)
bool fetchExchangeRates();

// USD -> TWD exchange rate (backward compatibility wrapper)
// Returns: true = success, outRate is populated
bool fetchUsdToTwdRate(float& outRate);
