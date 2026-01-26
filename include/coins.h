#pragma once
#include <Arduino.h>

// Coin registry (single source of truth).
// - NVS stores 'ticker' (e.g., "XRP").
// - UI list order is automatically sorted by market cap at build time.
//   (see scripts/update_coin_order.py)
// - paprikaId/geckoId/krakenPair are provider-specific identifiers.

struct CoinInfo {
  const char* ticker;        // persisted key + UI ticker
  const char* display;       // display label (currently same as ticker)
  const char* paprikaId;     // e.g., "xrp-xrp"
  const char* geckoId;       // e.g., "ripple"
  const char* krakenPair;    // e.g., "XXRPZUSD" (nullptr/"" if not available)
  const char* binanceSymbol; // e.g., "XRPUSDT" (nullptr/"" if not available)
};

int coinCount();
const CoinInfo& coinAt(int idx);

// Returns index for ticker, or default coin index if not found/invalid.
int coinIndexFromTicker(const char* ticker);

// Default coin (chosen for backward-compatibility with earlier builds).
const CoinInfo& coinDefault();
