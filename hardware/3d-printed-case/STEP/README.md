# STEP Files - CAD Source Files

This directory contains STEP format CAD files for users who want to modify the enclosure design.

---

## 📁 Files

### Complete Enclosure Assembly
- `CryptoBar Enclosure V1.step` - Complete enclosure assembly (24 MB)
  - Contains all parts: front case, back cover, knob, PCB strap, LED lens
  - All components in single file for easy editing

**Total:** 1 STEP file (24 MB)

**Note:** This is a complete assembly file. You can edit individual components within your CAD software and export them separately if needed.

---

## 🔧 Editing Instructions

### Compatible CAD Software
- **Fusion 360** (recommended for beginners)
- **FreeCAD** (open-source)
- **SolidWorks**
- **OnShape**
- **Inventor**
- Any CAD software supporting STEP format

### Modification Guidelines

1. **Import STEP File**
   - Open in your preferred CAD software
   - Verify units (millimeters)
   - Check all features imported correctly

2. **Make Your Modifications**
   - Resize for different displays
   - Add custom branding/text
   - Modify mounting holes
   - Adjust for different components

3. **Export to STL**
   - Mesh resolution: Medium to High
   - Format: Binary STL (smaller file size)
   - Verify no errors in mesh

4. **Test Print**
   - Print a test piece first
   - Verify dimensions with calipers
   - Check component fit before final print

---

## ⚠️ Important Notes

- **Preserve mounting points** for M3 and M2 heat set inserts
- **Maintain clearances** for ESP32-S3, display, encoder
- **LED lens optical properties** depend on material and finish - use matte white PLA for best results
- **Test fit** before printing multiple copies

---

## 📚 Resources

- [Main README](../README.md) - Project overview
- [Hardware Guide](../../docs/guides/HARDWARE_GUIDE.md) - Component specifications and assembly
- [STL Directory](../STL/) - Ready-to-print files

---

## 🤝 Contributing

If you create an improved design or modification:
1. Test thoroughly with actual hardware
2. Export clean STEP and STL files
3. Document your changes
4. Submit a pull request with photos

We welcome community improvements!
