# CryptoBar - Pending Items Checklist

**Last Updated:** 2025-12-25
**Owner:** max05210238

---

## 📋 Documentation Completion Checklist

### High Priority (Needed for Full Launch)

- [ ] **Product Photos/Videos**
  - [ ] Assembled device photo (front view)
  - [ ] Assembled device photo (side/back view)
  - [ ] Device in use (showing display with crypto prices)
  - [ ] LED status demonstration (different colors)
  - [ ] 3D printed enclosure (before assembly)
  - [ ] Internal assembly photo (optional, for advanced users)
  - [ ] Short demo video (optional, 30-60 seconds)
  - **Target location:** `docs/images/`
  - **Usage:** README.md header, HARDWARE_GUIDE.md

- [ ] **3D Printing Files**
  - [x] Create `hardware/3d-printed-case/` directory ✅
  - [ ] Upload STL files for enclosure
  - [ ] Add printing instructions (material, layer height, supports)
  - [ ] Add assembly instructions with photos
  - **Files needed:** Base, cover, any mounting parts

- [ ] **Product Page/Purchase Link**
  - [ ] Complete DIY kit product page
  - [ ] Add product URL to README.md
  - [ ] Add product URL to HARDWARE_GUIDE.md
  - **Product description draft:**
    > Professional DIY Kit with pre-configured components:
    > - ✅ ESP32-S3 with 5V-out pad pre-soldered
    > - ✅ Latest firmware pre-flashed (V0.99q)
    > - ✅ Encoder module with ribbon cable pre-soldered (correct orientation)
    > - ✅ Heat set inserts pre-installed in enclosure
    > - ✅ Optional: Laser-engraved backplate (premium finish)
    > - 🔧 Assembly required: Plug-and-play component installation
    > - ⏱️ Assembly time: 15-30 minutes

### Medium Priority (Can be added later)

- [ ] **Wiring Diagram/Schematic**
  - [ ] Create simple wiring diagram (Fritzing or hand-drawn)
  - [ ] Or create detailed pin connection table
  - **Target location:** `docs/images/wiring_diagram.png`
  - **Usage:** HARDWARE_GUIDE.md

- [ ] **Assembly Video Tutorial**
  - [ ] Record step-by-step assembly process
  - [ ] Upload to YouTube
  - [ ] Link in README.md and HARDWARE_GUIDE.md

- [ ] **Troubleshooting Guide Expansion**
  - [ ] Add common assembly mistakes
  - [ ] Add hardware debugging steps
  - [ ] Add photos of correct/incorrect assembly

### Low Priority (Nice to Have)

- [ ] **Enclosure Customization Guide**
  - [ ] How to modify STL files
  - [ ] Alternative mounting options
  - [ ] Paint/finish recommendations

- [ ] **Advanced Modification Guide**
  - [ ] How to use different display sizes
  - [ ] How to add battery power
  - [ ] Custom PCB design (if created)

---

## 🛠️ Technical Specifications - Collected Information

### Hardware Components (from Amazon links)

| Component | Qty/Device | Amazon Link | Notes |
|-----------|------------|-------------|-------|
| ESP32-S3 N16R8 | 1 | https://amzn.to/4952HZK | 16MB Flash, 8MB PSRAM |
| WaveShare 2.9" e-Paper | 1 | https://amzn.to/4pdoJj1 | B&W display module |
| KY-040 Rotary Encoder | 1 | https://amzn.to/3Lip1Hm | With ribbon cable kit |
| WS2812B LED Strip | 1 LED | https://amzn.to/3YPYPXD | Only need 1 LED per device |
| M3 x 8mm Hex Screws | 2 | https://amzn.to/3L2rsOg | Kit contains many |
| M2 x 5mm Flat Head Screws | 4 | https://amzn.to/44HXbLf | Kit contains many |
| M3 Heat Set Inserts | 2 | https://amzn.to/44HXbLf | - |
| M2 Heat Set Inserts | 4 | https://amzn.to/49rPdbL | - |

**Note:** Many components come in multi-packs. Recommend users to build multiple units.

### GPIO Pin Assignments (from code)

| Component | Pin Function | GPIO Number |
|-----------|--------------|-------------|
| **E-ink Display** | CS (Chip Select) | GPIO 10 |
| | DC (Data/Command) | GPIO 17 |
| | RST (Reset) | GPIO 16 |
| | BUSY | GPIO 4 |
| **Rotary Encoder** | CLK (A Phase) | GPIO 2 |
| | DT (B Phase) | GPIO 1 |
| | SW (Button) | GPIO 21 |
| **Status LED** | NeoPixel Data | GPIO 15 |
| **Board LED** | RGB LED (onboard) | GPIO 48 |

**Power:** 5V via USB-C

### DIY Kit Pre-Configuration Details

Components pre-configured when purchased from official kit:

1. **ESP32-S3 Board Modification**
   - 5V-out soldering pad short-circuited (required for display power)
   - Latest firmware pre-flashed (currently V0.99q)

2. **Encoder Module Modification**
   - Ribbon cable re-soldered in correct orientation
   - KY-040 module pins reversed for proper connection

3. **Enclosure Preparation**
   - M3 heat set inserts installed (2x)
   - M2 heat set inserts installed (4x)
   - Ready for component mounting

4. **Optional Premium Add-on**
   - Laser-engraved backplate (professional finish)
   - Custom branding/designs available

**User Assembly Required:**
- Install components into enclosure
- Connect ribbon cables
- Secure with screws
- Estimated time: 15-30 minutes

---

## 📝 Project Description - Draft Options

### Option 1: Direct & Practical
> **CryptoBar**: An affordable, customizable ESP32-S3 cryptocurrency price display with e-ink screen and rotary encoder control. Built as an open-source alternative to expensive commercial solutions like Tickrmeter, offering essential features without unnecessary complexity or smartphone app dependency.

### Option 2: Feature-Focused
> **CryptoBar**: Open-source cryptocurrency price ticker featuring a 2.9" e-ink display, hardware rotary encoder navigation, multi-currency support (9 currencies), and RGB LED status indicator. Designed for makers who want full customization without the premium price tag.

### Option 3: Problem-Solution
> **CryptoBar**: Fed up with overpriced crypto tickers packed with features you'll never use? CryptoBar delivers what you actually need: real-time prices, clean e-ink display, and intuitive encoder control—all open-source and fully customizable.

### Option 4: Technical & Honest
> **CryptoBar**: ESP32-S3 cryptocurrency display that started as a personal challenge to build a better, cheaper alternative to Tickrmeter. Now open-sourced for the maker community, featuring encoder-based navigation (no phone app needed), multi-API reliability, and complete hardware/software customization.

**Recommended:** Option 4 (authentic story + technical credibility)

**One-sentence version:**
> Open-source ESP32-S3 crypto price display with e-ink screen and encoder control—an affordable, customizable alternative to overpriced commercial tickers.

---

## 🎯 Next Actions

1. **Immediate (this session):**
   - [x] Create tracking checklist ✅
   - [ ] Create `hardware/3d-printed-case/` directory
   - [ ] Implement Phase 1 documentation improvements

2. **User to provide (when available):**
   - [ ] Upload 3D printing STL files
   - [ ] Provide product photos
   - [ ] Complete DIY kit product page
   - [ ] Share product purchase URL

3. **Future enhancements:**
   - [ ] Create wiring diagram
   - [ ] Record assembly tutorial
   - [ ] Expand troubleshooting guide

---

**Status:** Phase 1 documentation improvements in progress
**Blocker:** None (proceeding with available information)
