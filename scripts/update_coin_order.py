#!/usr/bin/env python3
"""
update_coin_order.py - Fetch latest market cap rankings and update coins.cpp

This script is run automatically before each build (via PlatformIO extra_scripts).
It fetches the current market cap rankings from multiple API sources and reorders
the coin list in src/coins.cpp accordingly.

API Sources (tried in order):
1. CoinGecko - Primary source
2. CoinPaprika - Backup source
3. CoinCap - Second backup source

Usage:
  - Automatic: PlatformIO runs this before build (via extra_scripts = pre:scripts/update_coin_order.py)
  - Manual: python scripts/update_coin_order.py

Requirements:
  - Python 3.6+
  - requests library (pip install requests)
"""

import json
import os
import sys
import time

# Global variable to store project directory (set by PlatformIO hook or standalone detection)
_project_dir = None

# Supported coins with their API identifiers
# Format: (ticker, display, paprikaId, geckoId, coincapId, krakenPair, binanceSymbol)
SUPPORTED_COINS = [
    ("BTC",  "BTC",  "btc-bitcoin",          "bitcoin",           "bitcoin",           "XXBTZUSD",  "BTCUSDT"),
    ("ETH",  "ETH",  "eth-ethereum",         "ethereum",          "ethereum",          "XETHZUSD",  "ETHUSDT"),
    ("BNB",  "BNB",  "bnb-binance-coin",     "binancecoin",       "binance-coin",      None,        "BNBUSDT"),
    ("XRP",  "XRP",  "xrp-xrp",              "ripple",            "xrp",               "XXRPZUSD",  "XRPUSDT"),
    ("SOL",  "SOL",  "sol-solana",           "solana",            "solana",            None,        "SOLUSDT"),
    ("TRX",  "TRX",  "trx-tron",             "tron",              "tron",              None,        "TRXUSDT"),
    ("DOGE", "DOGE", "doge-dogecoin",        "dogecoin",          "dogecoin",          None,        "DOGEUSDT"),
    ("ADA",  "ADA",  "ada-cardano",          "cardano",           "cardano",           None,        "ADAUSDT"),
    ("BCH",  "BCH",  "bch-bitcoin-cash",     "bitcoin-cash",      "bitcoin-cash",      None,        "BCHUSDT"),
    ("LINK", "LINK", "link-chainlink",       "chainlink",         "chainlink",         None,        "LINKUSDT"),
    ("XMR",  "XMR",  "xmr-monero",           "monero",            "monero",            None,        "XMRUSDT"),
    ("XLM",  "XLM",  "xlm-stellar",          "stellar",           "stellar",           None,        "XLMUSDT"),
    ("LTC",  "LTC",  "ltc-litecoin",         "litecoin",          "litecoin",          None,        "LTCUSDT"),
    ("AVAX", "AVAX", "avax-avalanche",       "avalanche-2",       "avalanche",         None,        "AVAXUSDT"),
    ("HBAR", "HBAR", "hbar-hedera-hashgraph","hedera-hashgraph",  "hedera-hashgraph",  None,        "HBARUSDT"),
    ("SHIB", "SHIB", "shib-shiba-inu",       "shiba-inu",         "shiba-inu",         None,        "SHIBUSDT"),
    ("TON",  "TON",  "ton-toncoin",          "the-open-network",  "toncoin",           None,        "TONUSDT"),
    ("UNI",  "UNI",  "uni-uniswap",          "uniswap",           "uniswap",           None,        "UNIUSDT"),
    ("DOT",  "DOT",  "dot-polkadot",         "polkadot",          "polkadot",          None,        "DOTUSDT"),
    ("KAS",  "KAS",  "kas-kaspa",            "kaspa",             "kaspa",             None,        "KASUSDT"),
    ("FLR",  "FLR",  "flr-flare-network",    "flare",             "flare",             None,        None),
]

# API endpoints
COINGECKO_API_URL = "https://api.coingecko.com/api/v3/coins/markets"
COINPAPRIKA_API_URL = "https://api.coinpaprika.com/v1/tickers"
COINCAP_API_URL = "https://api.coincap.io/v2/assets"

