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

## 🛡️ OTA Rollback Safety (Anti-Brick Protection)

CryptoBar uses a **dual-partition OTA system** to prevent bricking your device.

### How Dual Partitions Work

Your ESP32-S3 has **TWO firmware slots** (A/B partitions):

```
┌─────────────────────────────────────────────┐
│  ESP32-S3 Flash Memory (16MB)               │
├─────────────────────────────────────────────┤
│  Partition: app0 (ota_0)  ← Slot A          │
│  Size: ~1.5MB                               │
│  Status: Running (current firmware)         │
├─────────────────────────────────────────────┤
│  Partition: app1 (ota_1)  ← Slot B          │
│  Size: ~1.5MB                               │
│  Status: Inactive (backup/update target)    │
└─────────────────────────────────────────────┘
```

**How it protects you:**
- Only ONE partition runs at a time
- The OTHER partition is used for updates
- If new firmware fails → device automatically boots from old partition

---

### Automatic Rollback Mechanism

CryptoBar has **two layers** of protection:

#### Layer 1: OTA Safety Guard (NVS-based)

**Policy:** 2 failed boots → automatic rollback

```
Upload new firmware (V1.01)
        ↓
Writes to inactive partition (app1)
        ↓
Reboot and try to run V1.01
        ↓
┌─────────────────────────────────┐
│  Boot Attempt #1                │
│  - Firmware starts              │
│  - Crashes or won't boot        │
│  → Counter: attempts = 1        │
└─────────────────────────────────┘
        ↓
Device reboots automatically
        ↓
┌─────────────────────────────────┐
│  Boot Attempt #2                │
│  - Firmware starts again        │
│  - Still crashes                │
│  → Counter: attempts = 2        │
│  → TRIGGER ROLLBACK!            │
└─────────────────────────────────┘
        ↓
Device switches back to app0 (old firmware)
        ↓
Boots successfully with V1.00 (previous version)
```

**Rollback triggers:**
- 2 consecutive boot failures
- Firmware crashes before 25-second mark
- Watchdog timer resets

**Success criteria (new firmware marked valid):**
- Device runs stably for **25 seconds**
- All systems initialized (WiFi, display, encoder)
- No crashes or panics

---

#### Layer 2: ESP-IDF Native Rollback (Bootloader-level)

**Built into ESP32-S3 bootloader:**
- Validates firmware signature before boot
- Checks partition table integrity
- Hardware-level protection

---

### What You See During Rollback

**Scenario: You upload corrupted firmware**

1. **Upload completes:**
   ```
   Upload Progress: [████████████████] 100%
   Writing firmware to app1...
   Success! Device will reboot in 3 seconds...
   ```

2. **First boot attempt fails:**
   ```
   [Display shows nothing or freezes]
   → Device auto-reboots after watchdog timeout (~10s)
   ```

3. **Second boot attempt fails:**
   ```
   [Still won't boot properly]
   → Rollback triggered!
   ```

4. **Device boots old firmware:**
   ```
   ┌─────────────────────────┐
   │ CryptoBar V1.00         │  ← Previous version
   │ Connecting WiFi...      │
   │ [Normal operation]      │
   └─────────────────────────┘
   ```

**LED indicator during rollback:**
- 🔵 **Blue** - Attempting to boot (may flash multiple times)
- ⚠️ **Pattern:** Blue → crash → Blue → crash → Blue (success with old firmware)

---

### Checking OTA Status

**On maintenance web page (`http://192.168.4.1`):**

```
┌─────────────────────────────────────┐
│ CryptoBar Firmware Update           │
│                                     │
│ Current: V1.00 (2025-12-25 10:30)   │
│ OTA slot: running app0 → target app1│ ← Shows A/B status
│                                     │
│ OTA safety guard: Enabled           │ ← Rollback protection
│ (2 failed boots → rollback)         │
└─────────────────────────────────────┘
```

**Via Menu [11] WiFi Info:**
- Shows current firmware version
- If rollback occurred, version will be old version

**Serial output (if USB connected):**
```
[OTA] Boot attempt 1
[OTA] Boot attempt 2
[OTA] Rollback to app0 (2 failed attempts)
[OTA] Running app0 (previous firmware)
```

---

### Manual Recovery (If Needed)

**Extremely rare, but if both partitions fail:**

1. **Connect USB cable to computer**

2. **Re-flash via PlatformIO:**
   ```bash
   cd CryptoBar
   pio run -t upload
   ```

3. **Or use esptool (if PlatformIO not available):**
   ```bash
   pip install esptool
   esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x0 firmware.bin
   ```

**This will overwrite both partitions with working firmware.**

---

### Technical Details

