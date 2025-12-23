#pragma once
#include <Arduino.h>

// NOTE (V0.97): Coin registry moved to coins.h / coins.cpp (single source of truth).

// V0.99k: Default update interval changed to 120 seconds (2 minutes)
// Reason: Aggregated market data APIs (CoinPaprika, CoinGecko) update every 30s-2min
// Using 2-minute interval ensures fresh data and avoids duplicate price displays
static const uint32_t UPDATE_INTERVAL_MS = 120UL * 1000UL;  // 2 minutes

// ----------------- Timezone Configuration -----------------
struct TimezoneInfo {
  const char* label;           // Display string in settings menu
  int8_t      utcOffsetHours;  // UTC offset in hours (integer)
};

// V0.97: Complete integer timezones (UTC-12 ~ UTC+14).
// Note: TIMEZONES[] preserves legacy tzIndex order for compatibility,
// but the settings menu uses UTC-sorted order (see TIMEZONE_DISPLAY_ORDER[]).
// Names are deliberately short to fit on screen.
static const TimezoneInfo TIMEZONES[] = {
 // --- V0.97 legacy order (index 0..6) ---
  { "UTC+00 London",    0  },
  { "UTC+01 Berlin",    1  },
  { "UTC+02 Athens",    2  },
  { "UTC+04 Dubai",     4  },
  { "UTC+08 Taipei",    8  },   // Taiwan representative city
  { "UTC-05 New York", -5  },
  { "UTC-08 Seattle",  -8  },   // Default timezone

 // --- Additional integer UTC offsets ---
  { "UTC-12 Baker",     -12 },
  { "UTC-11 Samoa",     -11 },
  { "UTC-10 Hawaii",    -10 },
  { "UTC-09 Alaska",     -9 },
  { "UTC-07 Denver",     -7 },
  { "UTC-06 Chicago",    -6 },
  { "UTC-04 Santiago",   -4 },
  { "UTC-03 SaoPaulo",   -3 },
  { "UTC-02 S.Georgia",  -2 },
  { "UTC-01 Azores",     -1 },
  { "UTC+03 Riyadh",      3 },
  { "UTC+05 Karachi",     5 },
  { "UTC+06 Dhaka",       6 },
  { "UTC+07 Bangkok",     7 },
  { "UTC+09 Tokyo",       9 },
  { "UTC+10 Sydney",     10 },
  { "UTC+11 Noumea",     11 },
  { "UTC+12 Auckland",   12 },
  { "UTC+13 Apia",       13 },
  { "UTC+14 Kiritim",    14 },
};

static const uint8_t TIMEZONE_COUNT         = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

// Default timezone: Seattle
// Note: must be declared BEFORE any inline helper that references it.
static const uint8_t DEFAULT_TIMEZONE_INDEX = 6;   // index 6 = "UTC-08 Seattle"

// V0.97: Timezone list should be presented in UTC order (ascending),
// without changing the stored tzIndex meaning (tzIndex is still an index into TIMEZONES[]).
// The UI uses TIMEZONE_DISPLAY_ORDER[] to map display position -> timezone index.
static const uint8_t TIMEZONE_DISPLAY_ORDER[TIMEZONE_COUNT] = {
  7, 8, 9, 10, 6, 11, 12, 5, 13, 14, 15, 16, 0, 1, 2, 17, 3, 18, 19, 20, 4, 21, 22, 23, 24, 25, 26
};

static inline int tzIndexFromDisplayPos(int pos) {
  if (pos < 0 || pos >= (int)TIMEZONE_COUNT) return (int)DEFAULT_TIMEZONE_INDEX;
  return (int)TIMEZONE_DISPLAY_ORDER[pos];
}

static inline int tzDisplayPosFromTzIndex(int tzIdx) {
  for (int i = 0; i < (int)TIMEZONE_COUNT; ++i) {
    if ((int)TIMEZONE_DISPLAY_ORDER[i] == tzIdx) return i;
  }
  return 0;
}

// ----------------- Display Currency -----------------
// V0.99f: Multi-currency support (USD, TWD, EUR, GBP, CAD, JPY, KRW, SGD, AUD)
enum DisplayCurrency : uint8_t {
  CURR_USD = 0,  // US Dollar ($)
  CURR_TWD = 1,  // Taiwan Dollar (NT) - formerly CURR_NTD
  CURR_EUR = 2,  // Euro (€)
  CURR_GBP = 3,  // British Pound (£)
  CURR_CAD = 4,  // Canadian Dollar (C$)
  CURR_JPY = 5,  // Japanese Yen (¥) - no decimals
  CURR_KRW = 6,  // Korean Won (₩) - no decimals
  CURR_SGD = 7,  // Singapore Dollar (S$)
  CURR_AUD = 8,  // Australian Dollar (A$)
  CURR_COUNT
};

// For backward compatibility: CURR_NTD is an alias for CURR_TWD
static const uint8_t CURR_NTD = CURR_TWD;

// Currency metadata
struct CurrencyInfo {
  const char* code;        // 3-letter ISO code
  const char* symbol;      // Display symbol (UTF-8)
  bool        noDecimals;  // true for JPY, KRW (no fractional units)
  bool        twoCharSym;  // true for NT, C$, S$, A$ (use smaller font)
};

static const CurrencyInfo CURRENCY_INFO[CURR_COUNT] = {
  { "USD", "$",   false, false },  // US Dollar
  { "TWD", "NT",  false, true  },  // Taiwan Dollar (NT compressed)
  { "EUR", "EUR", false, true  },  // Euro (ASCII-safe, compressed)
  { "GBP", "GBP", false, true  },  // British Pound (ASCII-safe, compressed)
  { "CAD", "C$",  false, true  },  // Canadian Dollar
  { "JPY", "JPY", true,  true  },  // Japanese Yen (no decimals, ASCII-safe)
  { "KRW", "KRW", true,  true  },  // Korean Won (no decimals, ASCII-safe)
  { "SGD", "S$",  false, true  },  // Singapore Dollar
  { "AUD", "A$",  false, true  },  // Australian Dollar
};
