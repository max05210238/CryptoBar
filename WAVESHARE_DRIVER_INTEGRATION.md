# WaveShare 2.9" Module (G) 官方驅動整合

## 📋 整合完成！

已成功將 **WaveShare 官方 EPD 2.9G 驅動** 整合到 CryptoBar 項目！

---

## ✅ 完成的工作

### 1. 下載官方驅動
從 WaveShare 官方 GitHub 下載了完整的 ESP32 驅動代碼：
```
lib/EPD_2in9g/
├── DEV_Config.h          - 硬件配置（已修改引腳）
├── DEV_Config.cpp        - 硬件接口實現
├── EPD_2in9g.h          - 顯示驅動頭文件
├── EPD_2in9g.cpp        - 顯示驅動實現
├── GUI_Paint.h          - 繪圖庫頭文件
├── GUI_Paint.cpp        - 繪圖庫實現
├── Debug.h              - 調試宏定義
└── EPD_GxEPD2_Compat.h  - GxEPD2 兼容包裝類（新建）
```

### 2. 修改引腳配置
**修改了 `lib/EPD_2in9g/DEV_Config.h` 和 `DEV_Config.cpp`**

| 信號 | WaveShare 默認 | CryptoBar 配置 |
|------|---------------|----------------|
| SCK  | 13            | 12             |
| MOSI | 14            | 11             |
| CS   | 15            | 10             |
| RST  | 26            | 16             |
| DC   | 27            | 17             |
| BUSY | 25            | 4              |
| PWR  | 33            | (未使用)        |

✅ 已註釋掉所有 `EPD_PWR_PIN` 相關代碼（CryptoBar 不使用外部電源控制）

### 3. 創建 GxEPD2 兼容層
**創建了 `lib/EPD_2in9g/EPD_GxEPD2_Compat.h`**

這個包裝類提供了 GxEPD2 API 接口，使得現有代碼無需大量修改：

```cpp
class EPD_GxEPD2_Compat : public Adafruit_GFX {
    // 實現 GxEPD2 常用 API:
    - init()
    - firstPage() / nextPage()
    - display()
    - clearScreen()
    - hibernate()
    - fillRect(), drawLine(), drawRect()
    - width(), height()
};
```

### 4. 修改源代碼
修改了以下文件來使用新驅動：

**主程序**:
- `src/main.cpp` - 替換 display 對象聲明
- `include/app_state.h` - 更新 display extern 聲明

**UI 文件**（批量更新）:
- `src/ui.cpp`
- `src/ui_coin.cpp`
- `src/ui_currency.cpp`
- `src/ui_list.cpp`
- `src/ui_menu.cpp`
- `src/ui_timezone.cpp`
- `src/ui_update.cpp`

所有文件的更改：
```cpp
// 舊代碼：
#include <GxEPD2_3C.h>
extern GxEPD2_3C<GxEPD2_290c, GxEPD2_290c::HEIGHT> display;

// 新代碼：
#include "../lib/EPD_2in9g/EPD_GxEPD2_Compat.h"
extern EPD_GxEPD2_Compat display;
```

---

## 🔧 技術細節

### 顯示規格
- **解析度**: 128×296 像素
- **顏色**: 4色（黑/白/黃/紅）
- **像素格式**: 2 bits per pixel
- **緩衝區大小**: 9,472 bytes (32 × 296)

### 顏色定義
```cpp
#define EPD_2IN9G_BLACK   0x0   // GxEPD_BLACK
#define EPD_2IN9G_WHITE   0x1   // GxEPD_WHITE
#define EPD_2IN9G_YELLOW  0x2   // GxEPD_YELLOW
#define EPD_2IN9G_RED     0x3   // GxEPD_RED
```

### API 兼容性
包裝類繼承自 `Adafruit_GFX`，因此支持：
- ✅ `setFont()` - 字體設置
- ✅ `setTextColor()` - 文字顏色
- ✅ `setCursor()` - 光標位置
- ✅ `print()` - 文字輸出
- ✅ `getTextBounds()` - 文字邊界計算
- ✅ `fillRect()` - 填充矩形
- ✅ `drawLine()` - 繪製直線
- ✅ `drawRect()` - 繪製矩形框

### 刷新模式
- ❌ **不支持局部刷新**（4色屏幕硬件限制）
- ✅ 僅支持全屏刷新
- ⏱️ 刷新時間：約 15-20 秒

---

## 📦 編譯說明

