#pragma once
#include <Arduino.h>

// NOTE (V0.97): Coin registry moved to coins.h / coins.cpp (single source of truth).

// 預設更新間隔（毫秒）– 30 秒
static const uint32_t UPDATE_INTERVAL_MS = 30UL * 1000UL;

// ----------------- 時區設定 -----------------
struct TimezoneInfo {
  const char* label;           // 設定選單裡顯示的字串
  int8_t      utcOffsetHours;  // 相對 UTC 的小時數（整數）
};

// V0.97: 補齊整數時區（UTC-12 ~ UTC+14）。
// 注意：TIMEZONES[] 仍保留舊版 tzIndex 的順序相容性；但「設定列表顯示」會用 UTC 排序（見 TIMEZONE_DISPLAY_ORDER[]）。
// 名稱刻意縮短，避免跑到畫面左邊
static const TimezoneInfo TIMEZONES[] = {
 // --- V0.97 legacy order (index 0..6) ---
  { "UTC+00 London",    0  },
  { "UTC+01 Berlin",    1  },
  { "UTC+02 Athens",    2  },
  { "UTC+04 Dubai",     4  },
  { "UTC+08 Taipei",    8  },   // 台灣代表城市固定用 Taipei
  { "UTC-05 New York", -5  },
  { "UTC-08 Seattle",  -8  },   // 這一個要當預設

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

// ✅ 預設時區：Seattle
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

// ----------------- 顯示幣別 -----------------
// 0 = USD, 1 = NTD (TWD)
enum DisplayCurrency : uint8_t {
  CURR_USD = 0,
  CURR_NTD = 1,
  CURR_COUNT
};