def get_project_dir():
    """Get the project root directory."""
    global _project_dir
    if _project_dir:
        return _project_dir
    # Fallback for standalone execution
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(script_dir)

def get_coins_cpp_path():
    """Get the path to coins.cpp."""
    return os.path.join(get_project_dir(), "src", "coins.cpp")

def fetch_from_coingecko(requests):
    """Fetch market cap rankings from CoinGecko API."""
    gecko_ids = [coin[3] for coin in SUPPORTED_COINS]

    params = {
        "vs_currency": "usd",
        "ids": ",".join(gecko_ids),
        "order": "market_cap_desc",
        "per_page": 250,
        "page": 1,
        "sparkline": "false",
    }

    headers = {
        "Accept": "application/json",
        "User-Agent": "CryptoBar/1.0",
    }

    print("[CoinOrder] Trying CoinGecko API...")
    response = requests.get(COINGECKO_API_URL, params=params, headers=headers, timeout=30)
    response.raise_for_status()
    data = response.json()

    # Build a map of geckoId -> market_cap_rank
    rank_map = {}
    for coin in data:
        gecko_id = coin.get("id")
        rank = coin.get("market_cap_rank")
        if gecko_id and rank:
            rank_map[gecko_id] = rank

    print(f"[CoinOrder] CoinGecko: Retrieved rankings for {len(rank_map)} coins")
    return rank_map, "gecko"

def fetch_from_coinpaprika(requests):
    """Fetch market cap rankings from CoinPaprika API."""
    paprika_ids = [coin[2] for coin in SUPPORTED_COINS]

    headers = {
        "Accept": "application/json",
        "User-Agent": "CryptoBar/1.0",
    }

    print("[CoinOrder] Trying CoinPaprika API...")
    response = requests.get(COINPAPRIKA_API_URL, headers=headers, timeout=30)
    response.raise_for_status()
    data = response.json()

    # Build a map of paprikaId -> rank
    rank_map = {}
    for coin in data:
        paprika_id = coin.get("id")
        rank = coin.get("rank")
        if paprika_id and rank and paprika_id in paprika_ids:
            rank_map[paprika_id] = rank

    print(f"[CoinOrder] CoinPaprika: Retrieved rankings for {len(rank_map)} coins")
    return rank_map, "paprika"

def fetch_from_coincap(requests):
    """Fetch market cap rankings from CoinCap API."""
    coincap_ids = [coin[4] for coin in SUPPORTED_COINS]

    headers = {
        "Accept": "application/json",
        "User-Agent": "CryptoBar/1.0",
    }

    print("[CoinOrder] Trying CoinCap API...")
    response = requests.get(COINCAP_API_URL, params={"limit": 250}, headers=headers, timeout=30)
    response.raise_for_status()
    data = response.json().get("data", [])

    # Build a map of coincapId -> rank
    rank_map = {}
    for coin in data:
        coincap_id = coin.get("id")
        rank = coin.get("rank")
        if coincap_id and rank and coincap_id in coincap_ids:
            rank_map[coincap_id] = int(rank)

    print(f"[CoinOrder] CoinCap: Retrieved rankings for {len(rank_map)} coins")
    return rank_map, "coincap"

