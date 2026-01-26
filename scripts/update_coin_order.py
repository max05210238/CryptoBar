#!/usr/bin/env python3
"""
update_coin_order.py - Fetch latest market cap rankings and update coins.cpp

This script is run automatically before each build (via PlatformIO extra_scripts).
It fetches the current market cap rankings from CoinGecko API and reorders
the coin list in src/coins.cpp accordingly.

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

# Supported coins with their API identifiers
# Format: (ticker, display, paprikaId, geckoId, krakenPair, binanceSymbol)
SUPPORTED_COINS = [
    ("BTC",  "BTC",  "btc-bitcoin",          "bitcoin",           "XXBTZUSD",  "BTCUSDT"),
    ("ETH",  "ETH",  "eth-ethereum",         "ethereum",          "XETHZUSD",  "ETHUSDT"),
    ("BNB",  "BNB",  "bnb-binance-coin",     "binancecoin",       None,        "BNBUSDT"),
    ("XRP",  "XRP",  "xrp-xrp",              "ripple",            "XXRPZUSD",  "XRPUSDT"),
    ("SOL",  "SOL",  "sol-solana",           "solana",            None,        "SOLUSDT"),
    ("TRX",  "TRX",  "trx-tron",             "tron",              None,        "TRXUSDT"),
    ("DOGE", "DOGE", "doge-dogecoin",        "dogecoin",          None,        "DOGEUSDT"),
    ("ADA",  "ADA",  "ada-cardano",          "cardano",           None,        "ADAUSDT"),
    ("BCH",  "BCH",  "bch-bitcoin-cash",     "bitcoin-cash",      None,        "BCHUSDT"),
    ("LINK", "LINK", "link-chainlink",       "chainlink",         None,        "LINKUSDT"),
    ("XMR",  "XMR",  "xmr-monero",           "monero",            None,        "XMRUSDT"),
    ("XLM",  "XLM",  "xlm-stellar",          "stellar",           None,        "XLMUSDT"),
    ("LTC",  "LTC",  "ltc-litecoin",         "litecoin",          None,        "LTCUSDT"),
    ("AVAX", "AVAX", "avax-avalanche",       "avalanche-2",       None,        "AVAXUSDT"),
    ("HBAR", "HBAR", "hbar-hedera-hashgraph","hedera-hashgraph",  None,        "HBARUSDT"),
    ("SHIB", "SHIB", "shib-shiba-inu",       "shiba-inu",         None,        "SHIBUSDT"),
    ("TON",  "TON",  "ton-toncoin",          "toncoin",           None,        "TONUSDT"),
    ("UNI",  "UNI",  "uni-uniswap",          "uniswap",           None,        "UNIUSDT"),
    ("DOT",  "DOT",  "dot-polkadot",         "polkadot",          None,        "DOTUSDT"),
    ("KAS",  "KAS",  "kas-kaspa",            "kaspa",             None,        "KASUSDT"),
    ("FLR",  "FLR",  "flr-flare-network",    "flare",             None,        None),
]

COINGECKO_API_URL = "https://api.coingecko.com/api/v3/coins/markets"

def get_project_dir():
    """Get the project root directory."""
    # When run from PlatformIO, we need to find the project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(script_dir)

def get_coins_cpp_path():
    """Get the path to coins.cpp."""
    return os.path.join(get_project_dir(), "src", "coins.cpp")

def fetch_market_ranks():
    """Fetch market cap rankings from CoinGecko API."""
    try:
        import requests
    except ImportError:
        print("[CoinOrder] Warning: 'requests' library not installed. Skipping coin order update.")
        print("[CoinOrder] To enable automatic coin ordering, run: pip install requests")
        return None

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

    try:
        print("[CoinOrder] Fetching market rankings from CoinGecko...")
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

        print(f"[CoinOrder] Retrieved rankings for {len(rank_map)} coins")
        return rank_map

    except Exception as e:
        print(f"[CoinOrder] Warning: Failed to fetch rankings: {e}")
        print("[CoinOrder] Using default order (alphabetical by ticker)")
        return None

def sort_coins_by_rank(rank_map):
    """Sort SUPPORTED_COINS by market cap rank."""
    if rank_map is None:
        # Fallback: sort alphabetically by ticker
        return sorted(SUPPORTED_COINS, key=lambda c: c[0])

    def get_rank(coin):
        gecko_id = coin[3]
        # If not found in rank_map, put at end (use large number)
        return rank_map.get(gecko_id, 99999)

    sorted_coins = sorted(SUPPORTED_COINS, key=get_rank)

    # Print the new order
    print("[CoinOrder] New order by market cap:")
    for i, coin in enumerate(sorted_coins, 1):
        gecko_id = coin[3]
        rank = rank_map.get(gecko_id, "N/A")
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
    lines.append('//       based on CoinGecko market cap rankings at build time.')
    lines.append('')
    lines.append('static const CoinInfo kCoins[] = {')
    lines.append(' // ticker, display, paprikaId, geckoId, krakenPair, binanceSymbol')

    for coin in sorted_coins:
        ticker, display, paprika_id, gecko_id, kraken_pair, binance_symbol = coin

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

    # Fetch market rankings
    rank_map = fetch_market_ranks()

    # Sort coins by rank
    sorted_coins = sort_coins_by_rank(rank_map)

    # Generate new coins.cpp
    new_content = generate_coins_cpp(sorted_coins)

    # Write to file
    coins_cpp_path = get_coins_cpp_path()
    print(f"[CoinOrder] Writing to {coins_cpp_path}")

    with open(coins_cpp_path, 'w') as f:
        f.write(new_content)

    print(f"[CoinOrder] Successfully updated coins.cpp with {len(sorted_coins)} coins")

# PlatformIO pre-build hook
# This is called automatically when PlatformIO loads this script
try:
    Import("env")
    # We're running inside PlatformIO
    def before_build(source, target, env):
        update_coins()

    env.AddPreAction("buildprog", before_build)
    print("[CoinOrder] Registered PlatformIO pre-build hook")

except NameError:
    # We're running standalone (not inside PlatformIO)
    if __name__ == "__main__":
        update_coins()
        sys.exit(0)
