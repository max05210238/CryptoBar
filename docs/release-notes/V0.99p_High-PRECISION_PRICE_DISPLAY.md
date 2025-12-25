# V0.99p: High-Precision Price Display with CoinGecko Enhancement

## Summary

V0.99p introduces enhanced price precision display using CoinGecko's `precision=full` parameter and intelligent length-based decimal formatting.

## Key Features

### 🔍 CoinGecko API Precision Enhancement
- Added `precision=full` parameter to `/simple/price` endpoint
- API now returns 14-16 decimal places (e.g., XRP: 1.86350365846271)
- Verified working with free CoinGecko API (no additional limits)

### 📏 Length-based Decimal Display (max 10 characters)
- Displays 4 decimals when total length ≤ 10 characters
- Auto-degrades to 2 decimals if 4-decimal format exceeds limit
- Auto-degrades to 0 decimals (integer) if 2-decimal format exceeds limit
- Example: "87619.3624" (10 chars, 4 decimals) → "134222.21" (9 chars, 2 decimals) → "13422202" (8 chars, 0 decimals)

### 🔢 Trailing Zeros Preserved
- Displays "1.8600" instead of "1.86" to indicate API precision limit
- Helps users identify when fallback APIs (Kraken/Binance) are used
- Example: "87620.00" indicates low-precision API vs "87619.36" high-precision

### 🌐 JPY/KRW Decimal Support
- Removed special restriction that forced JPY/KRW to display only integers
- Now applies unified length-based logic to all currencies
- Example: ETH in KRW now shows decimals when appropriate

## Display Examples

| Price (USD) | Total Length | Display |
|-------------|--------------|---------|
| 1.8635      | 6 chars      | 1.8635 (4 decimals) |
| 888.5000    | 8 chars      | 888.5000 (4 decimals) |
| 87619.3624  | 10 chars     | 87619.3624 (4 decimals) |
| 134222.2088 | 11 chars     | 134222.21 (2 decimals) |
| 1342220.21  | 10 chars     | 1342220.21 (2 decimals) |
| 13422202.21 | 11 chars     | 13422202 (0 decimals) |

## Technical Details

- **Files modified**: network.cpp, ui.cpp, main.cpp, day_avg.cpp, CHANGELOG.md
- **API URL change**: Added `&precision=full` query parameter
- **Display logic**: Length-based algorithm replaces trailing-zero detection
- **Currency handling**: Unified decimal logic for all currencies (removed `noDecimals` flag)

## Testing

- ✅ XRP displays 4 decimals (previously 2)
- ✅ BTC displays appropriate precision
- ✅ JPY/KRW currencies show decimals when appropriate
- ✅ ET Cycle chart display (7pm-7pm) works correctly
- ✅ Rolling 24h mean and Cycle mean both functional
- ✅ Length-based display handles all price ranges correctly

## Commits

Total commits: 15 (including debug iterations and final cleanup)
Final commit: ddf76ff "V0.99p: Remove debug logging and finalize release"

## Branch Information

**Source Branch**: `claude/increase-price-decimal-precision-e2Sm8`
**Base Commit**: 7d45dc1 (V0.99o)

## Notes

- All debug logging has been removed
- Code is production-ready
- CHANGELOG.md updated with release date 2025-12-25
