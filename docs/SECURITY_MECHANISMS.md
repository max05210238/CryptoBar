# CryptoBar Release Security Mechanisms

**Understanding GPG Signing, SHA256 Checksums, and GitHub Actions CI/CD**

**Target Audience:** Repository owner preparing for V1.00 release
**Purpose:** Implement cryptographic verification and automated builds for secure firmware distribution

---

## 🎯 Why We Need This (For V1.00 Launch)

你說得對：**不會寫代碼的用戶只能信任你的更新**。這就是為什麼我們需要這些安全機制：

**目前的問題（V0.99q）：**
- ❌ 用戶無法驗證 .bin 文件是否真的來自你
- ❌ GitHub 帳戶被入侵 → 攻擊者可以發布惡意固件
- ❌ 中間人攻擊可以替換下載的文件
- ❌ 用戶無法確認固件沒有被篡改

**V1.00 的目標：**
- ✅ 加密簽章證明固件來自你本人
- ✅ 校驗和確保文件完整性
- ✅ 自動化構建提供透明度
- ✅ 即使不會寫代碼的用戶也能驗證真實性

---

## 📚 目錄

1. [GPG 數位簽章](#1-gpg-數位簽章) - 證明「這是我發布的」
2. [SHA256 校驗和](#2-sha256-校驗和) - 證明「文件沒有被修改」
3. [GitHub Actions CI/CD](#3-github-actions-cicd) - 證明「固件來自公開源碼」
4. [實施計劃](#4-實施計劃-for-v100) - 如何在 V1.00 部署

---

## 1. GPG 數位簽章

### 🤔 什麼是 GPG？

**GPG (GNU Privacy Guard)** 是一個加密軟體，用於**數位簽章**。

**類比理解：**
想像你寄出一封重要文件：
- 📄 **文件本身** = firmware.bin（固件）
- ✍️ **你的手寫簽名** = GPG 簽章（證明是你）
- 🔍 **收件人驗證簽名** = 用戶驗證 GPG 簽章

**數位簽章 vs. 手寫簽名：**
- 手寫簽名可以偽造 ❌
- GPG 簽章**數學上不可偽造** ✅（除非私鑰被盜）

---

### 🔐 GPG 工作原理

GPG 使用**非對稱加密**（Asymmetric Cryptography）：

```
你有兩把鑰匙：

┌────────────────────┐         ┌────────────────────┐
│   私鑰 (Private)   │         │   公鑰 (Public)    │
│                    │         │                    │
│  - 只有你擁有      │         │  - 公開給所有人    │
│  - 用來簽署文件    │         │  - 用來驗證簽章    │
│  - 絕對保密！      │         │  - 可以隨便分享    │
└────────────────────┘         └────────────────────┘
         │                              │
         ▼                              ▼
    簽署 .bin                    驗證簽章是否真實
```

**數學原理（簡化版）：**

1. **你用私鑰簽署：**
   ```
   簽章 = Encrypt(Hash(firmware.bin), 你的私鑰)
   ```
   - Hash = 文件的數學指紋（SHA256）
   - Encrypt = 用私鑰加密這個指紋

2. **用戶用公鑰驗證：**
   ```
   驗證 = Decrypt(簽章, 你的公鑰) == Hash(下載的 firmware.bin)
   ```
   - 用公鑰解密簽章，得到原始 Hash
   - 比較解密出的 Hash 和下載文件的 Hash
   - 如果相同 → 簽章有效（確實是你簽的）

**為什麼無法偽造？**
- 只有你的私鑰能創建正確的簽章
- 攻擊者沒有私鑰 → 無法創建有效簽章
- 如果文件被修改 → Hash 改變 → 簽章驗證失敗

---

### 🛠️ 如何實施 GPG 簽章

#### 步驟 1：生成 GPG 金鑰對

```bash
# 安裝 GPG（如果還沒有）
# macOS:
brew install gnupg

# Ubuntu/Debian:
sudo apt-get install gnupg

# 生成金鑰對
gpg --full-generate-key

# 選擇：
# - 金鑰類型：RSA and RSA
# - 金鑰長度：4096 bits（更安全）
# - 有效期：0（永不過期，或選擇 2 years）
# - 姓名：你的 GitHub 用戶名（max05210238）
# - 郵箱：你的 GitHub 郵箱
# - 註釋：CryptoBar Release Signing Key
```

**輸出示例：**
```
gpg: key 1234ABCD5678EFGH marked as ultimately trusted
public and secret key created and signed.

pub   rsa4096 2025-12-26 [SC]
      ABCD1234EFGH5678IJKL9012MNOP3456QRST7890
uid   max05210238 (CryptoBar Release Signing Key) <your@email.com>
sub   rsa4096 2025-12-26 [E]
```

**重要：記住這個金鑰 ID！** (例如：`1234ABCD5678EFGH`)

---

#### 步驟 2：導出公鑰（分享給用戶）

```bash
# 導出公鑰到文件
gpg --armor --export 1234ABCD5678EFGH > cryptobar-gpg-public.asc

# 查看公鑰內容
cat cryptobar-gpg-public.asc
```

**輸出（公鑰）：**
```
-----BEGIN PGP PUBLIC KEY BLOCK-----

mQINBGVjX... [很長的 Base64 字串]
...
-----END PGP PUBLIC KEY BLOCK-----
```

**這個文件要做什麼？**
1. 提交到 GitHub 倉庫：`gpg-keys/cryptobar-release-key.asc`
2. 在 README.md 中鏈接
3. 用戶下載後用來驗證簽章

---

#### 步驟 3：簽署固件發布

**每次發布 V1.00 / V1.01 / ... 時：**

```bash
# 假設你已經構建了固件
cd .pio/build/esp32-s3-devkitc-1/

# 簽署 firmware.bin
gpg --detach-sign --armor --local-user 1234ABCD5678EFGH firmware.bin

# 生成 firmware.bin.asc（簽章文件）
```

**生成的文件：**
- `firmware.bin` - 原始固件（800KB-1.2MB）
- `firmware.bin.asc` - GPG 簽章（~500 bytes）

**兩個文件都要上傳到 GitHub Release！**

---

#### 步驟 4：用戶如何驗證

**用戶下載後：**

```bash
# 1. 下載公鑰（只需要一次）
wget https://raw.githubusercontent.com/max05210238/CryptoBar/main/gpg-keys/cryptobar-release-key.asc

# 2. 導入公鑰
gpg --import cryptobar-release-key.asc

# 3. 下載固件和簽章
wget https://github.com/max05210238/CryptoBar/releases/download/v1.00/firmware.bin
wget https://github.com/max05210238/CryptoBar/releases/download/v1.00/firmware.bin.asc

# 4. 驗證簽章
gpg --verify firmware.bin.asc firmware.bin
```

**驗證成功（綠色）：**
```
gpg: Signature made Wed 26 Dec 2025 10:30:00 AM PST
gpg:                using RSA key ABCD1234EFGH5678IJKL9012MNOP3456QRST7890
gpg: Good signature from "max05210238 (CryptoBar Release Signing Key)" [ultimate]
```

**驗證失敗（紅色）：**
```
gpg: BAD signature from "max05210238 (CryptoBar Release Signing Key)"
```
→ **文件被篡改，絕對不要安裝！**

---

### 🔑 私鑰安全管理

**⚠️ 超級重要：私鑰洩漏 = 遊戲結束**

**保護私鑰：**
1. **絕對不要**提交私鑰到 GitHub
2. **絕對不要**分享私鑰給任何人
3. **備份**私鑰到加密 USB 隨身碟（放在安全的地方）
4. **設置強密碼**保護私鑰（生成時會要求）

**備份私鑰：**
```bash
# 導出私鑰（加密備份）
gpg --armor --export-secret-keys 1234ABCD5678EFGH > private-key-BACKUP.asc

# 存到加密 USB，然後刪除這個文件
rm private-key-BACKUP.asc
```

**如果私鑰被盜：**
1. 立即撤銷金鑰（生成撤銷證書）
2. 公告所有用戶
3. 生成新金鑰對
4. 重新簽署所有發布

---

## 2. SHA256 校驗和

### 🤔 什麼是 SHA256？

**SHA256 (Secure Hash Algorithm 256-bit)** 是一個**單向哈希函數**。

**類比理解：**
想像文件是一本書：
- 📚 **原書** = firmware.bin（固件）
- 🔍 **書的指紋** = SHA256 校驗和（64 個字元）

**特性：**
- 同樣的文件 → 同樣的哈希（確定性）
- 文件改變一個 bit → 哈希完全不同（雪崩效應）
- 無法從哈希反推原文件（單向性）
- 幾乎不可能找到兩個文件有相同哈希（抗碰撞性）

---

### 🔐 SHA256 工作原理

**數學魔法：**

```
firmware.bin (1,024,000 bytes)
        ↓
    SHA256 算法
        ↓
64 個字元的哈希值
(256 bits = 32 bytes = 64 hex digits)

示例：
a3f2b8c9d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1
```

**為什麼安全？**

1. **確定性：** 同樣的文件 → 永遠同樣的哈希
   ```
   SHA256(firmware_v1.00.bin) = a3f2b8c9d4e5f6a7...
   SHA256(firmware_v1.00.bin) = a3f2b8c9d4e5f6a7...  ← 相同
   ```

2. **雪崩效應：** 改變 1 個 bit → 哈希完全不同
   ```
   原始：SHA256(firmware.bin) = a3f2b8c9d4e5f6a7...
   修改一個字節：
   篡改：SHA256(firmware.bin) = 7c1d2e3f4a5b6c7d...  ← 完全不同！
   ```

3. **單向性：** 無法從哈希反推文件
   ```
   知道 a3f2b8c9d4e5f6a7... → 無法得知原始文件內容
   ```

---

### 🛠️ 如何實施 SHA256 校驗和

#### 生成校驗和

```bash
# macOS/Linux
sha256sum firmware.bin > firmware.bin.sha256

# macOS (替代命令)
shasum -a 256 firmware.bin > firmware.bin.sha256

# Windows PowerShell
Get-FileHash firmware.bin -Algorithm SHA256 | Out-File firmware.bin.sha256
```

**生成的 `firmware.bin.sha256` 內容：**
```
a3f2b8c9d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1  firmware.bin
```

---

#### 發布時包含校驗和

**GitHub Release 中應該有：**
```
Release V1.00
├── firmware.bin          (1.05 MB) - 固件
├── firmware.bin.asc      (479 bytes) - GPG 簽章
└── firmware.bin.sha256   (97 bytes) - SHA256 校驗和
```

**Release 說明中添加：**
```markdown
## Download

- **Firmware:** [firmware.bin](https://github.com/.../firmware.bin)
- **GPG Signature:** [firmware.bin.asc](https://github.com/.../firmware.bin.asc)
- **SHA256 Checksum:** [firmware.bin.sha256](https://github.com/.../firmware.bin.sha256)

### Verification

**SHA256:**
```
a3f2b8c9d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1
```

**Verify:**
```bash
sha256sum --check firmware.bin.sha256
```
```

---

#### 用戶如何驗證

**方法 1：自動驗證（推薦）**
```bash
# 下載校驗和文件
wget https://github.com/.../firmware.bin.sha256

# 驗證（會自動比較）
sha256sum --check firmware.bin.sha256
```

**成功輸出：**
```
firmware.bin: OK ✅
```

**失敗輸出：**
```
firmware.bin: FAILED ❌
sha256sum: WARNING: 1 computed checksum did NOT match
```

---

**方法 2：手動驗證**
```bash
# 計算下載文件的哈希
sha256sum firmware.bin

# 輸出：
a3f2b8c9d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1

# 用眼睛比對 Release 說明中的哈希
# 一個字元不同 = 文件被篡改！
```

---

### 🆚 SHA256 vs. GPG

| 功能 | SHA256 校驗和 | GPG 簽章 |
|------|--------------|---------|
| **驗證內容** | 文件完整性 | 文件來源（發布者身份） |
| **防止** | 意外損壞、傳輸錯誤 | 惡意篡改、冒名頂替 |
| **攻擊場景** | 下載過程中斷 | 攻擊者上傳假固件 |
| **易用性** | 非常簡單 | 需要學習 GPG |
| **安全強度** | 中等（防意外） | 高（防惡意） |

**最佳實踐：兩者都用！**
- SHA256：快速檢查文件完整性
- GPG：證明文件確實來自你

---

## 3. GitHub Actions CI/CD

### 🤔 什麼是 CI/CD？

**CI/CD (Continuous Integration / Continuous Deployment)** = 自動化構建和發布

**目前的流程（手動）：**
```
你在電腦上：
1. 修改代碼
2. 運行 pio run
3. 得到 firmware.bin
4. 手動上傳到 GitHub Release
```

**問題：**
- ❌ 用戶無法確認 .bin 是從公開源碼編譯的
- ❌ 你可能在本地修改代碼但不提交（後門）
- ❌ 編譯環境不透明（你用什麼編譯器？什麼設置？）

---

### ✅ GitHub Actions 如何解決

**自動化流程：**
```
GitHub 伺服器上（所有人可見）：
1. 你推送代碼到 GitHub
2. GitHub Actions 自動觸發
3. 在乾淨的虛擬機上編譯（Ubuntu Linux）
4. 生成 firmware.bin
5. 自動上傳到 Release
6. 自動生成 SHA256 校驗和
7. 自動用 GPG 簽署（如果配置了）
```

**透明度：**
- ✅ **構建日誌公開** → 任何人可以查看編譯過程
- ✅ **源碼 → 固件可追溯** → 固件確實來自 GitHub 上的代碼
- ✅ **可重現構建** → 相同源碼 = 相同固件（bit-by-bit）

---

### 🛠️ 如何實施 GitHub Actions

#### 步驟 1：創建 Workflow 文件

在倉庫中創建：`.github/workflows/build-firmware.yml`

```yaml
name: Build Firmware

# 觸發條件：推送 tag 時（例如 v1.00）
on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    # 1. 拉取代碼
    - name: Checkout code
      uses: actions/checkout@v3

    # 2. 設置 Python 環境
    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.11'

    # 3. 安裝 PlatformIO
    - name: Install PlatformIO
      run: |
        pip install platformio

    # 4. 編譯固件
    - name: Build firmware
      run: |
        pio run

    # 5. 生成 SHA256 校驗和
    - name: Generate SHA256
      run: |
        cd .pio/build/esp32-s3-devkitc-1/
        sha256sum firmware.bin > firmware.bin.sha256

    # 6. 上傳到 Release
    - name: Create Release
      uses: softprops/action-gh-release@v1
      with:
        files: |
          .pio/build/esp32-s3-devkitc-1/firmware.bin
          .pio/build/esp32-s3-devkitc-1/firmware.bin.sha256
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

---

#### 步驟 2：推送 Tag 觸發構建

```bash
# 1. 提交最新代碼
git add .
git commit -m "Prepare for V1.00 release"
git push

# 2. 創建 tag
git tag v1.00

# 3. 推送 tag（觸發 GitHub Actions）
git push origin v1.00
```

---

#### 步驟 3：查看構建過程

1. 前往 GitHub 倉庫
2. 點擊 "Actions" 標籤
3. 看到正在運行的 workflow
4. 點擊進入，查看即時日誌

**日誌示例：**
```
Run pio run
Processing esp32-s3-devkitc-1 (platform: espressif32; board: esp32-s3-devkitc-1)
---------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32-s3-devkitc-1.html
PLATFORM: Espressif 32 (6.4.0) > Espressif ESP32-S3-DevKitC-1
HARDWARE: ESP32S3 240MHz, 320KB RAM, 16MB Flash
...
Linking .pio/build/esp32-s3-devkitc-1/firmware.elf
Building .pio/build/esp32-s3-devkitc-1/firmware.bin
Creating esp32s3 image...
Successfully created esp32s3 image.
...
```

**所有步驟都是公開的！** 任何人都能驗證編譯過程。

---

#### 步驟 4：自動生成的 Release

構建完成後，GitHub 自動創建 Release：

```
Release v1.00
Created by GitHub Actions

Assets:
- firmware.bin (1.05 MB) ← 自動上傳
- firmware.bin.sha256 (97 bytes) ← 自動生成
- Source code (zip)
- Source code (tar.gz)
```

---

### 🔐 添加 GPG 自動簽署（進階）

**讓 GitHub Actions 自動簽署固件：**

#### 步驟 1：導出私鑰為 base64

```bash
# 導出私鑰（包含密碼保護）
gpg --armor --export-secret-keys 1234ABCD5678EFGH | base64 > gpg-private-key-base64.txt

# 複製這個文件的內容
cat gpg-private-key-base64.txt
```

---

#### 步驟 2：添加到 GitHub Secrets

1. GitHub 倉庫 → Settings → Secrets and variables → Actions
2. 點擊 "New repository secret"
3. Name: `GPG_PRIVATE_KEY`
4. Value: 貼上 base64 私鑰內容
5. 點擊 "Add secret"

**另外添加：**
- Name: `GPG_PASSPHRASE`
- Value: 你的 GPG 私鑰密碼

---

#### 步驟 3：更新 Workflow

```yaml
    # 在編譯後添加：

    # 6. 導入 GPG 私鑰
    - name: Import GPG key
      run: |
        echo "${{ secrets.GPG_PRIVATE_KEY }}" | base64 --decode | gpg --import
        echo "${{ secrets.GPG_PASSPHRASE }}" | gpg --batch --yes --passphrase-fd 0 --quick-set-primary-uid 1234ABCD5678EFGH

    # 7. 簽署固件
    - name: Sign firmware
      run: |
        cd .pio/build/esp32-s3-devkitc-1/
        echo "${{ secrets.GPG_PASSPHRASE }}" | gpg --batch --yes --passphrase-fd 0 --detach-sign --armor firmware.bin

    # 8. 上傳（包含簽章）
    - name: Create Release
      uses: softprops/action-gh-release@v1
      with:
        files: |
          .pio/build/esp32-s3-devkitc-1/firmware.bin
          .pio/build/esp32-s3-devkitc-1/firmware.bin.asc
          .pio/build/esp32-s3-devkitc-1/firmware.bin.sha256
```

**現在完全自動化！** GitHub Actions 會自動：
1. 編譯固件
2. 生成 SHA256
3. 用 GPG 簽署
4. 上傳所有文件到 Release

---

## 4. 實施計劃 (for V1.00)

### 📅 準備 V1.00 發布的步驟

#### 階段 1：GPG 設置（1-2 天）

**任務清單：**
- [ ] 安裝 GPG：`brew install gnupg` 或 `apt-get install gnupg`
- [ ] 生成 GPG 金鑰對：`gpg --full-generate-key`
- [ ] 導出公鑰：`gpg --armor --export > gpg-keys/cryptobar-release-key.asc`
- [ ] 提交公鑰到倉庫
- [ ] 備份私鑰到加密 USB
- [ ] 測試簽署一個測試文件

**驗證：**
```bash
# 測試簽署
echo "test" > test.txt
gpg --detach-sign --armor test.txt

# 測試驗證
gpg --verify test.txt.asc test.txt
# 應該看到 "Good signature"
```

---

#### 階段 2：SHA256 集成（30 分鐘）

**任務清單：**
- [ ] 創建腳本生成校驗和
- [ ] 測試：`sha256sum firmware.bin > firmware.bin.sha256`
- [ ] 測試驗證：`sha256sum --check firmware.bin.sha256`
- [ ] 在發布流程中添加校驗和生成

**創建腳本：`scripts/generate-checksums.sh`**
```bash
#!/bin/bash
cd .pio/build/esp32-s3-devkitc-1/
sha256sum firmware.bin > firmware.bin.sha256
echo "SHA256 checksum generated:"
cat firmware.bin.sha256
```

---

#### 階段 3：GitHub Actions 設置（2-3 小時）

**任務清單：**
- [ ] 創建 `.github/workflows/build-firmware.yml`
- [ ] 測試基本構建（不含簽署）
- [ ] 推送測試 tag：`git tag v0.99r-test`
- [ ] 檢查 Actions 日誌，確認編譯成功
- [ ] 添加 GPG 簽署（私鑰到 Secrets）
- [ ] 完整測試：tag → 構建 → 簽署 → Release

**測試流程：**
```bash
# 創建測試 tag
git tag v0.99r-test
git push origin v0.99r-test

# 前往 GitHub → Actions 查看構建
# 如果失敗，檢查日誌並修正

# 測試成功後，刪除測試 tag
git tag -d v0.99r-test
git push origin :refs/tags/v0.99r-test
```

---

#### 階段 4：文檔更新（1 小時）

**需要更新的文檔：**

1. **README.md** - 添加驗證說明
   ```markdown
   ## 📦 Firmware Verification

   All releases are signed with GPG and include SHA256 checksums.

   **Verify integrity:**
   ```bash
   sha256sum --check firmware.bin.sha256
   ```

   **Verify authenticity:**
   ```bash
   gpg --import gpg-keys/cryptobar-release-key.asc
   gpg --verify firmware.bin.asc firmware.bin
   ```
   ```

2. **OTA_UPDATE_GUIDE.md** - 添加驗證步驟
   - 在 "Download Firmware File" 章節添加驗證說明
   - 提供 Windows/macOS/Linux 驗證命令

3. **創建 SECURITY.md**
   ```markdown
   # Security Policy

   ## Reporting Vulnerabilities

   Contact: [你的郵箱]

   ## Release Verification

   All firmware releases are:
   - GPG signed by key: `1234ABCD5678EFGH`
   - SHA256 checksum included
   - Built automatically by GitHub Actions
   ```

---

#### 階段 5：V1.00 發布（30 分鐘）

**發布流程：**

```bash
# 1. 確認代碼穩定
# 運行所有測試，確認沒有 bug

# 2. 更新版本號
# 修改 platformio.ini 和代碼中的版本字串

# 3. 提交最終代碼
git add .
git commit -m "Release V1.00 - First stable release with security measures"
git push

# 4. 創建發布 tag
git tag -a v1.00 -m "V1.00 - First stable release"
git push origin v1.00

# 5. GitHub Actions 自動構建
# 前往 Actions 查看進度

# 6. 構建完成後，編輯 Release 說明
# GitHub Releases → Edit release
# 添加更新日誌、校驗和、驗證說明
```

**Release 說明模板：**
```markdown
# CryptoBar V1.00 - First Stable Release 🎉

## What's New
- [功能列表]
- [Bug 修復]

## Security Enhancements
- ✅ GPG signed releases
- ✅ SHA256 checksums
- ✅ Automated builds via GitHub Actions

## Download
- **Firmware:** [firmware.bin](...)
- **GPG Signature:** [firmware.bin.asc](...)
- **SHA256 Checksum:** [firmware.bin.sha256](...)

## Verification

**SHA256:**
```
[實際的校驗和]
```

**GPG:**
```bash
gpg --import gpg-keys/cryptobar-release-key.asc
gpg --verify firmware.bin.asc firmware.bin
```

See [OTA Update Guide](docs/guides/OTA_UPDATE_GUIDE.md) for detailed instructions.
```

---

### ⏱️ 總時間估計

| 階段 | 時間 | 難度 |
|------|------|------|
| GPG 設置 | 1-2 天 | 中等 |
| SHA256 集成 | 30 分鐘 | 簡單 |
| GitHub Actions | 2-3 小時 | 中等 |
| 文檔更新 | 1 小時 | 簡單 |
| V1.00 發布 | 30 分鐘 | 簡單 |
| **總計** | **2-3 天** | - |

**建議時間表：**
- 第 1 天：GPG 設置 + SHA256（測試階段）
- 第 2 天：GitHub Actions 設置 + 測試
- 第 3 天：文檔更新 + V1.00 正式發布

---

## 5. 常見問題

### Q1：如果我丟失了 GPG 私鑰怎麼辦？

**A:**
1. 立即生成撤銷證書（如果還沒有）
2. 公告所有用戶舊金鑰作廢
3. 生成新金鑰對
4. 重新簽署所有發布（如果可能）

**預防：**
- 生成金鑰時同時生成撤銷證書
- 備份私鑰到加密 USB（多個副本）
- 記錄金鑰密碼在安全的地方

---

### Q2：GitHub Actions 構建失敗怎麼辦？

**A:**
1. 查看 Actions 日誌找出錯誤
2. 常見問題：
   - 依賴問題：檢查 platformio.ini
   - 環境問題：確認 Python 版本
   - GPG 問題：檢查 Secrets 配置
3. 本地修復後重新推送

---

### Q3：用戶不會用 GPG 怎麼辦？

**A:**
- 提供詳細的圖文教程
- SHA256 作為備選（更簡單）
- 大多數用戶會直接信任（但提供選項）

**現實：**
- 90% 用戶不會驗證簽章
- 但 10% 進階用戶會驗證
- 更重要的是**透明度**和**可驗證性**

---

### Q4：GitHub Actions 會增加成本嗎？

**A:**
不會！

- GitHub Actions 對公開倉庫**完全免費**
- 每月 2,000 分鐘免費額度（私有倉庫）
- 編譯 CryptoBar 約 5-10 分鐘
- 即使每天發布也用不完額度

---

### Q5：可重現構建是什麼意思？

**A:**
**可重現構建（Reproducible Builds）** = 相同源碼編譯出 bit-by-bit 相同的固件

**為什麼重要：**
- 任何人都能驗證固件確實來自源碼
- 無法隱藏後門（編譯結果必須相同）

**PlatformIO 默認接近可重現：**
- 固定工具鏈版本
- 固定編譯器設置
- 但時間戳可能不同（可以禁用）

**100% 可重現需要：**
```ini
[env:esp32-s3-devkitc-1]
build_flags =
    -DBUILD_TIMESTAMP=0  ; 固定時間戳
    -fno-random-seed      ; 禁用隨機種子
```

---

## 6. 總結

### 🔐 安全三角形

```
         GPG 簽章
        /        \
       /          \
      /   安全的   \
     /    V1.00     \
    /    發布流程    \
   /________________\
 SHA256         GitHub Actions
 校驗和          自動化構建
```

**三者配合：**
1. **SHA256** - 快速檢查完整性
2. **GPG** - 證明來源真實性
3. **GitHub Actions** - 確保透明度

**給用戶的承諾：**
> 「所有 V1.00+ 的發布都經過加密簽章驗證，
> 構建過程公開透明，
> 你可以信任但也可以驗證。」
>
> **Trust, but verify.** ✅

---

**下一步：開始實施！** 🚀

如果在實施過程中遇到問題，隨時詢問。祝 V1.00 發布成功！
