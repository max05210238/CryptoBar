# Assembly Guide Images

This folder contains images for the [Assembly Guide](../../guides/ASSEMBLY_GUIDE.md).

## Required Images

Extract the following images from `docs/temp/CryptoBar組裝教學.pdf`:

### step1_esp32_soldered.jpg
- **PDF Page:** 1
- **Description:** ESP32-S3-DevKitC-1 N16R8 with pin headers soldered
- **Shows:** Completed ESP32-S3 module with both rows of pin headers soldered

### step2_5v_out_pad.jpg
- **PDF Page:** 2
- **Description:** 5V-OUT PAD location (circled in red)
- **Shows:** Close-up of the 5V-OUT solder pad near USB-C connector with red circle marking

### step5_encoder_modified.jpg
- **PDF Page:** 3
- **Description:** KY-040 rotary encoder with 90° pin headers removed
- **Shows:** Modified encoder with ribbon cable soldered to pads

### step5_encoder_mounting_pin.jpg
- **PDF Page:** 4
- **Description:** Encoder mounting pin (circled in red) that should be removed
- **Shows:** Close-up of center mounting pin on encoder shaft

### step6_led_cut.jpg
- **PDF Page:** 5
- **Description:** WS2812B LED cut from strip at the marked line
- **Shows:** LED strip with cut lines (scissors symbols) and single cut LED

### step7_led_arrow.jpg
- **PDF Page:** 6
- **Description:** LED arrow marking (circled in red) indicating data direction
- **Shows:** Close-up of WS2812B LED with arrow symbol highlighted

### step8_led_on_lens.jpg
- **PDF Page:** 7
- **Description:** WS2812B LED glued into white LED lens
- **Shows:** LED attached to Matte_White_LED_Lens_V1 with wires routed

### step9_m2_insert.jpg
- **PDF Page:** 8
- **Description:** M2 heat set insert installed flush in front case
- **Shows:** Brass heat set insert properly seated below surface

### step10_m3_insert.jpg
- **PDF Page:** 9
- **Description:** M3 heat set insert in back cover
- **Shows:** M3 insert installed in back cover mounting location

### step11_magnet.jpg
- **PDF Page:** 10
- **Description:** 4mm x 6mm magnet installed in enclosure cavity
- **Shows:** Small magnet pressed into front case cavity

### step12_esp32_mounted.jpg
- **PDF Page:** 11
- **Description:** ESP32-S3 secured to back cover with PCB strap
- **Shows:** Complete assembly with Matte_Black_PCB_Strap_V1 holding ESP32

### step13_led_lens_installed.jpg
- **PDF Page:** 12
- **Description:** White LED lens snapped into front case
- **Shows:** LED lens assembly installed in front case opening

### step14_display_installed.jpg
- **PDF Page:** 13
- **Description:** WaveShare 2.9" e-ink display mounted with M2 screws
- **Shows:** Display installed in front case with ribbon cable

### step15_encoder_installed.jpg
- **PDF Page:** 14
- **Description:** KY-040 encoder mounted with washer and nut
- **Shows:** Encoder installed through front case with correct orientation

### step17_wiring_complete.jpg
- **PDF Page:** 16
- **Description:** All components wired before final assembly
- **Shows:** Open enclosure with all wiring complete and visible

### step19_finished.jpg
- **PDF Page:** 17
- **Description:** Completed CryptoBar showing splash screen
- **Shows:** Professional product shot of finished CryptoBar

## Image Extraction Instructions

### Method 1: Using PDF Image Export Tool

1. Open `docs/temp/CryptoBar組裝教學.pdf` in a PDF reader
2. Use built-in image export or screenshot functionality
3. Save images with the exact filenames listed above
4. Recommended resolution: 800-1200px width (maintain aspect ratio)
5. Format: JPG with 85-90% quality

### Method 2: Using Online PDF Tools

1. Upload PDF to a PDF-to-images converter (e.g., pdf2jpg.net, smallpdf.com)
2. Download all pages as images
3. Crop each image to show only the relevant assembly step
4. Rename according to the list above
5. Optimize file sizes (target <500KB per image)

### Method 3: Using Command Line (Linux/Mac)

```bash
# Install pdftoppm (from poppler-utils)
sudo apt-get install poppler-utils  # Ubuntu/Debian
# or
brew install poppler  # macOS

# Convert PDF pages to images
pdftoppm -jpeg -r 150 docs/temp/CryptoBar組裝教學.pdf docs/images/assembly/page

# This creates page-01.jpg, page-02.jpg, etc.
# Then manually crop and rename each image
```

### Method 4: Using Python (Automated)

```python
# Requires: pip install pdf2image pillow
from pdf2image import convert_from_path
import os

pdf_path = 'docs/temp/CryptoBar組裝教學.pdf'
output_dir = 'docs/images/assembly/'

# Convert PDF to images
images = convert_from_path(pdf_path, dpi=150)

# Save each page
for i, image in enumerate(images, start=1):
    image.save(f'{output_dir}page-{i:02d}.jpg', 'JPEG', quality=85)

print(f"Extracted {len(images)} images to {output_dir}")
```

## Image Guidelines

- **Format:** JPG (preferred for photos) or PNG (if transparency needed)
- **Resolution:** 800-1200px width recommended
- **File Size:** Target <500KB per image for web performance
- **Quality:** Clear enough to see component details and markings
- **Cropping:** Frame the component being discussed, remove excess background
- **Orientation:** Landscape or portrait as appropriate for the component

## Current Status

- [x] ASSEMBLY_GUIDE.md updated with image references
- [ ] Images extracted from PDF
- [ ] Images optimized and renamed
- [ ] Images committed to repository

## Notes

Once all images are extracted and placed in this folder, the assembly guide will display them automatically. The guide currently contains placeholder references to these images with descriptive alt text.
