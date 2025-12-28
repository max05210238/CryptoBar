# CryptoBar 3D Printed Enclosure

This directory contains 3D printable files for the CryptoBar enclosure.

---

## 📁 Directory Structure

```
3d-printed-case/
├── STL/              # Ready-to-print STL files
│   └── README.md     # Printing instructions
├── STEP/             # Editable CAD source files (for customization)
│   └── README.md     # CAD editing guide
└── README.md         # This file
```

### File Organization

- **STL/** - Production-ready files for direct 3D printing (5 files, ~706 KB)
  - Front case, back cover, knob, PCB strap (black matte)
  - LED lens (white matte - CRITICAL!)
  - See [`STL/README.md`](STL/README.md) for complete file list and printing instructions

- **STEP/** - Source CAD files for users who want to modify the design (1 file, 24 MB)
  - Complete enclosure assembly with all components
  - Editable in Fusion 360, FreeCAD, SolidWorks, etc.
  - Customize for different displays, add branding, etc.
  - See [`STEP/README.md`](STEP/README.md) for editing guide

---

## 🖨️ Printing Recommendations

### Material Requirements

#### Main Enclosure (Black Matte)
- **Recommended:** [PolyMaker Matte Black PLA](https://amzn.to/4atrUPS)
- **Finish:** Matte preferred (professional appearance)
- **Alternative:** Any black PLA or PETG
- **Parts:** Base, cover, mounting components

#### LED Lens (White Matte - CRITICAL!)
- **Required:** [PolyMaker Matte White PLA](https://amzn.to/4qquieL)
- **Finish:** **Matte is MANDATORY** - glossy will NOT work properly!
- **Why Matte White:**
  - ✅ Optimal light diffusion from RGB LED
  - ✅ Even illumination without hot spots
  - ✅ Professional frosted appearance
  - ✅ Best optical reflection properties
  - ❌ Glossy white creates uneven lighting and harsh reflections
  - ❌ Other colors reduce LED brightness and color accuracy

**Important:** The LED lens is the ONLY part that must be printed in white. Using any other color or finish will significantly degrade the LED visibility and appearance!

### Print Settings

| Setting | Value | Notes |
|---------|-------|-------|
| **Layer Height** | 0.2mm | Standard quality, good balance |
| **Infill** | 20% | Sufficient strength for enclosure |
| **Supports** | As needed | Check STL orientation |
| **Wall Thickness** | 3-4 perimeters | For structural integrity |
| **Top/Bottom Layers** | 5 layers | Solid surfaces |
| **Print Speed** | 50-60mm/s | Standard speed |
| **Bed Temperature** | 60°C (PLA) | Standard PLA settings |

### Estimated Print Times

- **Main enclosure base:** ~2-3 hours
- **Cover/faceplate:** ~1-2 hours
- **LED lens:** ~15-30 minutes
- **Total:** ~3-4 hours (single printer)

---

## 🔧 Post-Processing

### Heat Set Insert Installation

**Required inserts:**
- 2x M3 brass heat set inserts
- 4x M2 brass heat set inserts

**Installation steps:**
1. Heat soldering iron to 200-220°C (PLA) or 240-260°C (PETG)
2. Place insert on designated hole
3. Gently push insert straight down as plastic melts
4. Ensure insert is flush with surface
5. Allow to cool for 1-2 minutes

**Pro tip:** Practice on a test piece first if you're new to heat set inserts!

---

## 🔧 Assembly Instructions

**See [HARDWARE_GUIDE](../../docs/guides/HARDWARE_GUIDE.md) for complete assembly instructions.**

**Quick assembly overview:**
1. Print all parts (black for enclosure, white matte for LED lens)
2. Install heat set inserts in base/cover
3. Test-fit ESP32-S3, display, encoder
4. Install components and secure with screws
5. Install LED lens for light diffusion

---

## 📝 Customization

### Using STEP Files

Want to modify the design? See [`STEP/README.md`](STEP/README.md) for:
- Compatible CAD software
- Editing guidelines
- Export instructions
- Design considerations

### Common Modifications

- **Different displays:** Resize display cutout
- **Custom branding:** Add text/logos to cover
- **Alternative mounts:** Add wall mount holes, desk stand
- **Color variations:** Print in any color (except LED lens - must be white matte!)

---

## 🎨 Color Combinations

### Standard Build
- Enclosure: Matte Black
- LED Lens: Matte White ✅

### Alternative Builds (Advanced)
- Enclosure: Any color you like
- LED Lens: **ALWAYS Matte White** (non-negotiable!)

---

## 📚 Additional Resources

- [Main README](../../README.md) - Project overview
- [Hardware Guide](../../docs/guides/HARDWARE_GUIDE.md) - Complete assembly instructions
- [STL Directory](STL/) - Ready-to-print files with detailed instructions
- [STEP Directory](STEP/) - CAD source files for customization

---

## 📄 License

Same as main project: MIT License

**Designer:** max05210238

---

## ⚠️ Important Reminders

1. **LED Lens MUST be white matte PLA** - This is not optional!
2. Use heat set inserts (not just screws into plastic)
3. Test-fit before final assembly
4. Follow print orientation in STL files
5. Check [STL/README.md](STL/README.md) for part-specific instructions

---

**Last Updated:** 2025-12-25
**Compatible with:** CryptoBar V0.99q and later
