# CryptoBar - Pending Items Checklist

**Last Updated:** 2025-12-25
**Owner:** max05210238

---

## 📋 Documentation Completion Checklist

### High Priority (Needed for Full Project Completion)

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

| Component | Qty/Device | Est. Cost | Amazon Link |
|-----------|------------|-----------|-------------|
| ESP32-S3 N16R8 | 1 | ~$6 | https://amzn.to/4952HZK |
| WaveShare 2.9" e-Paper | 1 | ~$27 | https://amzn.to/4pdoJj1 |
| KY-040 Rotary Encoder | 1 | ~$2 | https://amzn.to/3Lip1Hm |
| WS2812B LED Strip | 1 LED | ~$2 | https://amzn.to/3YPYPXD |
| M3 x 8mm Hex Screws | 2 | Kit | https://amzn.to/3L2rsOg |
| M2 x 5mm Flat Head Screws | 4 | Kit | https://amzn.to/44HXbLf |
| M3 Heat Set Inserts | 2 | Kit | https://amzn.to/44HXbLf |
| M2 Heat Set Inserts | 4 | Kit | https://amzn.to/49rPdbL |
| PolyMaker Matte Black PLA | ~50g (enclosure) | ~$2 | https://amzn.to/4atrUPS |
| PolyMaker Matte White PLA | ~5g (LED lens) | ~$0.20 | https://amzn.to/4qquieL |
| USB-C Cable & Adapter | 1 | ~$5-8 | https://amzn.to/3KO9BKZ |

**Total Cost:** ~$35 USD (extremely affordable!)

**Note:** LED lens MUST be white matte PLA for optimal light diffusion

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

### Important Assembly Notes

1. **ESP32-S3 Board Modification (REQUIRED)**
   - 5V-out soldering pad must be short-circuited
   - This powers the e-ink display and LED from USB 5V
   - Use soldering iron to bridge the two pads on back of board

2. **KY-040 Encoder Module Modification (May Be Required)**
   - Ribbon cable pins may be in wrong orientation
   - Check against GPIO pin table before connecting
   - May need to desolder and reverse ribbon cable connector

3. **3D Printing Enclosure**
   - M3 heat set inserts (2x) must be installed with soldering iron
   - M2 heat set inserts (4x) must be installed with soldering iron
   - Use PolyMaker Matte Black PLA for best finish

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

**Recommended:** Option 4 (authentic story + technical credibility) ✅ **Used in README.md**

**One-sentence version (used in README):**
> Open-source ESP32-S3 crypto price display with e-ink screen and encoder control—an affordable, customizable alternative to overpriced commercial tickers.

---

## 🎯 Next Actions

1. **Immediate:**
   - [x] Create tracking checklist ✅
   - [x] Create `hardware/3d-printed-case/` directory ✅
   - [x] Implement Phase 1 documentation improvements ✅

2. **User to provide (when available):**
   - [ ] Upload 3D printing STL files
   - [ ] Provide product photos/videos
   - [ ] Review and approve documentation

3. **Future enhancements:**
   - [ ] Create wiring diagram
   - [ ] Record assembly tutorial video
   - [ ] Expand troubleshooting guide
   - [ ] Add configuration guide
   - [ ] Add more example use cases

---

**Status:** Phase 1 complete ✅ Ready for launch
**Next Phase:** Add photos, STL files, and visual documentation
