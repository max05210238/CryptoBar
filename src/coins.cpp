#include "coins.h"
#include <string.h>

// NOTE: Stablecoins (USDT/USDC/RLUSD) intentionally omitted.
// NOTE: Order is automatically updated by scripts/update_coin_order.py
//       based on market cap rankings at build time.

static const CoinInfo kCoins[] = {
 // ticker, display, paprikaId, geckoId, krakenPair, binanceSymbol
  { "BTC", "BTC", "btc-bitcoin", "bitcoin", "XXBTZUSD", "BTCUSDT" },
  { "ETH", "ETH", "eth-ethereum", "ethereum", "XETHZUSD", "ETHUSDT" },
  { "BNB", "BNB", "bnb-binance-coin", "binancecoin", nullptr, "BNBUSDT" },
  { "XRP", "XRP", "xrp-xrp", "ripple", "XXRPZUSD", "XRPUSDT" },
  { "SOL", "SOL", "sol-solana", "solana", nullptr, "SOLUSDT" },
  { "TRX", "TRX", "trx-tron", "tron", nullptr, "TRXUSDT" },
  { "DOGE", "DOGE", "doge-dogecoin", "dogecoin", nullptr, "DOGEUSDT" },
  { "ADA", "ADA", "ada-cardano", "cardano", nullptr, "ADAUSDT" },
  { "BCH", "BCH", "bch-bitcoin-cash", "bitcoin-cash", nullptr, "BCHUSDT" },
  { "LINK", "LINK", "link-chainlink", "chainlink", nullptr, "LINKUSDT" },
  { "XMR", "XMR", "xmr-monero", "monero", nullptr, "XMRUSDT" },
  { "XLM", "XLM", "xlm-stellar", "stellar", nullptr, "XLMUSDT" },
  { "LTC", "LTC", "ltc-litecoin", "litecoin", nullptr, "LTCUSDT" },
  { "AVAX", "AVAX", "avax-avalanche", "avalanche-2", nullptr, "AVAXUSDT" },
  { "HBAR", "HBAR", "hbar-hedera-hashgraph", "hedera-hashgraph", nullptr, "HBARUSDT" },
  { "SHIB", "SHIB", "shib-shiba-inu", "shiba-inu", nullptr, "SHIBUSDT" },
  { "TON", "TON", "ton-toncoin", "toncoin", nullptr, "TONUSDT" },
  { "UNI", "UNI", "uni-uniswap", "uniswap", nullptr, "UNIUSDT" },
  { "DOT", "DOT", "dot-polkadot", "polkadot", nullptr, "DOTUSDT" },
  { "KAS", "KAS", "kas-kaspa", "kaspa", nullptr, "KASUSDT" },
  { "FLR", "FLR", "flr-flare-network", "flare", nullptr, nullptr },
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
