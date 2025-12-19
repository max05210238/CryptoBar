#include "coins.h"
#include <string.h>

// NOTE: Stablecoins (USDT/USDC/RLUSD) intentionally omitted.

static const CoinInfo kCoins[] = {
 // ticker, display, rank, paprikaId, geckoId, krakenPair
  { "BTC",  "BTC",  1,  "btc-bitcoin",         "bitcoin",          "XXBTZUSD"  },
  { "ETH",  "ETH",  2,  "eth-ethereum",        "ethereum",         "XETHZUSD"  },
  { "BNB",  "BNB",  4,  "bnb-binance-coin",    "binancecoin",      nullptr     },
  { "XRP",  "XRP",  5,  "xrp-xrp",             "ripple",           "XXRPZUSD"  },
  { "SOL",  "SOL",  7,  "sol-solana",          "solana",           nullptr     },
  { "TRX",  "TRX",  9,  "trx-tron",            "tron",             nullptr     },
  { "DOGE", "DOGE", 10, "doge-dogecoin",       "dogecoin",         nullptr     },
  { "ADA",  "ADA",  11, "ada-cardano",         "cardano",          nullptr     },
  { "BCH",  "BCH",  15, "bch-bitcoin-cash",    "bitcoin-cash",     nullptr     },
  { "LINK", "LINK", 19, "link-chainlink",      "chainlink",        nullptr     },
  { "XMR",  "XMR",  25, "xmr-monero",          "monero",           nullptr     },
  { "XLM",  "XLM",  26, "xlm-stellar",         "stellar",          nullptr     },
  { "LTC",  "LTC",  30, "ltc-litecoin",        "litecoin",         nullptr     },
  { "AVAX", "AVAX", 32, "avax-avalanche",      "avalanche-2",      nullptr     },
  { "HBAR", "HBAR", 33, "hbar-hedera-hashgraph","hedera-hashgraph", nullptr    },
  { "SHIB", "SHIB", 34, "shib-shiba-inu",      "shiba-inu",        nullptr     },
  { "TON",  "TON",  39, "ton-toncoin",         "toncoin",          nullptr     },
  { "UNI",  "UNI",  44, "uni-uniswap",         "uniswap",          nullptr     },
  { "DOT",  "DOT",  45, "dot-polkadot",        "polkadot",         nullptr     },
  { "KAS",  "KAS",  86, "kas-kaspa",           "kaspa",            nullptr     },
};

int coinCount() {
  return (int)(sizeof(kCoins) / sizeof(kCoins[0]));
}

const CoinInfo& coinAt(int idx) {
  if (idx < 0) idx = 0;
  int n = coinCount();
  if (idx >= n) idx = n - 1;
  return kCoins[idx];
}

static int findIndexInternal(const char* ticker) {
  if (!ticker || !ticker[0]) return -1;
  for (int i = 0; i < coinCount(); ++i) {
    if (strcasecmp(kCoins[i].ticker, ticker) == 0) return i;
  }
  return -1;
}

const CoinInfo& coinDefault() {
 // V0.97: default coin is BTC.
  int idx = findIndexInternal("BTC");
  if (idx < 0) idx = 0;
  return kCoins[idx];
}

int coinIndexFromTicker(const char* ticker) {
  int idx = findIndexInternal(ticker);
  if (idx < 0) idx = findIndexInternal(coinDefault().ticker);
  if (idx < 0) idx = 0;
  return idx;
}