def fetch_market_ranks():
    """Fetch market cap rankings, merging data from multiple APIs to fill gaps."""
    try:
        import requests
    except ImportError:
        print("[CoinOrder] Warning: 'requests' library not installed. Skipping coin order update.")
        print("[CoinOrder] To enable automatic coin ordering, run: pip install requests")
        return None, None

    # Build a unified rank map keyed by ticker
    # This allows merging data from different APIs
    unified_ranks = {}  # ticker -> rank

    # Helper to get missing tickers
    def get_missing_tickers():
        all_tickers = {coin[0] for coin in SUPPORTED_COINS}
        return all_tickers - set(unified_ranks.keys())

    # Try CoinGecko first (primary source)
    try:
        gecko_map, _ = fetch_from_coingecko(requests)
        # Map geckoId -> ticker for lookup
        gecko_to_ticker = {coin[3]: coin[0] for coin in SUPPORTED_COINS}
        for gecko_id, rank in gecko_map.items():
            ticker = gecko_to_ticker.get(gecko_id)
            if ticker:
                unified_ranks[ticker] = rank
        print(f"[CoinOrder] After CoinGecko: {len(unified_ranks)}/{len(SUPPORTED_COINS)} coins have ranks")
    except Exception as e:
        print(f"[CoinOrder] CoinGecko failed: {e}")

    # Check for missing coins and try CoinPaprika
    missing = get_missing_tickers()
    if missing:
        print(f"[CoinOrder] Missing coins: {', '.join(sorted(missing))} - trying CoinPaprika...")
        try:
            paprika_map, _ = fetch_from_coinpaprika(requests)
            # Map paprikaId -> ticker for lookup
            paprika_to_ticker = {coin[2]: coin[0] for coin in SUPPORTED_COINS}
            for paprika_id, rank in paprika_map.items():
                ticker = paprika_to_ticker.get(paprika_id)
                if ticker and ticker in missing:
                    unified_ranks[ticker] = rank
                    print(f"[CoinOrder]   Found {ticker} via CoinPaprika (rank #{rank})")
        except Exception as e:
            print(f"[CoinOrder] CoinPaprika failed: {e}")

    # Check for still missing coins and try CoinCap
    missing = get_missing_tickers()
    if missing:
        print(f"[CoinOrder] Still missing: {', '.join(sorted(missing))} - trying CoinCap...")
        try:
            coincap_map, _ = fetch_from_coincap(requests)
            # Map coincapId -> ticker for lookup
            coincap_to_ticker = {coin[4]: coin[0] for coin in SUPPORTED_COINS}
            for coincap_id, rank in coincap_map.items():
                ticker = coincap_to_ticker.get(coincap_id)
                if ticker and ticker in missing:
                    unified_ranks[ticker] = rank
                    print(f"[CoinOrder]   Found {ticker} via CoinCap (rank #{rank})")
        except Exception as e:
            print(f"[CoinOrder] CoinCap failed: {e}")

    # Final check
    missing = get_missing_tickers()
    if missing:
        print(f"[CoinOrder] Warning: No rank data for: {', '.join(sorted(missing))} (will be placed at end)")

    if len(unified_ranks) >= 10:
        return unified_ranks, "unified"
    elif len(unified_ranks) > 0:
        print(f"[CoinOrder] Warning: Only {len(unified_ranks)} coins have ranks, but proceeding anyway")
        return unified_ranks, "unified"
    else:
        print("[CoinOrder] All APIs failed. Using last known order.")
        return None, None

def sort_coins_by_rank(rank_map, source):
    """Sort SUPPORTED_COINS by market cap rank."""
    if rank_map is None:
        # Fallback: keep original order (which should be market cap order from last successful fetch)
        print("[CoinOrder] Using existing order (no API available)")
        return SUPPORTED_COINS

    # For unified source, rank_map is keyed by ticker
    # For single-source fallback, determine the appropriate index
    if source == "unified":
        def get_rank(coin):
            ticker = coin[0]
            return rank_map.get(ticker, 99999)
        def get_display_rank(coin):
            ticker = coin[0]
            return rank_map.get(ticker, "N/A")
    else:
        # Legacy single-source mode (fallback)
        source_index = {
            "gecko": 3,     # geckoId
            "paprika": 2,   # paprikaId
            "coincap": 4,   # coincapId
        }.get(source, 3)
        def get_rank(coin):
            api_id = coin[source_index]
            return rank_map.get(api_id, 99999)
        def get_display_rank(coin):
            api_id = coin[source_index]
            return rank_map.get(api_id, "N/A")

    sorted_coins = sorted(SUPPORTED_COINS, key=get_rank)

    # Print the new order
    print(f"[CoinOrder] Final order by market cap:")
    for i, coin in enumerate(sorted_coins, 1):
        rank = get_display_rank(coin)
        print(f"  {i:2}. {coin[0]:<5} (rank #{rank})")

    return sorted_coins