**OTA partition layout (platformio.ini):**
```ini
board_build.partitions = default_16MB.csv
```

**Partition table:**
```
# Name,     Type, SubType, Offset,  Size
nvs,        data, nvs,     0x9000,  0x5000
otadata,    data, ota,     0xe000,  0x2000
app0,       app,  ota_0,   0x10000, 0x180000  ← Slot A (1.5MB)
app1,       app,  ota_1,   0x190000,0x180000  ← Slot B (1.5MB)
spiffs,     data, spiffs,  0x310000,0xF0000
```

**OTA safety guard (ota_guard.cpp):**
- Max boot attempts before rollback: **2**
- Stability verification time: **25 seconds**
- Storage: NVS (non-volatile memory)
- Automatic: No user intervention needed

---

### Best Practices

**To minimize rollback risk:**

1. ✅ **Only upload firmware from official releases**
   - Verified builds from GitHub
   - GPG-signed releases (V1.00+)

2. ✅ **Check file size before upload**
   - Valid firmware: 800KB - 1.2MB
   - Too small (<500KB) → likely incomplete download
   - Too large (>2MB) → wrong file or corrupted

3. ✅ **Wait for "Success!" message**
   - Don't interrupt upload mid-process
   - Keep phone/browser page open until complete

4. ✅ **Don't disconnect power during update**
   - Keep USB cable connected throughout process
   - Wait for device to fully reboot

5. ✅ **Test new firmware before major updates**
   - If building from source, test locally first
   - Check GitHub issues for reported problems

**If rollback occurs:**
- ❌ **Don't panic** - Your device is still working (old firmware)
- ✅ **Download firmware again** - Previous download may be corrupted
- ✅ **Verify SHA256 checksum** (V1.00+)
- ✅ **Report issue on GitHub** - Help improve firmware quality

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
- ✅ Device has stable power supply (USB connected)
- ✅ Phone/tablet/computer with WiFi capability
- ✅ Downloaded firmware .bin file ready

**⚠️ DO NOT disconnect power during update!**

**Note:** Device will create its own WiFi hotspot for OTA update - you do NOT need to be on the same network beforehand.

---

## 🚪 How to Enter Maintenance Mode

There are **two ways** to enter OTA maintenance mode. Choose the method that works best for you:

### Method 1: Via Menu (Standard Method)

**When to use:** Normal firmware updates, device is working properly

**Steps:**
1. **From main display** - Short press encoder to enter menu
2. **Navigate to [12] "Firmware Update"**
3. **Short press to select** - Confirmation screen appears
4. **Confirm with short press** - Device enters maintenance mode

**Display shows:**
```
┌─────────────────────────┐
│ Firmware Update         │
│                         │
│ Device will reboot into │
│ maintenance mode.       │
│                         │
│ Press to confirm        │
│ Hold to cancel          │
└─────────────────────────┘
```

---

### Method 2: Boot-Time Entry (Quick Access)

**When to use:**
- ✅ **Quick updates** - Skip navigating through menu
- ✅ **Emergency recovery** - Firmware corruption prevents normal menu access
- ✅ **Troubleshooting** - Device boots but UI is broken

**Steps:**

1. **Disconnect power** from CryptoBar (unplug USB)

