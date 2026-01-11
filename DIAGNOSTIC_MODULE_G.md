# WaveShare 2.9" Module (G) 驅動診斷報告

## 🔍 問題分析

### 你的硬件
- **產品**: WaveShare 2.9inch e-Paper Module (G)
- **解析度**: 296×128 像素
- **顏色**: 4色（紅/黃/黑/白）
- **面板型號**: GDEY029F51 (EOL) 或類似
- **控制器**: JD79661 或 UC8151/IL0373 系列

### GxEPD2 庫支持情況

❌ **問題發現**：GxEPD2 庫 **沒有** 直接支持你的這個屏幕！

| 驅動類別 | 型號 | 解析度 | 顏色 | 狀態 |
|---------|------|--------|------|------|
| **GxEPD2_290c_GDEY029F51H** | GDEY029F51H | 168×384 | 4色 | ❌ 解析度不匹配 |
| **GxEPD2_290c** | GDEW029Z10 | 128×296 | 3色 | ⚠️ 嘗試中 |
| **GxEPD2_290_Z13c** | GDEH029Z13 | 128×296 | 3色 | 🔄 備選方案 |

---

## ✅ 當前嘗試：使用 3色驅動

因為找不到完全匹配的 4色驅動，我嘗試使用解析度匹配的 3色驅動：

### 最新提交（6b96e37）
```
使用 GxEPD2_3C<GxEPD2_290c> 驅動（GDEW029Z10）
- 解析度：128×296（與你的 296×128 相同，只是旋轉方向）
- 控制器：UC8151/IL0373（可能與你的屏幕兼容）
- 顏色：黑/白/紅（3色）
```

### 測試步驟

```bash
cd ~/CryptoBar
git fetch origin
git pull origin claude/4color-display-V099r-G-K0kQP

# 清理並重新編譯
pio run -t clean
pio run

# 上傳到 ESP32
pio run --target upload

# 查看串口輸出
pio device monitor -b 115200
```

### 預期結果

#### 🎯 最佳情況（成功）
```
_InitDisplay reset : 34000
_PowerOn : 91999
_refresh : 15000
display.init() completed
```
- 屏幕會顯示畫面（即使可能只有黑白）
- 沒有 "Busy Timeout" 錯誤

#### ⚠️ 仍然失敗
```
Busy Timeout!
```
- 表示這個控制器也不兼容
- 需要嘗試其他驅動或方案

---

## 🔧 備選方案

如果當前驅動仍然 Timeout，我準備了以下備選方案：

### 方案 A: 嘗試 GxEPD2_290_Z13c
```cpp
// 使用另一個 3色驅動（GDEH029Z13，UC8151D 控制器）
GxEPD2_3C<GxEPD2_290_Z13c, GxEPD2_290_Z13c::HEIGHT> display(...)
```

### 方案 B: 嘗試 GxEPD2_290_C90c
```cpp
// 使用第三個 3色驅動（GDEM029C90，SSD1680 控制器）
GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> display(...)
```

### 方案 C: 使用 WaveShare 官方驅動
- 從 WaveShare GitHub 獲取 Module (G) 專用驅動代碼
- 移植到 CryptoBar 項目（需要大量修改）
- 網址：https://github.com/waveshare/e-Paper

---

## 📊 下一步行動

### 請你提供以下信息：

1. **編譯結果**：
   - 能成功編譯嗎？
   - 有任何錯誤訊息嗎？

2. **串口輸出**：
   - 還有 "Busy Timeout" 嗎？
   - 有任何新的錯誤訊息？

3. **屏幕狀態**：
   - 完全沒有畫面？
   - 有閃爍或部分畫面？
   - 有任何變化嗎？

4. **硬件確認**：
   - 屏幕背面標籤寫的型號是什麼？（拍照更好）
   - 確定是 WaveShare 2.9" Module **(G)** 版本，不是 (B) 或 V2？

---

## 🎓 技術背景

### 為什麼會出現這個問題？

1. **WaveShare 產品線複雜**：
   - 2.9" Module (G) 是 4色版本
   - 使用 GDEY029F51 面板（已停產）
   - Good Display 推薦升級到 GDEY029F51H（不同解析度！）

2. **GxEPD2 庫限制**：
   - 只支持 GDEY029F51H (168×384)
   - 沒有支持 GDEY029F51 (296×128)
   - 這兩個是不同的面板！

3. **解決方向**：
   - 嘗試用類似控制器的驅動初始化
   - 如果控制器兼容，即使顏色數量不同也能工作
   - 最壞情況：需要自定義驅動或用官方驅動

---

## 📚 參考資料

- [GxEPD2 GitHub](https://github.com/ZinggJM/GxEPD2)
- [WaveShare 2.9" Module (G) 產品頁](https://www.waveshare.com/2.9inch-e-paper-module-g.htm)
- [Good Display GDEY029F51 (EOL)](https://www.good-display.com/product/468.html)
- [WaveShare GitHub Repository](https://github.com/waveshare/e-Paper)

---

**最後更新**: 2026-01-11
**當前分支**: `claude/4color-display-V099r-G-K0kQP`
**最新提交**: `6b96e37` - 嘗試 3色驅動
**狀態**: 🔄 等待測試結果

---

## 💡 提示

如果這次嘗試仍然失敗，不用擔心！我們還有多個備選方案。關鍵是要：
1. 確認屏幕的確切型號
2. 系統性地測試不同驅動
3. 找到正確的控制器初始化序列

**請測試後回報結果，我會根據你的反饋調整策略！** 🚀
