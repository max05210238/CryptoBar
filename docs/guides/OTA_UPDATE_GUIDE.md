# CryptoBar OTA Firmware Update Guide

**Complete guide for safely updating CryptoBar firmware over-the-air**

⚠️ **IMPORTANT SECURITY NOTICE**
This guide includes critical security information about firmware updates. Please read the [Security Considerations](#-security-considerations) section before performing any updates.

---

## 📖 Table of Contents

1. [What is OTA Update?](#-what-is-ota-update)
2. [When to Update](#-when-to-update)
3. [Prerequisites](#-prerequisites)
4. [Step-by-Step Update Process](#-step-by-step-update-process)
5. [Security Considerations](#-security-considerations)
6. [Troubleshooting](#-troubleshooting)
7. [Reverting to Previous Version](#-reverting-to-previous-version)

---

## 🔄 What is OTA Update?

**OTA (Over-The-Air)** update allows you to update CryptoBar firmware wirelessly without connecting USB cable to your computer.

### Benefits
- ✅ **No USB cable required** - Update from any device with web browser
- ✅ **Fast** - Takes ~30 seconds
- ✅ **Safe** - Dual partition system allows rollback if update fails
- ✅ **Convenient** - Update from phone, tablet, or computer

### How It Works

```
┌─────────────────────────────────────────┐
│  1. Device enters OTA mode              │
│     (WiFi stays connected)              │
└─────────────────────────────────────────┘
            ↓
┌─────────────────────────────────────────┐
│  2. You upload .bin file via browser    │
│     (HTTP upload to device IP)          │
└─────────────────────────────────────────┘
            ↓
┌─────────────────────────────────────────┐
│  3. Device writes firmware to           │
│     inactive partition                  │
└─────────────────────────────────────────┘
            ↓
┌─────────────────────────────────────────┐
│  4. Device verifies firmware integrity  │
│     (checks partition table, size, etc) │
└─────────────────────────────────────────┘
            ↓
┌─────────────────────────────────────────┐
│  5. Device reboots to new firmware      │
│     (automatic rollback if boot fails)  │
└─────────────────────────────────────────┘
```

---

## 🕒 When to Update

Update your CryptoBar firmware when:

- ✅ **New features available** - Check [Changelog](../../CHANGELOG.md) for new versions
- ✅ **Bug fixes released** - Security patches or critical bug fixes
- ✅ **Performance improvements** - Better API reliability, faster refresh
- ⚠️ **Security vulnerabilities** - ALWAYS update immediately for security patches

**Check for updates:**
- GitHub Releases: https://github.com/max05210238/CryptoBar/releases
- Current version shown in Menu [11] WiFi Info

---

## ✅ Prerequisites

Before starting the update process:

### 1. Verify Current Version
- Enter main menu (short press encoder)
- Navigate to [11] WiFi Info
- Note your current version (e.g., V0.99q)

### 2. Download Firmware File

**⚠️ ONLY download firmware from official sources:**

#### Option A: GitHub Releases (Recommended)
1. Visit: https://github.com/max05210238/CryptoBar/releases
2. Find the latest release (e.g., "V0.99r")
3. Download: `firmware.bin` or `cryptobar-v0.99r.bin`
4. **Verify file size:** Should be ~800KB-1.2MB (ESP32-S3 firmware)

#### Option B: Build from Source (Advanced)
```bash
# Clone repository
git clone https://github.com/max05210238/CryptoBar.git
cd CryptoBar

# Build with PlatformIO
pio run

# Firmware location:
# .pio/build/esp32-s3-devkitc-1/firmware.bin
```

### 3. Prepare for Update
- ✅ Ensure device is connected to WiFi
- ✅ Device has stable power supply (USB connected)
- ✅ Computer/phone is on **same WiFi network** as CryptoBar
- ✅ Know your CryptoBar's IP address (shown in WiFi Info menu)

**⚠️ DO NOT disconnect power during update!**

---

## 📋 Step-by-Step Update Process

### Step 1: Enter OTA Update Mode

1. **On CryptoBar device:**
   - Short press encoder to open main menu
   - Navigate to `[12] Firmware Update`
   - Short press to select

2. **Display shows:**
   ```
   ┌─────────────────────────┐
   │ OTA Update Mode         │
   │                         │
   │ Connect to:             │
   │ http://192.168.1.100    │  ← Your device IP
   │                         │
   │ Upload .bin file        │
   └─────────────────────────┘
   ```

3. **LED indicator:**
   - 🟣 **Purple (solid)** - OTA mode active
   - Device stays in this mode until you upload firmware or long-press to exit

---

### Step 2: Access OTA Web Interface

1. **On your computer/phone/tablet:**
   - Open web browser (Chrome, Firefox, Safari, etc.)
   - Enter the IP address shown on CryptoBar display
   - Example: `http://192.168.1.100`

2. **You should see:**
   ```
   ┌─────────────────────────────────────┐
   │ CryptoBar Firmware Update           │
   │                                     │
   │ Current Version: V0.99q             │
   │                                     │
   │ [Choose File]  [Upload]             │
   │                                     │
   │ Select .bin file and click Upload   │
   └─────────────────────────────────────┘
   ```

**⚠️ If page doesn't load:**
- Check device and computer are on same WiFi network
- Verify IP address is correct (check WiFi Info menu)
- Try adding `:80` to URL: `http://192.168.1.100:80`
- See [Troubleshooting](#-troubleshooting) section

---

### Step 3: Upload Firmware

1. **Click "Choose File" button**
   - Navigate to downloaded `.bin` file
   - Select the firmware file

2. **Verify file before uploading:**
   - ✅ File name ends with `.bin`
   - ✅ File size is 800KB - 1.2MB (reasonable for ESP32-S3)
   - ⚠️ **DO NOT upload random .bin files!** (See security section)

3. **Click "Upload" button**
   - Progress bar appears
   - Upload takes ~10-30 seconds depending on WiFi speed

4. **Wait for upload completion:**
   ```
   Upload Progress: [████████████████] 100%
   Writing firmware...
   Verifying...
   Success! Device will reboot in 3 seconds...
   ```

---

### Step 4: Device Reboot

1. **Automatic reboot sequence:**
   - Device validates firmware integrity
   - Sets new firmware as active partition
   - Reboots automatically

2. **Boot sequence (on display):**
   ```
   [1] CryptoBar V0.99r  ← New version!
   [2] WiFi Connecting...
   [3] Syncing time...
   [4] Fetching prices...
   [5] Main Display (Ready)
   ```

3. **LED indicator during boot:**
   - 🔵 **Blue** - Booting and connecting WiFi
   - 🟢/🔴 **Green/Red** - Normal operation (price movement)

---

### Step 5: Verify Update Success

1. **Check version:**
   - Enter main menu
   - Navigate to [11] WiFi Info
   - Verify version matches uploaded firmware

2. **Test functionality:**
   - ✅ Price updates working
   - ✅ Encoder navigation responsive
   - ✅ LED showing correct colors
   - ✅ WiFi connected
   - ✅ Time synced correctly

**✅ Update complete!**

---

## 🔒 Security Considerations

### ⚠️ CRITICAL: Malware Risk

Your concern about malicious firmware is **100% valid and important**. Here's what you need to know:

### The Threat: Malicious Firmware

**What could a malicious .bin file do?**

❌ **If you upload infected firmware from an untrusted source:**

1. **Network Surveillance**
   - Scan your local network for devices (computers, phones, NAS)
   - Identify open ports and services
   - Map network topology

2. **Data Theft**
   - Capture WiFi credentials stored on device
   - Sniff unencrypted HTTP traffic on local network
   - Exfiltrate data to remote server

3. **Botnet Recruitment**
   - Device becomes part of botnet
   - Used for DDoS attacks
   - Cryptocurrency mining (drains power)

4. **Lateral Movement**
   - Exploit vulnerabilities in other devices on network
   - Attempt to access shared folders, printers
   - Pivot to attack other devices

**This is NOT theoretical - ESP32 malware exists in the wild.**

---

### 🛡️ GitHub Security Mechanisms

**Does GitHub detect malware?**

GitHub has **multiple security layers**, but **NOT perfect protection:**

#### ✅ What GitHub DOES Provide

1. **Dependency Scanning**
   - Scans `package.json`, `requirements.txt`, etc. for known vulnerable packages
   - Alerts repository owner of vulnerable dependencies
   - **CryptoBar uses:** PlatformIO libraries (ArduinoJson, GxEPD2, etc.)

2. **Secret Scanning**
   - Detects accidentally committed API keys, passwords, tokens
   - Prevents credential leaks

3. **Code Scanning (CodeQL)**
   - Static analysis for common vulnerabilities (XSS, SQL injection, etc.)
   - **Limitation:** Only works for source code, NOT compiled .bin files

4. **Virus Scanning**
   - Microsoft Defender scans uploaded files
   - **Limitation:** Generic malware signatures, not IoT-specific threats

#### ❌ What GitHub DOES NOT Do

1. **❌ Does NOT scan compiled .bin firmware files**
   - Binary files are opaque to static analysis
   - Cannot detect backdoors in compiled ESP32 firmware

2. **❌ Does NOT verify firmware authenticity**
   - No built-in code signing for releases
   - Anyone can fork repo and upload malicious .bin

3. **❌ Does NOT prevent malicious releases**
   - If attacker compromises repository access, they can upload infected firmware
   - No automatic firmware verification before release

---

### 🔐 How to Protect Yourself

Follow these security best practices:

#### 1️⃣ **ONLY Download from Official Repository**

✅ **Trusted source:**
```
https://github.com/max05210238/CryptoBar/releases
```

❌ **DO NOT download from:**
- Random websites offering "faster updates"
- Forum posts with .bin file attachments
- Third-party mirrors
- Unverified forks

#### 2️⃣ **Verify Repository Authenticity**

Before downloading, check:
- ✅ Repository owner is `max05210238`
- ✅ Repository has commit history (not newly created)
- ✅ Release has description explaining changes
- ✅ Release timestamp is recent (actively maintained)

#### 3️⃣ **Build from Source (Most Secure)**

**Recommended for maximum security:**

```bash
# 1. Clone official repository
git clone https://github.com/max05210238/CryptoBar.git
cd CryptoBar

# 2. Inspect source code (look for suspicious network calls)
grep -r "http://" src/
grep -r "WiFi.begin" src/

# 3. Build firmware yourself
pio run

# 4. Upload self-built firmware
# Location: .pio/build/esp32-s3-devkitc-1/firmware.bin
```

**Why this is safer:**
- ✅ You control entire build process
- ✅ Can audit source code for backdoors
- ✅ No risk of pre-compiled malware

#### 4️⃣ **Verify File Integrity (Future Enhancement)**

**Currently NOT implemented, but SHOULD be added:**

```bash
# Future: Releases should include SHA256 checksums
sha256sum firmware.bin
# Compare with checksum in release notes
```

**Recommendation for repository owner:**
- Add SHA256 checksums to all releases
- Ideally: GPG-signed releases for cryptographic verification

#### 5️⃣ **Network Isolation (Advanced)**

For paranoid users:
- Put CryptoBar on isolated IoT VLAN
- Block outbound traffic except to known API endpoints (CoinGecko, NTP)
- Monitor network traffic for suspicious connections

---

### 🔍 How to Inspect Firmware for Malware

**If you're suspicious of a .bin file:**

#### Method 1: Strings Analysis (Basic)

```bash
# Extract readable strings from firmware
strings firmware.bin | grep -i "http"
strings firmware.bin | grep -i "password"
strings firmware.bin | grep -i "botnet"

# Look for:
# - Unknown URLs (not CoinGecko, NTP servers)
# - Suspicious IP addresses
# - Common malware strings
```

#### Method 2: ESP32 Firmware Analysis (Advanced)

```bash
# Install esptool
pip install esptool

# Dump firmware sections
esptool.py image_info firmware.bin

# Look for:
# - Unreasonable file size (>2MB for CryptoBar is suspicious)
# - Multiple suspicious partitions
# - Encrypted sections (not used in CryptoBar)
```

#### Method 3: Network Traffic Monitoring (Runtime)

```bash
# After update, monitor device network activity
# Use Wireshark or tcpdump on your router

# Expected traffic:
# - DNS queries to api.coingecko.com, pool.ntp.org
# - HTTPS to CoinGecko API
# - NTP sync to time servers

# Suspicious traffic:
# - Connections to unknown IPs
# - Port scanning activity (SYN packets to multiple ports)
# - Large data uploads to unknown servers
```

---

### 🚨 Red Flags - DO NOT Install Firmware If:

| Red Flag | Why It's Dangerous |
|----------|-------------------|
| File size > 2MB | ESP32-S3 firmware shouldn't exceed ~1.2MB |
| Uploaded by unknown user | Not from official repository owner |
| No release notes | Legitimate releases explain changes |
| Released same day as fork | Possible hijack attempt |
| Requests unusual permissions | ESP32 doesn't need OS permissions |
| Comes with "crack" or "modded" label | Obvious malware risk |

---

### ✅ CryptoBar Source Code Transparency

**Why CryptoBar is relatively safe:**

1. **Open source** - All code is public on GitHub
2. **Simple architecture** - Limited attack surface
3. **No cloud services** - Device doesn't phone home
4. **Standard APIs** - Only connects to public cryptocurrency APIs
5. **No user data collection** - No analytics, telemetry, or tracking

**What CryptoBar firmware does:**
- ✅ Connects to WiFi (credentials you provided)
- ✅ Fetches crypto prices from CoinGecko/Binance APIs
- ✅ Syncs time via NTP
- ✅ Updates e-ink display
- ✅ Reads encoder input

**What it does NOT do:**
- ❌ Collect user data
- ❌ Phone home to unknown servers
- ❌ Scan network
- ❌ Open backdoor ports
- ❌ Mine cryptocurrency

---

### 📝 Security Recommendations for Repository Owner

**To improve security for all users, consider adding:**

1. **Release Signing**
   ```bash
   # Sign releases with GPG key
   gpg --detach-sign -a firmware.bin
   # Publish public key in repository
   ```

2. **SHA256 Checksums**
   - Include checksums in release notes
   - Users can verify: `sha256sum firmware.bin`

3. **GitHub Actions CI/CD**
   - Automated builds from source
   - Reproducible builds (same source = same .bin)
   - Build logs publicly visible

4. **Security Policy**
   - Add SECURITY.md to repository
   - Responsible disclosure process
   - Security contact email

5. **Two-Factor Authentication**
   - Protect repository access
   - Prevent account compromise

---

## 🔧 Troubleshooting

### Update Failed / Device Won't Boot

**Symptom:** Device stuck on boot screen or reboots continuously.

**Cause:** Corrupted firmware upload or incompatible version.

**Solution:**
1. **Automatic rollback** - ESP32 OTA has built-in rollback
   - If new firmware fails to boot 3 times, reverts to previous version
   - Wait ~60 seconds for automatic rollback

2. **Manual recovery via USB**
   ```bash
   # Connect USB cable
   # Re-flash using PlatformIO
   pio run -t upload
   ```

---

### Can't Access OTA Web Interface

**Symptom:** Browser shows "Connection refused" or timeout.

**Solutions:**

1. **Check IP address**
   - Verify IP shown on display matches what you typed
   - Try adding `:80` to URL: `http://192.168.1.100:80`

2. **Same network**
   - Ensure computer/phone is on same WiFi as CryptoBar
   - Disable VPN if active

3. **Firewall**
   - Temporarily disable firewall on computer
   - Check router doesn't block device-to-device communication

4. **Exit and re-enter OTA mode**
   - Long press encoder to exit OTA mode
   - Re-enter from main menu

---

### Upload Fails Halfway

**Symptom:** Upload progress bar stops at 50-80%.

**Causes:**
- Weak WiFi signal
- File too large (>1.5MB)
- Wrong file format (not .bin)

**Solutions:**
1. Move device closer to WiFi router
2. Verify file is actually .bin firmware (not .elf, .hex, etc.)
3. Retry upload
4. Use USB cable method as fallback

---

### Version Doesn't Change After Update

**Symptom:** WiFi Info still shows old version after update.

**Possible causes:**
1. **Uploaded wrong file** - Re-check firmware file
2. **Update failed silently** - Check serial output via USB
3. **Cached partition** - Power cycle device (unplug USB, wait 10 seconds)

---

## ↩️ Reverting to Previous Version

If you want to downgrade to an older version:

### Method 1: Download Old Release

1. Go to GitHub Releases
2. Find older version (e.g., V0.99p)
3. Download that version's .bin file
4. Follow normal OTA update process

### Method 2: Rebuild Old Version from Source

```bash
# Checkout old version
git checkout v0.99p

# Build
pio run

# Upload via OTA or USB
```

⚠️ **Note:** Downgrading may lose settings if NVS format changed between versions.

---

## 📚 Additional Resources

- **Changelog:** [CHANGELOG.md](../../CHANGELOG.md) - All version changes
- **Hardware Guide:** [HARDWARE_GUIDE.md](HARDWARE_GUIDE.md) - USB connection info
- **Display Guide:** [DISPLAY_GUIDE.md](DISPLAY_GUIDE.md) - Menu navigation
- **GitHub Releases:** https://github.com/max05210238/CryptoBar/releases

---

## ⚠️ Final Security Reminder

**Before clicking "Upload":**
1. ✅ Downloaded from official repository?
2. ✅ Repository owner is `max05210238`?
3. ✅ File size reasonable (~800KB-1.2MB)?
4. ✅ Release notes explain what changed?
5. ✅ Other users reporting success?

**When in doubt:**
- ❌ **DO NOT upload unknown firmware**
- ✅ **Build from source instead**
- ✅ **Ask in GitHub Issues/Discussions**

Your security is YOUR responsibility. Stay vigilant!

---

**Last Updated:** V0.99q (2025-12-26)
