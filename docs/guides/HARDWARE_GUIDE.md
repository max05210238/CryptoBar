# CryptoBar Hardware Assembly Guide

Complete guide for building your CryptoBar cryptocurrency display device.

---

## 📦 Bill of Materials (BOM)

### Core Electronics

| Item | Description | Qty | Est. Cost | Amazon Link |
|------|-------------|-----|-----------|-------------|
| ESP32-S3 Dev Board | N16R8 (16MB Flash, 8MB PSRAM) | 1 | $10-15 | [Buy](https://amzn.to/4952HZK) |
| E-ink Display | WaveShare 2.9" B&W e-Paper Module | 1 | $20-25 | [Buy](https://amzn.to/4pdoJj1) |
| Rotary Encoder | KY-040 Module with Ribbon Cable Kit | 1 | $8-10 | [Buy](https://amzn.to/3Lip1Hm) |
| RGB LED | WS2812B LED Strip (only need 1 LED) | 1 | $8-12 | [Buy](https://amzn.to/3YPYPXD) |

### Hardware & Fasteners

| Item | Description | Qty Needed | Kit Size | Amazon Link |
|------|-------------|------------|----------|-------------|
| M3 Hex Screws | M3 x 8mm | 2 | Multi-pack | [Buy Kit](https://amzn.to/3L2rsOg) |
| M2 Flat Screws | M2 x 5mm Flat Head | 4 | Multi-pack | [Buy Kit](https://amzn.to/44HXbLf) |
| M3 Heat Set Inserts | Brass, M3 size | 2 | Multi-pack | [Buy](https://amzn.to/44HXbLf) |
| M2 Heat Set Inserts | Brass, M2 size | 4 | Multi-pack | [Buy](https://amzn.to/49rPdbL) |

### Additional Required Items

| Item | Description | Source |
|------|-------------|--------|
| USB-C Cable | For power and programming | Any electronics supplier |
| 5V Power Supply | USB power adapter, 1A minimum | Any electronics supplier |
| 3D Printed Enclosure | See [`hardware/3d-printed-case/`](../../hardware/3d-printed-case/) | 3D printer or service |

### Optional

| Item | Purpose | Cost |
|------|---------|------|
| Laser-Engraved Backplate | Professional finish | Available with DIY kit |
| Soldering Iron | For heat set inserts | $20-50 |
| Wire/Ribbon Cable | Only if kit doesn't include | $5-10 |

---

## 💰 Cost Breakdown

### DIY Build (from scratch)
- **Core Electronics:** $40-60 USD
- **Hardware/Fasteners:** $5-10 USD (kits contain extras)
- **3D Printing:** $5-15 USD (material cost)
- **Shipping:** Varies by location

**Total Estimated Cost:** **$50-85 USD**

**Note:** Hardware kits come with many extras - consider building multiple units with friends to maximize value!

### DIY Kit Option (Coming Soon)
- **Pre-configured kit:** TBD
- **Includes:** Pre-soldered ESP32-S3, pre-flashed firmware, pre-installed inserts
- **Assembly time:** 15-30 minutes vs 2-4 hours

---

## 🔌 GPIO Pin Connections

### Pin Assignment Table

| Component | Function | ESP32-S3 GPIO | Wire Color (typical) | Notes |
|-----------|----------|---------------|----------------------|-------|
| **E-ink Display (SPI)** |
| | CS (Chip Select) | GPIO 10 | Orange/Yellow | SPI slave select |
| | DC (Data/Command) | GPIO 17 | Green | Data/command mode |
| | RST (Reset) | GPIO 16 | Blue | Hardware reset |
| | BUSY | GPIO 4 | Purple/Gray | Status feedback |
| | SCK (Clock) | GPIO 18 | White | SPI clock (default) |
| | MOSI (Data) | GPIO 23 | Yellow | SPI data out (default) |
| | VCC | 3.3V | Red | Power |
| | GND | GND | Black | Ground |
| **Rotary Encoder** |
| | CLK (Phase A) | GPIO 2 | White | PCNT-compatible |
| | DT (Phase B) | GPIO 1 | Green | PCNT-compatible |
| | SW (Button) | GPIO 21 | Blue | Push button (active low) |
| | VCC | 3.3V | Red | Power |
| | GND | GND | Black | Ground |
| **Status LED (WS2812B)** |
| | Data In | GPIO 15 | Green/White | NeoPixel data |
| | VCC | 5V | Red | Power (requires 5V!) |
| | GND | GND | Black | Ground |

### Important Notes

1. **ESP32-S3 PCNT Compatibility**
   - Encoder MUST use GPIO 1 and 2 (PCNT-compatible pins)
   - GPIO 5/6 do NOT support PCNT on ESP32-S3 (common mistake!)

2. **5V Power Rail Requirement**
   - ESP32-S3 dev boards may NOT have 5V-out by default
   - **Required modification:** Short the 5V-out solder jumper on the back
   - This powers the e-ink display and LED from USB 5V
   - **DIY kit users:** This is pre-soldered for you ✅

3. **KY-040 Encoder Module**
   - Kit may have ribbon cable pins in wrong orientation
   - **May require:** Desoldering and reversing ribbon cable connector
   - **DIY kit users:** This is pre-modified for you ✅

---

## 🛠️ Assembly Instructions

### Tools Required

- ✅ Soldering iron (for heat set inserts)
- ✅ Small Phillips screwdriver
- ✅ Wire strippers (if making custom cables)
- ✅ Multimeter (optional, for testing)
- ✅ Tweezers (helpful for small components)

### Step 1: Prepare the Enclosure

1. **3D Print the Enclosure**
   - Download STL files from [`hardware/3d-printed-case/`](../../hardware/3d-printed-case/)
   - Print with PLA or PETG, 0.2mm layer height, 20% infill
   - Allow print to cool completely

2. **Install Heat Set Inserts**
   - Heat soldering iron to 200-220°C (PLA) or 240-260°C (PETG)
   - Carefully insert M3 inserts (2x) into designated holes
   - Insert M2 inserts (4x) into designated holes
   - Hold straight while plastic melts around insert
   - Allow to cool for 1-2 minutes

   **DIY kit users:** Skip this step - inserts pre-installed ✅

3. **Test Fit Components**
   - Dry-fit ESP32-S3 board
   - Check e-ink display mounting area
   - Verify encoder mounting position

### Step 2: Prepare ESP32-S3 Board

1. **Inspect the Board**
   - Locate 5V-out solder pads on the back of the board
   - Check if jumper is already bridged (some boards come pre-bridged)

2. **Bridge 5V-out Pads** (if not already done)
   - Heat soldering iron to 350°C
   - Apply small amount of solder to bridge the two pads
   - Verify continuity with multimeter between 5V pin and USB 5V

   **DIY kit users:** Skip this step - pre-soldered ✅

3. **Flash Firmware** (if building from scratch)
   ```bash
   cd CryptoBar
   pio run -t upload
   ```

   **DIY kit users:** Skip this step - firmware pre-flashed ✅

### Step 3: Prepare KY-040 Encoder Module

1. **Check Ribbon Cable Orientation**
   - KY-040 modules often ship with ribbon cable pins in wrong direction
   - Verify against GPIO diagram above

2. **Modify if Needed**
   - Desolder existing ribbon cable connector (if wrong orientation)
   - Reverse and re-solder ribbon cable
   - Verify pin order: GND, VCC, SW, DT, CLK

   **DIY kit users:** Skip this step - pre-modified ✅

### Step 4: Connect Components

**⚠️ POWER OFF** the ESP32-S3 before making connections!

1. **Connect E-ink Display**
   - Connect 8-pin ribbon cable to display
   - Connect to ESP32-S3 pins according to GPIO table
   - Ensure MOSI and SCK connect to default SPI pins (GPIO 23, GPIO 18)

2. **Connect Rotary Encoder**
   - Connect 5-pin ribbon cable (if not already attached)
   - Wire to ESP32-S3:
     - CLK → GPIO 2
     - DT → GPIO 1
     - SW → GPIO 21
     - VCC → 3.3V
     - GND → GND

3. **Connect Status LED (WS2812B)**
   - Cut ONE LED from WS2812B strip
   - Solder three wires:
     - Data In → GPIO 15
     - VCC → 5V (NOT 3.3V!)
     - GND → GND
   - Verify polarity (check LED markings)

### Step 5: Test Before Final Assembly

1. **Power On**
   - Connect USB-C cable
   - Check for board power LED
   - Check for serial output (115200 baud)

2. **Verify Display**
   - Should show boot splash screen after 3 seconds
   - WiFi portal should activate if no credentials saved

3. **Test Encoder**
   - Rotate encoder - should navigate menu (after WiFi setup)
   - Press encoder - should enter submenu

4. **Test LED**
   - Should show blue during startup
   - Purple during WiFi portal mode
   - Green/red/gray based on price after configuration

### Step 6: Final Assembly

1. **Mount ESP32-S3**
   - Position board in enclosure
   - Secure with M2 screws (4x)

2. **Mount E-ink Display**
   - Route ribbon cable through opening
   - Secure display to front panel
   - Ensure screen is aligned properly

3. **Mount Rotary Encoder**
   - Insert through front panel hole
   - Secure with encoder nut (included with KY-040)

4. **Position Status LED**
   - Mount LED in designated position
   - Secure with hot glue or double-sided tape
   - Ensure visible through light pipe/diffuser

5. **Close Enclosure**
   - Route all cables neatly
   - Verify no pinched wires
   - Secure top cover with M3 screws (2x)

### Step 7: Initial Configuration

1. **First Boot**
   - Device creates WiFi access point: `CryptoBar-XXXXXX`
   - Connect with phone/computer

2. **Configure via Web Portal**
   - Navigate to `http://192.168.4.1`
   - Enter WiFi credentials
   - Select cryptocurrency (default: BTC)
   - Choose display currency (default: USD)
   - Set timezone (auto-detected or manual)
   - Configure refresh settings

3. **Verify Operation**
   - Device connects to WiFi
   - Fetches price data from CoinGecko
   - Updates e-ink display
   - LED shows price trend (green/red)

---

## 🔧 Troubleshooting

### Display Issues

**Problem:** Display shows nothing / stays white
- ✅ Check all 8 display connections (especially CS, DC, RST, BUSY)
- ✅ Verify 5V-out jumper is bridged on ESP32-S3
- ✅ Check display ribbon cable fully inserted
- ✅ Test with serial monitor - look for display init messages

**Problem:** Display is garbled / partial image
- ✅ Verify SPI pins (MOSI=GPIO23, SCK=GPIO18)
- ✅ Check for loose ribbon cable connection
- ✅ Try full refresh mode instead of partial

**Problem:** Ghosting on display
- ✅ Normal for e-ink - use full refresh mode
- ✅ Calibrate display through settings (if available)

### Encoder Issues

**Problem:** Encoder not responding
- ✅ **Critical:** Verify using GPIO 1/2 (not GPIO 5/6!)
- ✅ Check 3.3V power to encoder module
- ✅ Test with serial monitor debug output
- ✅ Verify ribbon cable orientation

**Problem:** Encoder direction reversed
- ✅ Swap CLK and DT wires
- ✅ Or modify `ENC_DIR_INVERT` in code

**Problem:** Encoder very sensitive / jumpy
- ✅ Adjust `ENC_COUNTS_PER_DETENT` in encoder_pcnt.cpp
- ✅ Increase `ENC_DIR_LOCK_MS` for more filtering

### LED Issues

**Problem:** LED not lighting up
- ✅ **Critical:** Verify LED connected to 5V (not 3.3V)
- ✅ Check data pin connected to GPIO 15
- ✅ Verify WS2812B polarity (check LED markings)
- ✅ Test with LED brightness setting (not set to Low or 0)

**Problem:** LED wrong colors
- ✅ Check firmware version (V0.99h+ required for party mode)
- ✅ Verify RGB order (some strips use GRB instead of RGB)

### WiFi Issues

**Problem:** Can't find CryptoBar access point
- ✅ Long-press encoder for 12 seconds to force WiFi portal
- ✅ Check WiFi scanning on phone (2.4GHz only)
- ✅ Move closer to device

**Problem:** Can't connect to home WiFi
- ✅ Verify WiFi credentials (case-sensitive)
- ✅ Check 2.4GHz network (ESP32 doesn't support 5GHz)
- ✅ Verify router compatibility (WPA2 recommended)

### API / Price Fetch Issues

**Problem:** Yellow LED / "API Failure" message
- ✅ Verify internet connection (ping test)
- ✅ Check firewall settings
- ✅ Wait 30-60 seconds for automatic retry
- ✅ CoinGecko may be rate-limited (use 3min+ update interval)

**Problem:** Price not updating
- ✅ Check update interval setting (3min recommended)
- ✅ Verify time is synchronized (check clock display)
- ✅ Review serial monitor for API error messages

---

## 📐 Wiring Diagrams

**Coming Soon:** Detailed Fritzing diagrams will be added here.

For now, refer to the GPIO Pin Connection table above.

---

## 🎨 Customization Ideas

### Hardware Modifications

- **Battery Power**: Add 18650 battery + TP4056 charging module
- **Larger Display**: Adapt for 4.2" or 7.5" e-ink displays
- **External Antenna**: For better WiFi in metal enclosures
- **PCB Design**: Create custom PCB to eliminate wiring

### Enclosure Modifications

- **Wall Mount**: Add mounting holes to back panel
- **Desk Stand**: Design angled stand for better visibility
- **Transparent Cover**: Use clear acrylic for minimalist look
- **RGB Light Pipe**: Add diffuser for LED visibility

---

## 🛒 Where to Buy

### Individual Components
All components available via Amazon links in BOM table above.

### DIY Kit (Coming Soon)
Pre-configured kit with plug-and-play assembly:
- ESP32-S3: 5V-out pre-soldered, firmware pre-flashed
- Encoder: Ribbon cable pre-modified
- Enclosure: Heat set inserts pre-installed
- Optional: Laser-engraved backplate

**Product page:** Coming soon

---

## 📚 Additional Resources

- [Main README](../../README.md) - Project overview
- [LED Display Guide](LED_DISPLAY_GUIDE.md) - LED colors and animations
- [Configuration Guide](CONFIGURATION_GUIDE.md) - Settings explained (Coming Soon)
- [Troubleshooting Guide](TROUBLESHOOTING.md) - Common issues (Coming Soon)
- [Release Notes](../release-notes/) - Technical version details

---

## ❓ Need Help?

- 🐛 [Report hardware issues](https://github.com/max05210238/CryptoBar/issues)
- 💬 [Ask questions](https://github.com/max05210238/CryptoBar/discussions)

---

**Last Updated:** 2025-12-25
**Hardware Version:** V0.99q compatible
