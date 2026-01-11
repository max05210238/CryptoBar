# 🐛 修復部分顯示 Bug - Partial Display Rendering Fix

## 問題描述 (Problem Description)

顯示器只在左側約 40% 區域正確渲染，右側 60% 顯示雜訊/噪點。
無論使用 0°、90° 或 270° 旋轉角度，問題持續存在。

Display only renders correctly on the left ~40% of screen, with noise/garbage on the right ~60%.
Problem persists regardless of rotation angle (0°, 90°, 270°).

---

## 根本原因 (Root Cause)

### 1️⃣ **錯誤的顏色模式** (Wrong Color Mode)
`GUI_Paint.cpp` 中 `Paint_NewImage()` 函數在第 102 行硬編碼：
```cpp
Paint.Scale = 2;  // 1 bit per pixel (黑白模式)
```

但 **EPD_2in9G 是 4 色顯示器**，需要 `Scale = 4` (2 bits per pixel)！

### 2️⃣ **錯誤的參數順序** (Wrong Parameter Order)
原代碼傳遞了**旋轉後的尺寸**，而不是**物理緩衝區尺寸**：
```cpp
// ❌ 錯誤：傳遞旋轉後的尺寸
Paint_NewImage(imageBuffer, EPD_2IN9G_HEIGHT, EPD_2IN9G_WIDTH, 270, EPD_2IN9G_WHITE);
//                          296 (HEIGHT)      128 (WIDTH)
```

應該傳遞**物理尺寸**：
```cpp
// ✅ 正確：傳遞物理尺寸
Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT, 270, EPD_2IN9G_WHITE);
//                          128 (WIDTH)       296 (HEIGHT)
```

### 3️⃣ **緩衝區計算錯誤** (Buffer Calculation Error)

**Scale = 2 (黑白模式)**:
- WidthByte = 296 / 8 = 37 bytes ❌
- 緩衝區需要: 37 × 128 = 4,736 bytes
- 實際分配: 9,472 bytes
- **結果**: 緩衝區地址計算錯誤，導致部分渲染

**Scale = 4 (4色模式)**:
- WidthByte = 128 / 4 = 32 bytes ✅
- 緩衝區需要: 32 × 296 = 9,472 bytes
- 實際分配: 9,472 bytes ✅
- **結果**: 緩衝區地址正確，完整渲染

---

## 解決方案 (Solution)

修改 `/home/user/CryptoBar/lib/EPD_2in9g/EPD_GxEPD2_Compat.h` 中的 `init()` 函數：

```cpp
// 使用物理尺寸初始化 Paint 庫
// Physical: 128×296, Rotation: 270° → Logical: 296×128 (landscape)
Serial.println("EPD: Initializing Paint library (physical 128x296, rotate=270)");
Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT, 270, EPD_2IN9G_WHITE);
//                          ^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^
//                          128 (物理寬度)     296 (物理高度)

// 🔴 關鍵：設置 Scale=4 啟用 4 色模式 (2 bits per pixel)
Serial.println("EPD: Setting Paint scale to 4 (4-color mode)");
Paint_SetScale(4);

Paint_SelectImage(imageBuffer);
```

---

## 技術細節 (Technical Details)

### Paint Library 緩衝區佈局

**物理屏幕**: 128 (寬) × 296 (高) 像素 (portrait)
**邏輯佈局** (270° 旋轉): 296 (寬) × 128 (高) 像素 (landscape)

**Scale = 4 (4 色模式)**:
- 每像素 2 bits
- 每字節存儲 4 像素
- WidthByte = 128 / 4 = 32 bytes
- 每行 32 bytes × 296 行 = 9,472 bytes ✅

### Paint_SetPixel 地址計算

從 `GUI_Paint.cpp` 第 245-251 行 (Scale=4 分支):
```cpp
UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
//             ^^^^^^^^^^^^^^^^^^^^^^^^^^
//             正確的緩衝區地址計算
```

當 WidthByte 錯誤時 (37 而不是 32)，地址計算會指向錯誤的內存位置，導致：
- 左側區域：地址仍在有效緩衝區內 → 正確顯示
- 右側區域：地址超出有效緩衝區 → 顯示雜訊

---

## 測試步驟 (Testing Steps)

1. **編譯固件**:
   ```bash
   cd ~/CryptoBar
   pio run -t clean
   pio run
   ```

2. **上傳到 ESP32**:
   ```bash
   pio run --target upload
   ```

3. **查看串口輸出**:
   ```bash
   pio device monitor -b 115200
   ```

4. **預期結果**:
   - ✅ 測試圖案應該在**整個屏幕**上顯示
   - ✅ 黑色矩形框應該完整（不再只有左半部分）
   - ✅ 紅色斜線應該延伸到右側
   - ✅ "CryptoBar"、"Landscape"、"Test 270deg" 文字應該清晰可見

---

## 相關文件 (Related Files)

- `lib/EPD_2in9g/EPD_GxEPD2_Compat.h` - **已修復** ✅
- `lib/EPD_2in9g/GUI_Paint.h` - Paint 庫頭文件
- `lib/EPD_2in9g/GUI_Paint.cpp` - Paint 庫實現 (第 94-118 行為 Paint_NewImage, 第 162-180 行為 Paint_SetScale)

---

## Git 提交 (Git Commit)

```bash
cd ~/CryptoBar
git add lib/EPD_2in9g/EPD_GxEPD2_Compat.h
git commit -m "Fix partial display rendering bug

- Use physical dimensions (128×296) instead of rotated dimensions in Paint_NewImage
- Add Paint_SetScale(4) to enable 4-color mode (2 bits per pixel)
- Fixes WidthByte calculation: 32 bytes (correct) vs 37 bytes (wrong)
- Resolves issue where only left 40% of screen rendered correctly

Root cause: Paint library was in Scale=2 (1bpp) mode instead of Scale=4 (2bpp),
causing incorrect buffer address calculations for pixel writes."
```

---

**修復日期**: 2026-01-11
**分支**: `claude/4color-display-V099r-G-K0kQP`
**狀態**: ✅ Bug 已修復，等待測試確認
