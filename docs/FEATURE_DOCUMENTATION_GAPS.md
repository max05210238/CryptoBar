# CryptoBar 未記錄功能與機制分析報告

**生成時間:** 2025-12-26
**分析範圍:** 源代碼 vs. 現有用戶文檔
**目的:** 識別已實現但未充分記錄的功能和內部機制

---

## 📖 目錄

### 概覽與分析
1. [📊 現有文檔概覽](#現有文檔概覽)
2. [🔍 功能差距分析](#功能差距分析)

### 高優先級（用戶直接可見）
- [1.1 多幣種顯示系統（9種貨幣）](#11-多幣種顯示系統9種貨幣) ✅ **已完成**
- [1.2 Day Average Mode（日均價參考線）](#12-day-average-mode日均價參考線) ✅ **已完成**
- [1.3 獨立時間刷新機制](#13-獨立時間刷新機制v099q-新功能) ✅ **已確認**
- [1.4 更新間隔預設值修正](#14-更新間隔預設值修正) ✅ **已完成**
- [1.5 維護模式啟動方式](#15-維護模式啟動方式boot-time-trigger) ✅ **已完成**

### 中優先級（內部機制）
- [2.1 Prefetch 調度機制](#21-prefetch-調度機制預取優化)
- [2.2 Runtime WiFi 重連機制](#22-runtime-wifi-重連機制)
- [2.3 Partial/Full Refresh 策略](#23-partialfull-refresh-策略) ✅ **已修正**
- [2.4 API Fallback 詳細順序](#24-api-fallback-詳細順序)
- [2.5 NTP 重新同步機制](#25-ntp-重新同步機制) ✅ **已修正**
- [2.6 Timezone 自動檢測](#26-timezone-自動檢測)

### 低優先級（開發者技術）
- [3.1 編碼器配置參數](#31-編碼器配置參數) ✅
- [3.2 LED 動畫任務](#32-led-動畫任務) ✅

### 行動計劃
3. [📝 建議的文檔改進優先級](#建議的文檔改進優先級)
4. [🎯 推薦行動計劃](#推薦行動計劃)
   - Phase 1: 立即更新（高優先級 1-5）✅ **已完成**
   - Phase 2: 完善補充（中優先級 6-10）
   - Phase 3: 可選高級內容
5. [📊 文檔完整性評分](#文檔完整性評分)
6. [📌 總結](#總結)

---

## 📊 現有文檔概覽

### 已有的用戶指南

| 文檔 | 行數 | 主要內容 |
|------|------|----------|
| `DISPLAY_GUIDE.md` | 558 | 主顯示、菜單系統、12個菜單項、屏幕狀態 |
| `LED_DISPLAY_GUIDE.md` | 243 | LED 顏色含義、呼吸動畫、Party Mode |
| `OTA_UPDATE_GUIDE.md` | 940 | OTA 更新流程、安全機制、A/B 分區回滾 |
| `HARDWARE_GUIDE.md` | 394 | 硬件組裝、BOM、接線圖 |

### 新增的開發者文檔

| 文檔 | 行數 | 主要內容 |
|------|------|----------|
| `src/README.md` | ~500 | 26 個源代碼模組詳細說明 |
| `include/README.md` | ~650 | 20 個頭文件 API 參考 |
| `lib/README.md` | ~60 | 外部庫列表 |
| `test/README.md` | ~110 | 測試策略建議 |

---

## 🔍 功能差距分析

### 1. ⚠️ 高優先級：用戶可見但未充分記錄的功能

#### 1.1 多幣種顯示系統（9種貨幣）

**狀態:** ⚠️ 僅簡單提及，缺乏詳細說明

**代碼實現位置:**
- `config.h:80-114` - 貨幣枚舉和元數據
- `network.cpp` - `fetchExchangeRates()` 函數

**已實現功能:**
```cpp
enum DisplayCurrency : uint8_t {
  CURR_USD = 0,  // US Dollar ($)
  CURR_TWD = 1,  // Taiwan Dollar (NT)
  CURR_EUR = 2,  // Euro (€)
  CURR_GBP = 3,  // British Pound (£)
  CURR_CAD = 4,  // Canadian Dollar (C$)
  CURR_JPY = 5,  // Japanese Yen (¥)
  CURR_KRW = 6,  // Korean Won (₩)
  CURR_SGD = 7,  // Singapore Dollar (S$)
  CURR_AUD = 8,  // Australian Dollar (A$)
};
```

**特殊處理:**
- **長度限制顯示:** 所有貨幣使用統一的長度限制算法（最多 10 字符）
- **自動調整小數位:** 4 位 → 2 位 → 0 位（根據總長度自動調整）
- **V0.99p 改進:** 移除了 JPY/KRW 強制整數限制，現在也可顯示小數（基於長度限制）
- **2字符符號:** NT, C$, S$, A$, EUR, GBP, JPY, KRW 使用壓縮字體
- **匯率來源:** ExchangeRate-API.com
- **更新頻率:** 與價格更新同步

**文檔中的缺失:**
- 支持的 9 種貨幣完整列表
- V0.99p 長度限制算法說明（所有貨幣統一邏輯）
- 匯率更新機制和來源
- 貨幣符號的特殊渲染邏輯

**建議:** 在 DISPLAY_GUIDE.md 中添加 "Multi-Currency Display" 章節

---

#### 1.2 Day Average Mode（日均價參考線）

**狀態:** ⚠️ 僅列出 3 種模式名稱，無工作原理說明

**代碼實現位置:**
- `day_avg.cpp` - 滾動平均和週期平均計算
- `chart.h` - 7pm ET cycle 管理

**已實現的 3 種模式:**

**Mode 0: Off**
- 無參考線

**Mode 1: Rolling 24h Mean（預設）**
```cpp
// 實現細節：
// - 288 個 5 分鐘桶（24h × 60min ÷ 5min）
// - 滾動窗口：始終顯示過去 24 小時的平均價格
// - 函數：dayAvgRollingAdd(), dayAvgRollingGet()
```

**Mode 2: ET 7pm Cycle Mean**
```cpp
// 實現細節：
// - 每天東部時間晚上 7 點重置
// - 計算從 7pm ET 到當前時間的平均價格
// - 與圖表週期同步（g_chartSamples[]）
// - 函數：dayAvgCycleMean()
```

**為什麼選擇 7pm ET?**
- 美國股市閉市後（4pm ET）
- 加密貨幣市場 24/7 運作，但許多交易者以美東時間為基準
- 提供一致的每日基準點

**文檔中的缺失:**
- Rolling 24h mean 的 5 分鐘桶機制
- ET 7pm cycle 的重置時間和原因
- 如何解讀參考線（高於/低於的含義）
- 兩種模式的適用場景

**建議:** 在 DISPLAY_GUIDE.md 中擴展 "Reference Line" 章節

---

#### 1.3 獨立時間刷新機制（V0.99q 新功能）

**狀態:** ⚠️ 完全未記錄

**代碼實現位置:**
- `app_state.h:224-227` - 時間刷新狀態變量
- `app_scheduler.cpp` - 獨立時間刷新調度

**功能描述:**
```cpp
// 時間顯示每分鐘獨立刷新，不受價格更新間隔影響
extern time_t g_nextTimeRefreshUtc;    // 下次時間刷新時刻
extern bool   g_timeRefreshEnabled;     // 啟用/禁用獨立時間刷新（預設：啟用）

// 工作原理：
// - 價格更新間隔：1/3/5/10 分鐘（用戶可選）
// - 時間刷新間隔：固定每 1 分鐘
// - 即使價格更新間隔設為 10 分鐘，時間仍每分鐘更新
```

**用戶體驗提升:**
- 時間始終準確（不會因價格更新間隔長而延遲）
- 獨立於價格更新的時間顯示刷新
- 不影響價格更新頻率

**文檔中的缺失:**
- 完全未提及此功能
- 用戶可能誤以為時間更新頻率受價格更新間隔影響

**建議:** 在 DISPLAY_GUIDE.md "Date & Time Display" 章節中說明

---

#### 1.4 更新間隔預設值修正

**狀態:** ⚠️ 文檔中有誤導

**代碼實際實現:**
```cpp
// app_state.cpp:25-30
const uint32_t UPDATE_PRESETS_MS[] = {
  60UL * 1000UL,   // 1min
  180UL * 1000UL,  // 3min (default, recommended)
  300UL * 1000UL,  // 5min
  600UL * 1000UL   // 10min
};
```

**DISPLAY_GUIDE.md 中的說法:**
- "Price refresh interval (1/3/5/10 min)" ✅ 正確

**但缺少的信息:**
- **預設值:** 3 分鐘（不是 1 分鐘）
- **推薦值:** 3 分鐘（平衡 API 限制和及時性）
- **API 更新頻率:** CoinPaprika 每 30 秒更新一次（但用戶刷新間隔建議 ≥ 1 分鐘）

**建議:** 明確標註預設值和推薦值

---

#### 1.5 維護模式啟動方式（Boot-time Trigger）

**狀態:** ⚠️ 未記錄開機進入維護模式的方法

**代碼實現位置:**
- `maint_boot.cpp:7-11` - 開機檢測邏輯

**功能描述:**
```cpp
// 開機時長按編碼器按鈕可直接進入維護模式
bool maintBootShouldEnter() {
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  delay(50);  // 消抖
  return (digitalRead(ENC_SW_PIN) == LOW);
}
```

**用戶操作:**
1. 斷電
2. **按住編碼器按鈕**
3. 插入電源
4. 保持按住約 1-2 秒
5. 設備直接進入維護模式（跳過正常啟動）

**用途:**
- 無需通過菜單即可進入維護模式
- 固件損壞時的緊急恢復方法
- 快速 OTA 更新入口

**文檔中的缺失:**
- OTA_UPDATE_GUIDE.md 只提到從菜單進入
- 未說明開機長按的快捷方式

**建議:** 在 OTA_UPDATE_GUIDE.md 添加 "Quick Access" 章節

---

### 2. 🔧 中優先級：內部機制和高級功能

#### 2.1 Prefetch 調度機制（預取優化）

**狀態:** ⚠️ 完全未記錄

**代碼實現位置:**
- `app_scheduler.cpp` - 預取窗口計算
- `app_state.h:157-166` - 預取狀態變量

**機制說明:**
```cpp
// 預取窗口配置
extern const uint32_t PREFETCH_WINDOW_SEC;      // 30 秒窗口
extern const uint32_t PREFETCH_MIN_LEAD_SEC;    // 最小提前 5 秒
extern const uint32_t PREFETCH_FIXED_LEAD_SEC;  // 固定提前 10 秒

// 預取狀態
extern uint32_t g_fetchJitterSec;      // 隨機抖動（5-10 秒）
extern bool     g_prefetchValid;       // 預取數據有效標記
extern time_t   g_prefetchForUtc;      // 預取目標時刻
extern double   g_prefetchPrice;       // 預取的價格
extern double   g_prefetchChange;      // 預取的變化百分比
```

**工作流程:**
1. **計算下次更新時刻:** 例如 10:00:00
2. **生成隨機抖動:** 5-10 秒（避免所有設備同時請求 API）
3. **預取時間:** 10:00:00 - 7 秒 = 9:59:53
4. **在 9:59:53 獲取價格** → 緩存
5. **在 10:00:00 顯示更新** → 使用緩存的價格

**優勢:**
- 顯示更新更準時（網絡延遲已提前消化）
- 減少 API 速率限制風險（隨機抖動）
- 用戶感知的響應速度更快

**文檔中的缺失:**
- 完全未提及預取機制
- 用戶可能不理解為什麼更新如此準時

**建議:** 開發者文檔專題（技術細節，非用戶指南）

---

#### 2.2 Runtime WiFi 重連機制

**狀態:** ⚠️ 未記錄自動重連邏輯

**代碼實現位置:**
- `app_wifi.cpp` - 重連邏輯
- `app_state.h:195-201` - 重連狀態變量

**機制說明:**
```cpp
// 重連參數
extern const uint8_t  RUNTIME_RECONNECT_ATTEMPTS;     // 每批 3 次嘗試
extern const uint32_t RUNTIME_RECONNECT_TIMEOUT_MS;   // 每次嘗試 30 秒超時
extern const uint32_t RUNTIME_RECONNECT_BACKOFF_MS;   // 批次間隔 5 分鐘退避

// 重連狀態
extern bool     g_wifiEverConnected;        // 是否曾經連接過（避免過度重試）
extern uint32_t g_nextRuntimeReconnectMs;   // 下次重連時間
extern uint8_t  g_runtimeReconnectBatch;    // 當前重連批次計數
```

**重連策略:**
1. **檢測到斷線** → 等待 5 分鐘（避免頻繁重試）
2. **第一批重連:** 3 次嘗試，每次 30 秒超時
3. **失敗 → 等待 5 分鐘**
4. **第二批重連:** 3 次嘗試
5. **持續重試，指數退避**

**保護機制:**
- 如果從未連接過（首次配置失敗），不自動重連
- 避免電池設備無限重連耗電

**文檔中的缺失:**
- 斷線後的行為未說明
- 用戶可能不知道設備會自動重連

**建議:** 在 DISPLAY_GUIDE.md 添加 "WiFi Connection Management" 章節

---

#### 2.3 Partial/Full Refresh 策略

**狀態:** ⚠️ 只提到模式選擇，未說明自動 Full Refresh

**代碼實現位置:**
- `app_state.h:86-88` - Partial refresh 計數器
- `ui.cpp` - 刷新邏輯

**機制說明:**
```cpp
extern uint16_t g_partialRefreshCount;           // Partial refresh 計數器
extern const uint16_t PARTIAL_REFRESH_LIMIT;     // 20 次後自動 Full Refresh
```

**自動 Full Refresh 規則:**
- **Partial 模式下:** 每 20 次 partial refresh 後自動執行 1 次 full refresh
- **目的:** 清除累積的殘影（ghosting）
- **Full 模式下:** 每次都執行 full refresh（無計數器）

**Partial vs Full 比較:**
| 特性 | Partial Refresh | Full Refresh |
|------|-----------------|--------------|
| 速度 | ~1 秒 | ~2-3 秒 |
| 殘影 | 可能累積 | 完全清除 |
| 閃爍 | 無 | 黑白閃爍 |
| 電量消耗 | 低 | 高 |
| 推薦場景 | 頻繁更新 | 長時間靜態顯示 |

**文檔中的缺失:**
- 自動 Full Refresh 的 20 次規則
- Partial/Full 的詳細比較
- 何時選擇哪種模式

**建議:** 在 DISPLAY_GUIDE.md "Refresh Mode" 章節中詳細說明

---

#### 2.4 API Fallback 詳細順序

**狀態:** ⚠️ 僅提到 "fallback"，未說明具體順序

**代碼實現位置:**
- `network.cpp:fetchPrice()` - 4層價格 fallback
- `network.cpp:bootstrapHistoryFromKrakenOHLC()` - 3層歷史數據 fallback

**價格 API Fallback（4層）:**
```cpp
1. Binance WebSocket API (BTCUSDT pair)
   ↓ 失敗
2. Kraken REST API (XXBTZUSD pair, if configured)
   ↓ 失敗
3. CoinPaprika API (aggregated data, 30s refresh)
   ↓ 失敗
4. CoinGecko API (backup source)
   ↓ 全部失敗 → 顯示 "----" + LED 黃燈
```

**歷史數據 API Fallback（3層）:**
```cpp
1. Kraken OHLC API (5min interval, if krakenPair configured)
   ↓ 失敗
2. Binance Klines API (5min candles)
   ↓ 失敗
3. CoinGecko market_chart API (days=1)
   ↓ 全部失敗 → 圖表顯示 "Collecting data..."
```

**Fallback 觸發條件:**
- HTTP 錯誤（4xx/5xx）
- 超時（10 秒）
- JSON 解析失敗
- 返回數據無效

**文檔中的缺失:**
- 具體的 API 順序
- 為什麼選擇這個順序（可靠性、速度、限制）
- Fallback 的觸發條件

**建議:** 在 DISPLAY_GUIDE.md "API Labels" 章節中詳細說明

---

#### 2.5 NTP 重新同步機制

**狀態:** ⚠️ 未記錄定期同步

**代碼實現位置:**
- `app_time.cpp` - NTP 同步邏輯
- `app_state.h:217-222` - NTP 狀態變量

**機制說明:**
```cpp
extern const uint32_t NTP_RESYNC_INTERVAL_SEC;  // 10 分鐘（600 秒）
extern time_t g_nextNtpResyncUtc;               // 下次 NTP 同步時刻
```

**NTP 同步策略:**
1. **首次啟動:** WiFi 連接後立即同步
2. **定期同步:** 每 10 分鐘自動重新同步
3. **NTP 服務器:**
   - Primary: `pool.ntp.org`
   - Secondary: `time.nist.gov`

**時間精度:**
- ESP32 內部 RTC 漂移：~1-5 秒/天
- 10 分鐘同步間隔確保時間始終精確（誤差 < 0.1 秒）

**文檔中的缺失:**
- 定期 NTP 同步機制
- 時間精度保證
- NTP 服務器來源

**建議:** 在 DISPLAY_GUIDE.md 添加 "Time Accuracy" 說明

---

#### 2.6 Timezone 自動檢測

**狀態:** ⚠️ 未記錄首次啟動自動檢測

**代碼實現位置:**
- `app_time.cpp:appTimeAutoDetectTimezone()` - 自動檢測邏輯
- `settings_store.cpp:settingsStoreHasTzIndex()` - 檢查是否已設置時區

**機制說明:**
```cpp
// 自動檢測觸發條件：
// 1. 首次啟動（NVS 中無 tzIndex 鍵）
// 2. WiFi 連接成功
// 3. 一次性嘗試（成功或失敗後不再自動檢測）

// 檢測方法：
// - 使用 worldtimeapi.org IP-based timezone API
// - 返回當前 IP 的 UTC offset（整數小時）
// - 匹配到最接近的時區 index
// - 保存到 NVS（之後用戶可手動修改）
```

**Fallback 行為:**
- **自動檢測失敗** → 使用預設時區（UTC-08 Seattle）
- **VPN 用戶:** 檢測到的是 VPN 出口位置的時區
- **無網絡:** 保持預設時區

**文檔中的缺失:**
- 首次啟動自動檢測時區
- VPN 用戶的注意事項
- 為什麼預設是 Seattle

**建議:** 在 WiFi Portal 說明中添加時區自動檢測提示

---

### 3. 🛠️ 低優先級：開發者專用技術細節

#### 3.1 編碼器配置參數

**狀態:** ✅ 已在 `src/README.md` 和 `include/README.md` 中詳細記錄

**可調參數:**
```cpp
// encoder_pcnt.cpp 中可調整：
#define ENC_PCNT_FILTER_VAL 150        // 噪音過濾（100-300）
#define ENC_COUNTS_PER_DETENT 6        // 靈敏度（3/6/12）
#define ENC_DIR_INVERT 0               // 方向反轉（0/1）
#define ENC_DIR_LOCK_MS 10             // 方向鎖定時間（0-50ms）
#define ENC_DEBUG 2                    // 調試級別（0/1/2）
```

**文檔狀態:** ✅ 已充分記錄在開發者文檔中

---

#### 3.2 LED 動畫任務

**狀態:** ✅ 已在 `src/README.md` 和 `include/README.md` 中記錄

**功能描述:**
```cpp
// 可選：獨立 LED 動畫任務（20-30 Hz）
void ledAnimStartTask(uint16_t hz = 30, uint8_t core = 0);

// 優勢：
// - LED 在 e-paper 刷新期間仍能呼吸（主線程被阻塞）
// - 更流暢的動畫效果
// - 可指定運行在哪個 CPU 核心
```

**文檔狀態:** ✅ 已在開發者文檔中記錄

---

## 📝 建議的文檔改進優先級

### 🔴 高優先級（用戶直接可見）

1. **多幣種顯示系統** - 在 DISPLAY_GUIDE.md 添加專門章節
   - 9 種貨幣完整列表
   - V0.99p 長度限制算法說明（統一邏輯，所有貨幣都可顯示小數）
   - 匯率更新機制

2. **Day Average Mode 詳細說明** - 擴展 DISPLAY_GUIDE.md "Reference Line" 章節
   - Rolling 24h mean 原理（5分鐘桶）
   - ET 7pm cycle mean 原理和重置時間
   - 如何解讀參考線

3. **獨立時間刷新** - 在 DISPLAY_GUIDE.md "Date & Time Display" 章節添加
   - 時間每分鐘獨立刷新
   - 不受價格更新間隔影響

4. **維護模式快速進入** - 在 OTA_UPDATE_GUIDE.md 添加 "Quick Access" 章節
   - 開機長按編碼器按鈕
   - 緊急恢復方法

5. **更新間隔預設值** - 在 DISPLAY_GUIDE.md 明確標註
   - 預設：3 分鐘
   - 推薦：3 分鐘（平衡 API 限制和及時性）

### 🟡 中優先級（影響用戶體驗）

6. **Runtime WiFi 重連** - 在 DISPLAY_GUIDE.md 添加 "WiFi Connection Management"
   - 自動重連策略（3 次嘗試，5 分鐘退避）
   - 斷線後的行為

7. **Partial/Full Refresh 詳細說明** - 擴展 DISPLAY_GUIDE.md "Refresh Mode"
   - 20 次 Partial 後自動 Full Refresh
   - 兩種模式的詳細比較表
   - 推薦使用場景

8. **API Fallback 詳細順序** - 擴展 DISPLAY_GUIDE.md "API Labels"
   - 4 層價格 fallback 順序
   - 3 層歷史數據 fallback 順序
   - Fallback 觸發條件

9. **NTP 同步機制** - 在 DISPLAY_GUIDE.md 添加 "Time Accuracy"
   - 每 10 分鐘自動同步
   - 時間精度保證（< 0.1 秒）

10. **Timezone 自動檢測** - 在 WiFi Portal 說明中添加
    - 首次啟動自動檢測
    - VPN 用戶注意事項

### 🟢 低優先級（開發者專用）

11. **Prefetch 調度機制** - 可選：創建 `DEVELOPER_GUIDE.md`
    - 預取窗口和抖動機制
    - 技術實現細節

12. **編碼器/LED 高級配置** - ✅ 已在開發者文檔中充分記錄

---

## 🎯 推薦行動計劃

### Phase 1: 立即更新（高優先級項目 1-5）

**預估工作量:** 2-3 小時

1. 編輯 `DISPLAY_GUIDE.md`:
   - 添加 "Multi-Currency Display" 章節（~50 行）
   - 擴展 "Reference Line" 章節（~80 行）
   - 更新 "Date & Time Display" 添加獨立刷新說明（~20 行）
   - 更新菜單項說明，標註預設值（~10 行）

2. 編輯 `OTA_UPDATE_GUIDE.md`:
   - 添加 "Quick Access: Boot-time Entry" 章節（~30 行）

**預期成果:** 用戶指南完整性提升 80%

### Phase 2: 完善補充（中優先級項目 6-10）

**預估工作量:** 2-3 小時

1. 繼續編輯 `DISPLAY_GUIDE.md`:
   - 添加 "WiFi Connection Management" 章節（~40 行）
   - 擴展 "Refresh Mode" 詳細說明（~60 行）
   - 擴展 "API Labels" 說明 fallback 順序（~50 行）
   - 添加 "Time Accuracy & NTP Sync" 章節（~30 行）
   - 添加 "Timezone Auto-Detection" 說明（~20 行）

**預期成果:** 用戶指南完整性提升至 95%

### Phase 3: 可選高級內容（低優先級）

**預估工作量:** 3-4 小時

1. 創建 `DEVELOPER_GUIDE.md`（可選）:
   - Prefetch 調度機制詳解
   - 架構設計決策說明
   - 高級調試技巧

**預期成果:** 開發者文檔完整性 100%

---

## 📊 文檔完整性評分

| 分類 | 當前評分 | Phase 1 後 | Phase 2 後 |
|------|----------|------------|------------|
| **用戶指南** | 65% | 85% | 95% |
| **開發者文檔** | 90% | 90% | 100% |
| **整體完整性** | 75% | 87% | 97% |

---

## 📌 總結

**已完成:**
- ✅ 開發者文檔（src/include/lib/test README）已全面完善
- ✅ OTA 更新指南已補充 A/B 分區回滾機制

**需要補充:**
- ⚠️ 用戶指南中的 10 個高/中優先級功能說明
- ⚠️ 多幣種、Day Average、獨立時間刷新等關鍵功能缺乏詳細解釋

**推薦做法:**
1. **立即執行 Phase 1**（2-3 小時）→ 完整性提升至 85%
2. **有時間再執行 Phase 2**（2-3 小時）→ 完整性提升至 95%
3. **Phase 3 可選**（開發者高級內容）

---

**報告結束**