def generate_coins_cpp(sorted_coins):
    """Generate the new coins.cpp content."""
    lines = []
    lines.append('#include "coins.h"')
    lines.append('#include <string.h>')
    lines.append('')
    lines.append('// NOTE: Stablecoins (USDT/USDC/RLUSD) intentionally omitted.')
    lines.append('// NOTE: Order is automatically updated by scripts/update_coin_order.py')
    lines.append('//       based on market cap rankings at build time.')
    lines.append('')
    lines.append('static const CoinInfo kCoins[] = {')
    lines.append(' // ticker, display, paprikaId, geckoId, krakenPair, binanceSymbol')

    for coin in sorted_coins:
        # Format: (ticker, display, paprikaId, geckoId, coincapId, krakenPair, binanceSymbol)
        ticker, display, paprika_id, gecko_id, coincap_id, kraken_pair, binance_symbol = coin

        # Format each field
        kraken_str = f'"{kraken_pair}"' if kraken_pair else "nullptr"
        binance_str = f'"{binance_symbol}"' if binance_symbol else "nullptr"

        line = f'  {{ "{ticker}", "{display}", "{paprika_id}", "{gecko_id}", {kraken_str}, {binance_str} }},'
        lines.append(line)

    lines.append('};')
    lines.append('')
    lines.append('int coinCount() {')
    lines.append('  return (int)(sizeof(kCoins) / sizeof(kCoins[0]));')
    lines.append('}')
    lines.append('')
    lines.append('const CoinInfo& coinAt(int idx) {')
    lines.append('  if (idx < 0) idx = 0;')
    lines.append('  int n = coinCount();')
    lines.append('  if (idx >= n) idx = n - 1;')
    lines.append('  return kCoins[idx];')
    lines.append('}')
    lines.append('')
    lines.append('static int findIndexInternal(const char* ticker) {')
    lines.append('  if (!ticker || !ticker[0]) return -1;')
    lines.append('  for (int i = 0; i < coinCount(); ++i) {')
    lines.append('    if (strcasecmp(kCoins[i].ticker, ticker) == 0) return i;')
    lines.append('  }')
    lines.append('  return -1;')
    lines.append('}')
    lines.append('')
    lines.append('const CoinInfo& coinDefault() {')
    lines.append(' // V0.97: default coin is BTC.')
    lines.append('  int idx = findIndexInternal("BTC");')
    lines.append('  if (idx < 0) idx = 0;')
    lines.append('  return kCoins[idx];')
    lines.append('}')
    lines.append('')
    lines.append('int coinIndexFromTicker(const char* ticker) {')
    lines.append('  int idx = findIndexInternal(ticker);')
    lines.append('  if (idx < 0) idx = findIndexInternal(coinDefault().ticker);')
    lines.append('  if (idx < 0) idx = 0;')
    lines.append('  return idx;')
    lines.append('}')
    lines.append('')

    return '\n'.join(lines)

def update_coins():
    """Main function to update coins.cpp."""
    print("[CoinOrder] Starting coin order update...")

    # Fetch market rankings (tries multiple APIs)
    rank_map, source = fetch_market_ranks()

    # Sort coins by rank
    sorted_coins = sort_coins_by_rank(rank_map, source)

    # Generate new coins.cpp
    new_content = generate_coins_cpp(sorted_coins)

    # Write to file
    coins_cpp_path = get_coins_cpp_path()
    print(f"[CoinOrder] Writing to {coins_cpp_path}")

    with open(coins_cpp_path, 'w') as f:
        f.write(new_content)

    print(f"[CoinOrder] Successfully updated coins.cpp with {len(sorted_coins)} coins")

# PlatformIO pre-build hook
try:
    Import("env")
    # We're running inside PlatformIO - set project directory from env
    _project_dir = env['PROJECT_DIR']

    def before_build(source, target, env):
        global _project_dir
        _project_dir = env['PROJECT_DIR']
        update_coins()

    env.AddPreAction("buildprog", before_build)
    print("[CoinOrder] Registered PlatformIO pre-build hook")

except NameError:
    # We're running standalone (not inside PlatformIO)
    if __name__ == "__main__":
        update_coins()
        sys.exit(0)