2. **Press and HOLD encoder button**
   - Push down the rotary encoder knob
   - Keep holding (don't release yet)

3. **While holding button, reconnect power**
   - Plug in USB cable
   - Keep button pressed

4. **Hold for 1-2 seconds after power connected**
   - Wait for display to show maintenance screen
   - Then release button

5. **Device boots directly into maintenance mode**
   - Skips normal startup
   - Creates WiFi hotspot immediately

**Visual Guide:**

```
┌──────────────────────────────────────────┐
│  1. Unplug USB              [Disconnect] │
│         ↓                                │
│  2. Press & Hold Button     [●] ← Hold   │
│         ↓                                │
│  3. Plug in USB (keep holding)           │
│         ↓                                │
│  4. Wait 1-2 seconds        [Display ⚡] │
│         ↓                                │
│  5. Release button          [Maintenance]│
└──────────────────────────────────────────┘
```

**Troubleshooting Boot-Time Entry:**

| Issue | Solution |
|-------|----------|
| **Device boots normally** | Hold button earlier - press BEFORE plugging in power |
| **Nothing happens** | Ensure button is fully pressed down (you should feel click) |
| **Display stays blank** | Wait longer (2-3 seconds) before releasing button |
| **Not sure if it worked** | Check for WiFi hotspot `CryptoBar_MAINT_XXXX` on phone |

**⚡ Pro Tip:** This method is faster than menu navigation if you're doing multiple firmware updates (testing/development).

**⚠️ Important:**
- This does NOT bypass any security - it's an official recovery mechanism
- Same OTA safety features apply (rollback protection, etc.)
- Use when normal menu access isn't convenient or possible

---

## 📋 Step-by-Step Update Process

### Step 1: Enter Firmware Update Confirmation

1. **On CryptoBar device:**
   - Short press encoder to open main menu
   - Navigate to `[12] Firmware Update`
   - Short press to select

2. **Confirmation screen appears:**
   ```
   ┌─────────────────────────┐
   │ Firmware Update         │
   │                         │
   │ Hold button to enter    │
   │ maintenance mode        │
   │ (short press to cancel) │
   └─────────────────────────┘
   ```

3. **Choose:**
   - **Long press (hold)** - Proceed to Step 2
   - **Short press** - Cancel and return to menu

---

### Step 2: Device Creates Maintenance Hotspot

**⚠️ IMPORTANT:** When you long-press to confirm, the device will:
- Disconnect from your WiFi network
- Create a **dedicated WiFi hotspot** for OTA update
- This is NORMAL behavior - your device is NOT broken!

1. **Device disconnects from WiFi** (LED may blink)

2. **Maintenance AP starts** (~5 seconds)
   - LED turns 🟣 **purple (solid)**
   - Device creates open WiFi hotspot (no password)

3. **Display shows connection info:**
   ```
   ┌─────────────────────────┐
   │ Firmware Update         │
   │                         │
   │ 1) Connect phone to:    │
   │    CryptoBar_MAINT_A1B2 │  ← Your unique SSID
   │                         │
   │ 2) Open browser to:     │
   │    http://192.168.4.1   │  ← Always this IP
   │                         │
   │ 3) Upload .bin file     │
   └─────────────────────────┘
   ```

**Important notes:**
- **SSID format:** `CryptoBar_MAINT_XXXX` (XXXX = last 4 digits of device MAC address)
- **No password** - The hotspot is open (temporary, OTA only)
- **Fixed IP:** Always `192.168.4.1` (not your home network IP!)

---

### Step 3: Connect to Maintenance Hotspot

1. **On your phone/tablet/computer:**
   - Open WiFi settings
   - Look for network: `CryptoBar_MAINT_XXXX`
   - Connect to it (no password needed)

2. **Connection confirmation:**
   - You should connect successfully within 5-10 seconds
   - Your device will show "No internet" - **this is normal!**
   - You're connecting directly to CryptoBar, not to the internet

**⚠️ Troubleshooting connection:**
- If SSID doesn't appear: Wait 30 seconds, device may still be starting AP
- If phone auto-disconnects: Disable "Auto-switch network" feature
- On iPhone: Tap "Use Without Internet" when prompted
- On Android: Tap "Stay connected" when warned about no internet

---

### Step 4: Access OTA Web Interface

1. **Open web browser:**
   - While still connected to `CryptoBar_MAINT_XXXX`
   - Navigate to: `http://192.168.4.1`
   - **DO NOT use HTTPS** - device only supports HTTP

2. **You should see OTA web page:**
   ```
   ┌─────────────────────────────────────┐
   │ CryptoBar Firmware Update           │
   │                                     │
   │ Current: V0.99q (2025-12-25 10:30)  │
   │ OTA slot: running app0 → target app1│
   │                                     │
   │ [Choose File]  [Upload & Update]    │
   │                                     │
   │ Tip: keep this page open until it   │
   │      says rebooting                 │
   └─────────────────────────────────────┘
   ```

**⚠️ If page doesn't load:**
- Verify you're connected to `CryptoBar_MAINT_XXXX` (check WiFi settings)
- Try `http://192.168.4.1` again (copy-paste to avoid typos)
- Clear browser cache and retry
- Try different browser (Chrome, Firefox, Safari)
- See [Troubleshooting](#-troubleshooting) section

---

### Step 5: Upload Firmware

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

### Step 6: Device Reboot

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

### Step 7: Verify Update Success

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

1. **Verify connection to maintenance hotspot**
   - Check WiFi settings - should be connected to `CryptoBar_MAINT_XXXX`
   - If not connected, go back to Step 3
   - Ensure you didn't auto-switch back to home WiFi

2. **Check IP address**
   - Maintenance AP always uses: `http://192.168.4.1`
   - Try adding `:80` to URL: `http://192.168.4.1:80`
   - **DO NOT use HTTPS** - device only supports HTTP

3. **Phone/tablet auto-disconnect**
   - Some devices disconnect from "no internet" networks
   - iPhone: Tap "Use Without Internet" when prompted
   - Android: Tap "Stay connected" or disable "Auto-switch network"

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