### 依賴項
保持現有依賴（`platformio.ini`）：
```ini
lib_deps =
  bblanchon/ArduinoJson @ ^7.0.0
  adafruit/Adafruit GFX Library @ ^1.11.9  # 需要
  adafruit/Adafruit BusIO @ ^1.15.0
  adafruit/Adafruit NeoPixel @ ^1.12.0
  # GxEPD2 不再需要（可選移除）
```

### 編譯命令
```bash
cd ~/CryptoBar
git pull origin claude/4color-display-V099r-G-K0kQP

# 清理並重新編譯
pio run -t clean
pio run

# 上傳到 ESP32
pio run --target upload

# 查看串口輸出
pio device monitor -b 115200
```

---

## 🎯 測試

### 預期結果
1. **初始化**：顯示 "Initializing GPIOs..." 和 "EPD Init OK"
2. **清屏**：屏幕會變白（約 15 秒）
3. **顯示內容**：正常顯示 CryptoBar 界面
4. **無 Busy Timeout**：不再出現超時錯誤

### 測試程序
如果需要獨立測試驅動，使用：
```bash
# 運行最小測試（已創建但未啟用）
# src/main_epd_test.cpp
```

---

## 📝 已知限制

1. **刷新速度慢**：每次刷新 15-20 秒（4色屏幕硬件限制）
2. **無局部刷新**：每次更新都是全屏刷新
3. **暫時黑白顯示**：雖然硬件支持 4色，但當前代碼使用黑白
   - 要使用顏色，需要在 UI 代碼中使用 `GxEPD_YELLOW` 和 `GxEPD_RED`

---

## 🚀 後續增強（可選）

### 添加 4色支持
在 UI 代碼中使用彩色：

```cpp
// 例如：價格上漲用紅色，下跌用黃色
display.setTextColor(g_lastChange24h >= 0 ? GxEPD_RED : GxEPD_YELLOW);
display.print(priceText);
```

### 優化刷新策略
- 延長更新間隔（建議 ≥ 5分鐘）
- 在設置中添加"僅手動刷新"選項

---

## 📚 參考資料

- [WaveShare 2.9" Module (G) Wiki](https://www.waveshare.com/wiki/2.9inch_e-Paper_Module_(G))
- [WaveShare GitHub - 官方驅動](https://github.com/waveshareteam/e-Paper/tree/master/E-paper_Separate_Program/2.9inch_e-Paper_G/ESP32)
- [Good Display GDEY029F51 (EOL)](https://www.good-display.com/product/468.html)

---

## 🔧 重要 Bug 修復 (2026-01-11)

### 問題：部分顯示渲染 Bug
- **症狀**: 顯示器只在左側 ~40% 區域渲染，右側顯示雜訊
- **根本原因**: Paint 庫使用 Scale=2 (黑白模式) 而不是 Scale=4 (4色模式)
- **影響**: WidthByte 計算錯誤 (37 bytes 而不是 32 bytes)，導致緩衝區地址錯誤

### 解決方案
在 `EPD_GxEPD2_Compat.h` 的 `init()` 函數中：
1. 修正參數順序：使用物理尺寸 `(WIDTH, HEIGHT)` 而不是 `(HEIGHT, WIDTH)`
2. 添加 `Paint_SetScale(4)` 啟用 4色模式
3. 修正旋轉角度：從 270° 改為 90° (解決畫面倒轉問題)

詳細說明請參閱：`FIX_PARTIAL_DISPLAY_BUG.md`

---

## 🎉 實機測試成功 (2026-01-11)

### 測試結果
- ✅ **完整螢幕渲染** - 整個 296×128 區域都正確顯示
- ✅ **4 色顯示正常** - 黑/白/紅/黃四色清晰可見
- ✅ **正向顯示** - 90° 旋轉角度正確
- ✅ **測試圖案移除** - 已恢復正常 UI 顯示
- ✅ **5 分鐘更新間隔** - 預設值已設定為 300 秒（整點對齊：0:00, 0:05, 0:10...）

### 生產配置
- **固件版本**: V0.99r-G
- **更新間隔**: 5 分鐘（考慮 4 色螢幕刷新慢，會閃爍 15-20 秒）
- **旋轉角度**: 90° (landscape 296×128)
- **顏色模式**: Scale=4 (2 bits per pixel, 4-color)

---

**最後更新**: 2026-01-11
**分支**: `claude/4color-display-V099r-G-K0kQP`
**狀態**: ✅ 整合完成 | ✅ 實機測試成功 | 🚀 準備生產使用
