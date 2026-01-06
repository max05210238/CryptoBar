# 🔧 CryptoBar Hardware Assembly Guide

Complete guide for building your CryptoBar cryptocurrency display device - from purchasing components to final assembly.

> **⚠️ Important:** Read through all sections before starting your build. This guide combines BOM planning with detailed step-by-step assembly instructions.

---

## 📖 Table of Contents

### Planning & Parts
1. [📦 Bill of Materials (BOM)](#-bill-of-materials-bom)
2. [💰 Cost Breakdown](#-cost-breakdown)
3. [🛒 Where to Buy Components](#-where-to-buy-components)
4. [🛠️ Required Tools](#️-required-tools)

### Assembly
5. [📦 Pre-Assembly Checklist](#-pre-assembly-checklist)
6. [🔌 GPIO Pin Connections](#-gpio-pin-connections)
7. [⚡ Step-by-Step Assembly Instructions](#-step-by-step-assembly-instructions)
   - Step 1: Verify ESP32-S3 Module and Solder Pin Headers
   - Step 2: Solder 5V-OUT PAD for LED Power
   - Step 3: Flash Firmware
   - Step 4: Verify E-ink Display Model
   - Step 5: Modify Rotary Encoder
   - Step 6: Cut WS2812B LED from Strip
   - Step 7: Solder LED Ribbon Cable
   - Step 8: Install LED into Lens
   - Step 9: Install M2 Heat Set Inserts (Front Case)
   - Step 10: Install M3 Heat Set Inserts (Back Cover)
   - Step 11: Install Magnets (OPTIONAL)
   - Step 12: Mount ESP32-S3 to Back Cover
   - Step 13: Install LED Lens to Front Case
   - Step 14: Install E-ink Display
   - Step 15: Install Rotary Encoder
   - Step 16: Complete All Wiring
   - Step 17: Test Before Final Assembly
   - Step 18: Install Knob
   - Step 19: Final Assembly

### Reference
8. [🔍 Troubleshooting](#-troubleshooting)
9. [🎨 Customization Ideas](#-customization-ideas)
10. [📚 Additional Resources](#-additional-resources)

---

## 📦 Bill of Materials (BOM)

### Core Electronics

| Item | Description | Qty | Est. Cost | Amazon Link |
|------|-------------|-----|-----------|-------------|
| ESP32-S3 Dev Board | N16R8 (16MB Flash, 8MB PSRAM) | 1 | $10-15 | [Buy](https://amzn.to/4952HZK) |
| E-ink Display | WaveShare 2.9" B&W e-Paper Module **V2** | 1 | $20-25 | [Buy](https://amzn.to/4pdoJj1) |
| Rotary Encoder | KY-040 Module with Ribbon Cable Kit | 1 | $8-10 | [Buy](https://amzn.to/3Lip1Hm) |
| RGB LED | WS2812B LED Strip (only need 1 LED) | 1 | $8-12 | [Buy](https://amzn.to/3YPYPXD) |

### Hardware & Fasteners

| Item | Description | Qty Needed | Kit Size | Amazon Link |
|------|-------------|------------|----------|-------------|
| M3 Hex Screws | M3 x 8mm | 2 | Multi-pack | [Buy Kit](https://amzn.to/3L2rsOg) |
| M2 Flat Screws | M2 x 5mm Flat Head | 4 | Multi-pack | [Buy Kit](https://amzn.to/44HXbLf) |
| M3 Heat Set Inserts | Brass, M3 size | 2 | Multi-pack | [Buy](https://amzn.to/44HXbLf) |
| M2 Heat Set Inserts | Brass, M2 size | 4 | Multi-pack | [Buy](https://amzn.to/49rPdbL) |

### Optional Components

| Item | Description | Qty Needed | Use Case | Amazon Link |
|------|-------------|------------|----------|-------------|
| Small Magnets | 4mm x 6mm | 4 per unit | **Only needed for stacking multiple units** - allows secure vertical stacking | [Buy](https://amzn.to/4sjNFIy) |

**Note:** Magnets are embedded in enclosure and enable multiple CryptoBars to stack cleanly. Skip if building only one unit.

### 3D Printing Materials

| Component | Recommended Product | Amazon Link |
|-----------|---------------------|-------------|
| **Black PLA Filament** | PolyMaker Matte Black PLA (enclosure parts) | [Buy on Amazon](https://amzn.to/4atrUPS) |
| **White PLA Filament** | PolyMaker Matte White PLA (LED lens - REQUIRED) | [Buy on Amazon](https://amzn.to/4qquieL) |

- **Files:** Available in [`hardware/3d-printed-case/`](../../hardware/3d-printed-case/)
- **Enclosure:** Matte Black PLA recommended (or any color)
- **LED Lens:** **Matte White PLA REQUIRED** for optimal light diffusion
- **Print Time:** ~3-4 hours total
- **Cost:** ~$2 USD material cost (if you have a 3D printer)
- **Post-Processing:** Heat set insert installation required (soldering iron)

### Power Supply

| Component | Description | Amazon Link |
|-----------|-------------|-------------|
| **USB-C Cable & Adapter** | USB-C cable + power adapter kit | [Buy on Amazon](https://amzn.to/3KO9BKZ) |

- **Input:** 5V via USB-C, 1A minimum
- **Power Consumption:** ~0.5W average (e-ink refresh), <0.1W standby

---

## 💰 Cost Breakdown

### Complete Build Cost (with own 3D printer)
- **ESP32-S3 N16R8:** ~$6 USD
- **WaveShare 2.9" E-ink Display:** ~$27 USD
- **KY-040 Rotary Encoder:** ~$2 USD
- **WS2812B LED:** ~$2 USD (strip, need 1 LED)
- **M3/M2 Screws & Heat Set Inserts:** ~$0.50 USD (from kits)
- **3D Printing Material:** ~$2 USD (PLA)
- **USB-C Cable & Adapter:** ~$5-8 USD (if needed)

**Total Estimated Cost:** **~$35 USD** (extremely affordable!)

**Optional Add-ons:**
- **Small Magnets (4mm x 6mm):** ~$0.50 USD (only if stacking multiple units)

**Notes:**
- Hardware kits (screws, inserts, magnets) come in multi-packs - consider building multiple units with friends to maximize value!
- Shipping costs vary by location
- If using 3D printing service instead of own printer: add ~$5-10 USD
- **Many components come in multi-packs** - building 2-3 units together significantly reduces per-unit cost

---

## 🛒 Where to Buy Components

All components are readily available via the Amazon links provided in the BOM table above.

**Recommended Sources:**
- **Electronics:** Amazon (affiliate links provided in BOM)
- **3D Printing Filament:** [PolyMaker Matte Black PLA](https://amzn.to/4atrUPS) for best finish
- **USB-C Cable & Adapter:** [USB-C kit](https://amzn.to/3KO9BKZ)
- **Alternative Sources:** AliExpress, local electronics suppliers (longer shipping, lower cost)

---

## 🛠️ Required Tools

| Tool | Purpose | Est. Cost |
|------|---------|-----------|
| **Soldering Iron** | Heat set insert installation, ESP32-S3 5V-out mod, LED wiring | $20-50 |
| **Solder Wire** | Lead-free or 63/37 tin-lead | $5-10 |
| **Wire Cutters** | Flush cutters preferred for LED and encoder prep | $10-15 |
| **Heat Set Insert Tip** | Or use soldering iron tip | Optional |
| **Small Phillips Screwdriver** | Assembly screws | $5-10 |
| **Tweezers** | Small component handling | $5-10 |
| **Super Glue or Hot Glue Gun** | LED installation | $5-10 |
| **Multimeter** | Testing connections (optional but recommended) | $15-30 |
| **USB-C Cable** | For programming and power | Included |
| **Computer with VS Code** | For firmware flashing | Required |

---

## 📦 Pre-Assembly Checklist

Before starting, verify you have all components:

**Electronics:**
- [ ] ESP32-S3-DevKitC-1 **N16R8** (16MB Flash, 8MB PSRAM)
- [ ] WaveShare 2.9" B&W E-ink display (with **V2 sticker**)
- [ ] KY-040 rotary encoder module
- [ ] WS2812B addressable RGB LED (single LED or strip)
- [ ] Ribbon cable (for LED and encoder wiring)
- [ ] Pin headers (if not pre-soldered on ESP32)

**3D Printed Parts:**
- [ ] Matte_Black_Front_Case_V1
- [ ] Matte_Black_Back_Cover_V1
- [ ] Matte_White_LED_Lens_V1
- [ ] Matte_Black_PCB_Strap_V1
- [ ] Matte_Black_Knob_V1

**Hardware:**
- [ ] M2 heat set inserts (4x) - for back cover attachment
- [ ] M2 x 5mm flat head screws (4x) - for final assembly (attaching back cover to front case)
- [ ] M3 heat set inserts (2x) - for ESP32 mounting
- [ ] M3 x 8mm hex screws (2x) - for ESP32 mounting
- [ ] Small magnets 4mm x 6mm (4x) - **OPTIONAL** if stacking multiple units

**Note:** E-ink display uses its own screws (included with the display). The M2 x 5mm screws listed above are for attaching the back cover, NOT for the display.

---

## 🔌 GPIO Pin Connections

### Complete Pin Assignment Table

| Component | Component Pin | ESP32-S3 Pin | Wire Color (typical) | Notes |
|-----------|---------------|--------------|----------------------|-------|
| **E-ink Display (SPI)** |
| | VCC | 3V3 | Red | 3.3V power |
| | GND | GND | Black | Ground |
| | DIN (MOSI) | GPIO 11 | Yellow | SPI data |
| | CLK (SCK) | GPIO 12 | White | SPI clock |
| | CS | GPIO 10 | Orange/Yellow | Chip select |
| | DC | GPIO 17 | Green | Data/Command |
| | RST | GPIO 16 | Blue | Reset |
| | BUSY | GPIO 4 | Purple/Gray | Busy signal |
| **Rotary Encoder (KY-040)** |
| | + (VCC) | 3V3 | Red | 3.3V power |
| | GND | GND | Black | Ground |
| | SW | GPIO 21 | Blue | Push button (active low) |
| | DT (Phase B) | GPIO 1 | Green | Encoder B (PCNT) |
| | CLK (Phase A) | GPIO 2 | White | Encoder A (PCNT) |
| **WS2812B LED** |
| | VCC | 5V | Red | 5V power (requires 5V-OUT mod!) |
| | DIN (Data In) | GPIO 15 | Green/White | NeoPixel data |
| | GND | GND | Black | Ground |

### Important Notes

1. **ESP32-S3 PCNT Compatibility**
   - Encoder MUST use GPIO 1 and 2 (PCNT-compatible pins)
   - GPIO 5/6 do NOT support PCNT on ESP32-S3 (common mistake!)
   - **GPIO 2/1 specifically chosen** for ESP32-S3 PCNT (Pulse Counter) peripheral support

2. **5V Power Rail Requirement**
   - WS2812B LED requires 5V power
   - ESP32-S3 dev boards may NOT have 5V-out by default
   - **Required modification:** Short the 5V-OUT solder jumper on the back (see Step 2)
   - This enables 5V output on the pin header from USB power

3. **Display Version Critical**
   - Only **WaveShare 2.9" B&W V2** display is supported
   - Look for **V2 sticker** on the back of the display
   - V1 or other versions will NOT work (different driver IC)

---

## ⚡ Step-by-Step Assembly Instructions

### Step 1: Verify ESP32-S3 Module and Solder Pin Headers

**⚠️ CRITICAL:** Ensure you have the **ESP32-S3-DevKitC-1 N16R8** version (16MB Flash + 8MB PSRAM). The N8R8 version is unstable and not supported.

**How to verify:**
- Check the module label on the ESP32-S3 chip
- Look for "N16R8" marking on the silver shield

**Pin header soldering:**
1. If pin headers are not pre-soldered, insert them into the ESP32-S3 board
2. Use a breadboard or helping hands to keep headers perpendicular
3. Solder all pins on both sides (2x19 pins = 38 total)
4. Ensure good solder joints with smooth, shiny finish

![ESP32-S3 with soldered pin headers](../images/assembly/step1_esp32_soldered.jpg)
*ESP32-S3-DevKitC-1 N16R8 with pin headers soldered*

---

### Step 2: Solder 5V-OUT PAD for LED Power

**⚠️ IMPORTANT:** The WS2812B LED requires 5V power. The ESP32-S3 DevKit has a solder pad labeled **5V-OUT** or **IN-OUT** on the back that must be bridged.

**Location:** Bottom side of the ESP32-S3 DevKit board, near the USB-C connector

**Procedure:**
1. Locate the **5V-OUT** solder pad (two small pads close together)
2. Apply flux to both pads
3. Heat soldering iron to 350°C
4. Bridge the two pads with a small amount of solder
5. Verify continuity between the 5V pin and USB 5V with multimeter

**Why this is needed:** By default, the 5V pin is not connected to USB power. Bridging this pad enables 5V output on the pin header.

![5V-OUT PAD location on ESP32-S3](../images/assembly/step2_5v_out_pad.jpg)
*5V-OUT PAD location (circled in red) near the USB-C connector on the back of the ESP32-S3 board*

---

### Step 3: Flash Firmware

Before proceeding with mechanical assembly, flash the firmware to verify the ESP32-S3 is working correctly.

**Requirements:**
- Visual Studio Code with PlatformIO extension
- Git (to clone repository)
- USB-C cable

**Procedure:**

1. **Clone the repository:**
   ```bash
   git clone https://github.com/max05210238/CryptoBar.git
   cd CryptoBar
   ```

2. **Open in Visual Studio Code:**
   ```bash
   code .
   ```

3. **Install PlatformIO extension** if not already installed

4. **Build and upload:**
   - Click PlatformIO icon in left sidebar
   - Select "Upload" under esp32-s3-devkitc-1 environment
   - Wait for compilation and upload to complete

5. **Verify successful flash:**
   - Open Serial Monitor (115200 baud)
   - You should see boot messages and "CryptoBar" initialization logs

> **Note:** Detailed firmware flashing instructions are available in the [Developer Guide](DEVELOPER_GUIDE.md#build--flash-instructions).

---

### Step 4: Verify E-ink Display Model

**⚠️ CRITICAL:** Only the **WaveShare 2.9" B&W V2** display is supported. Using V1 or incompatible displays will not work.

**How to verify:**
- Look for a **V2 sticker** on the back
- The V2 model has a different driver IC than V1

**If you have the wrong version:**
- Purchase the correct V2 display from [Amazon link in BOM](#-bill-of-materials-bom)

---

### Step 5: Modify Rotary Encoder

The KY-040 rotary encoder module needs modification to fit inside the enclosure.

**Required modifications:**

1. **Remove 90° pin headers:**
   - Removing the pin holder cleanly requires good soldering skills. If you are not certain you can do it, you can cut it off
   - Use flush cutters to clip off the right-angle pin headers
   - Clean up the pads with solder wick if needed
   - You will solder wires directly to these pads later

2. **Remove center mounting pin (REQUIRED):**
   - The encoder has a small metal pin at the center for PCB mounting
   - This pin prevents the encoder from sitting flush in the enclosure
   - Use wire cutters to carefully clip this pin as close to the base as possible
   - File smooth if needed

**⚠️ Be careful not to damage the encoder mechanism while modifying.**

![Modified rotary encoder](../images/assembly/step5_encoder_modified.jpg)
*KY-040 rotary encoder with 90° pin headers removed and ribbon cable soldered*

![Encoder mounting pin removal](../images/assembly/step5_encoder_mounting_pin.jpg)
*Center mounting pin (circled in red) should be removed for flush installation*

---

### Step 6: Cut WS2812B LED from Strip

If using a WS2812B LED strip, you need to cut a single LED.

**Procedure:**
1. Identify the cut lines on the LED strip (usually marked with scissors symbol)
2. Use flush cutters to cut exactly on the cut line
3. You should have one LED with 3 solder pads on each side (VCC, DATA, GND)

**If using individual WS2812B LED:**
- Skip this step

![Cutting LED from strip](../images/assembly/step6_led_cut.jpg)
*Cut WS2812B LED at the marked cut line (scissors symbol)*

---

### Step 7: Solder LED Ribbon Cable

**⚠️ CRITICAL:** Pay attention to the **arrow direction** on the WS2812B LED. Data flows in only one direction.

**LED Orientation:**
- The LED has a small arrow (△) indicating data direction
- Arrow should point **AWAY** from the data input pad
- Typical pad labels: **5V / DIN / GND** or **VCC / DATA / GND**

**Procedure:**

1. **Prepare ribbon cable:**
   - Cut 3 wires from ribbon cable, approximately 10-15cm length
   - Strip 2-3mm from one end only, keep the female Dupont end (that will be used for plugging into the ESP32)

2. **Solder to LED:**
   - **Power wire** → 5V/VCC pad
   - **Data wire** → DIN/DATA pad (input side, where arrow points FROM)
   - **Ground wire** → GND pad

3. **Test with multimeter:**
   - Verify no shorts between adjacent pads

![LED arrow direction](../images/assembly/step7_led_arrow.jpg)
*Pay attention to the arrow marking (circled in red) - solder wires to the INPUT side where the arrow points FROM*

---

### Step 8: Install LED into Lens

The LED must be secured inside the **Matte_White_LED_Lens_V1** part.

**Procedure:**

1. **Test fit first:**
   - Insert LED into the lens cavity (square recess)
   - LED will fit in the square recess, it might not be flush with the surface, that is normal

2. **Apply adhesive:**
   - Use a small drop of super glue (cyanoacrylate) **OR**
   - Use hot glue gun for repositionable bond

3. **Position LED:**
   - Center the LED in the lens cavity
   - Ensure the LED faces **into the lens**
   - **⚠️ The cable direction is CRITICAL** - please make sure it's the same as shown in the photo

4. **Allow to cure:**
   - Super glue: 30-60 seconds
   - Hot glue: 2-3 minutes

**⚠️ Do not use too much glue** - excess glue can seep into the LED or block light.

![LED installed in lens](../images/assembly/step8_led_on_lens.jpg)
*WS2812B LED glued into the white LED lens with wires routed the same direction shown here*

---

### Step 9: Install M2 Heat Set Inserts (Front Case)

The **Matte_Black_Front_Case_V1** requires 4x M2 heat set inserts for attaching the back cover.

**Tools needed:**
- Soldering iron with clean tip (or dedicated heat set insert tip)
- Temperature: 200-220°C for PLA, 240-260°C for PETG/ABS

**Procedure:**

1. **Identify insert locations:**
   - There are 4 holes around the opening of the front case
   - Located at the corners where the back cover will mount

2. **Heat the insert:**
   - Hold M2 heat set insert with needle-nose pliers or tweezers
   - Touch with hot soldering iron tip for 2-3 seconds

3. **Insert into plastic:**
   - Align insert with hole (use perpendicular approach)
   - Apply gentle, even pressure
   - Insert should sink until flush with surface
   - **Do not over-insert** - stop when flush

4. **Allow to cool:**
   - Wait 30 seconds before handling
   - Verify insert is secure and flush

5. **Repeat for all 4 inserts**

**⚠️ Common mistakes:**
- Inserting crooked (ruins the hole - may need to re-print)
- Inserting too fast (melts too much plastic)
- Temperature too high (warps the part)

![M2 heat set insert installation](../images/assembly/step9_m2_insert.jpg)
*M2 heat set insert installed flush in the front case - insert must be below the surface*

---

### Step 10: Install M3 Heat Set Inserts (Back Cover)

The **Matte_Black_Back_Cover_V1** requires 2x M3 heat set inserts for attaching Matte_Black_PCB_Strap_V1.

**Procedure:**

1. **Identify insert locations:**
   - There are 2 holes on the back cover
   - Located on each side where the ESP32 will be installed

2. **Follow same procedure as Step 9:**
   - Heat M3 insert with soldering iron (200-260°C depending on material)
   - Align carefully and apply gentle pressure
   - Insert until flush with surface

3. **Verify alignment:**
   - Look from the side to ensure insert is perpendicular
   - If crooked, reheat and adjust before it cools

![M3 heat set insert installation](../images/assembly/step10_m3_insert.jpg)
*M3 heat set insert installed in the back cover for final assembly screws*

---

### Step 11: Install Magnets (OPTIONAL - For Stacking Only)

**⚠️ This step is OPTIONAL.** Only perform if you plan to **stack multiple CryptoBar units vertically**.

**Components:**
- 4x small magnets (4mm x 6mm)

**Magnet locations:**
- Front case top (inside): 2 magnet cavities
- Front case bottom (inside): 2 magnet cavities

**Procedure:**

1. **Test polarity first:**
   - Take 4 magnets and let them snap together
   - Pick one magnet on the end and mark the face with permanent marker
   - **Critical:** Make sure that the marked side is always facing up when insert

2. **Install in front case:**
   - If the fit is loose, apply a small drop of super glue in magnet cavity
   - Insert magnet with correct pole facing up
   - Repeat for all 4 magnets (2 on top, 2 on bottom)
   - **⚠️ Be very careful about the pole orientation - magnets are not easy to remove once installed**

3. **Allow to cure:**
   - Wait 5 minutes before handling

**Polarity check:**
- Hold front case near another front case
- They should snap together with magnetic attraction
- If they repel, you installed magnets with wrong polarity (need to re-do)

![Magnet installation](../images/assembly/step11_magnet.jpg)
*Small 4mm x 6mm magnet installed in the front case cavity (tight friction fit or with glue)*

---

### Step 12: Mount ESP32-S3 to Back Cover

**Components needed:**
- ESP32-S3 module (with pin headers soldered from Step 1)
- Matte_Black_Back_Cover_V1 (with M3 inserts installed from Step 10)
- Matte_Black_PCB_Strap_V1 (3D printed part)
- 2x M3 x 8mm hex screws

**Procedure:**

1. **Position ESP32-S3 in back cover:**
   - Place ESP32-S3 with **component side facing DOWN** (pin headers pointing UP)
   - Pin headers should point upward (toward the front)

2. **Install PCB strap:**
   - Place the PCB_Strap over the ESP32-S3 pins
   - Strap should press the ESP32-S3 firmly against the back cover

3. **Secure with screws:**
   - Insert 2x M3 x 8mm hex screws through the strap into the M3 heat set inserts
   - **⚠️ CRITICAL:** Tighten screws VERY GENTLY - just slightly snug
   - **Do NOT fully tighten** - this will warp and damage the ESP32-S3 PCB
   - It's normal to have a large gap between the strap and back case
   - After tightening, check from the side to ensure PCB is NOT bent or warped

4. **Verify installation:**
   - ESP32-S3 should be held securely but NOT bent
   - Pin headers should be exposed and accessible (a few pins will be covered, but those are not used)
   - PCB should be completely flat (no warping)

![ESP32-S3 mounted to back cover](../images/assembly/step12_esp32_mounted.jpg)
*ESP32-S3 secured to back cover with PCB strap and M3 screws (lightly tightened only)*

---

### Step 13: Install LED Lens to Front Case

The **Matte_White_LED_Lens_V1** (with LED installed from Step 8) will be a tight fit into the front case.

**Procedure:**

1. **Align lens:**
   - The lens has alignment features that match the front case
   - Position the lens over the light window opening

2. **Press to snap:**
   - Apply firm, even pressure
   - You should hear/feel a click when fully seated
   - Lens should be flush with front case surface

3. **Secure with glue:**
   - Apply some CA glue between the seams of the lens and front case to hold it together

4. **Verify installation:**
   - Lens should not wobble or come loose
   - LED should be visible through the lens
   - Wires should have enough slack (8-10cm)

![LED lens installed in front case](../images/assembly/step13_led_lens_installed.jpg)
*White LED lens with WS2812B LED snapped into the front case*

---

### Step 14: Install E-ink Display

**Components:**
- WaveShare 2.9" B&W V2 e-ink display
- WaveShare cable for screen (it should come with the display)

**Procedure:**

1. **Remove screws from display:**
   - Remove the 4 small screws from the display PCB
   - **Keep these screws** - you will use them to mount the display
   - Remove the small standoffs (copper pillars)
   - **Discard the standoffs** - they are not needed

2. **Remove protective film:**
   - Peel off the protective film from the display

3. **Install screen cable:**
   - The connector is just a plug-in type, no lock
   - Plug one end of the cable into the display's connector
   - The other side has female Dupont connectors for connecting to the ESP32 (will be connected in Step 16)

4. **Position display in front case:**
   - Place display into the **Matte_Black_Front_Case_V1**
   - **Orientation:** The end WITHOUT the ribbon cable should face toward the LED lens

5. **Screw down display:**
   - Use the 4 small screws you removed in step 14-1
   - **⚠️ CRITICAL:** You are screwing directly into the 3D printed plastic (NOT into heat set inserts)
   - Start all 4 screws by hand (do not tighten)
   - Once all 4 are started, tighten VERY GENTLY in a cross pattern:
     - Top-left → Bottom-right → Top-right → Bottom-left
   - **⚠️ Do not over-tighten** - just slightly snug is enough
   - Over-tightening will strip the plastic threads

6. **Verify display:**
   - Display should sit flat against the front case
   - No gaps or warping
   - Ribbon cable should have enough slack to reach the ESP32

![E-ink display installed](../images/assembly/step14_display_installed.jpg)
*WaveShare 2.9" e-ink display mounted in the front case with its original screws screwed directly into plastic*

---

### Step 15: Install Rotary Encoder

**Components:**
- Modified KY-040 rotary encoder (from Step 5)

**Procedure:**

1. **Insert encoder through front case:**
   - The encoder shaft goes through the round hole in the front case
   - Encoder body sits inside the case
   - **⚠️ Make sure the PCB is rotated to the right location - see photo**

2. **Secure with nut:**
   - The encoder comes with a metal washer and hex nut
   - Place washer on the shaft (outside the case)
   - Thread the hex nut onto the shaft
   - Can use pliers, but please be gentle

**⚠️ Do not over-tighten the nut** - it can crack the plastic case or damage the encoder threads.

![Rotary encoder installed](../images/assembly/step15_encoder_installed.jpg)
*KY-040 rotary encoder mounted through the front case with washer and nut*

---

### Step 16: Complete All Wiring

**⚠️ CRITICAL STEP:** Incorrect wiring can damage components. Double-check all connections before powering on.

Refer to the [GPIO Pin Connections table](#-gpio-pin-connections) for the complete wiring reference.

**Wiring Procedure:**

All wires have Dupont connectors and can be plugged directly into ESP32-S3 pin headers.

1. **Connect encoder wires to ESP32-S3:**
   - Plug encoder wires into the correct ESP32-S3 pins according to the table
   - **Double-check the pin assignment before connecting**
   - Encoder + → ESP32 3V3
   - Encoder GND → ESP32 GND
   - Encoder SW → ESP32 GPIO 21
   - Encoder DT → ESP32 GPIO 1
   - Encoder CLK → ESP32 GPIO 2

2. **Connect display cable to ESP32-S3:**
   - The display cable was already connected to the display module in Step 14-3
   - Plug each Dupont connector into the correct ESP32-S3 pin according to the table
   - Display VCC → ESP32 3V3
   - Display GND → ESP32 GND
   - Display DIN (MOSI) → ESP32 GPIO 11
   - Display CLK (SCK) → ESP32 GPIO 12
   - Display CS → ESP32 GPIO 10
   - Display DC → ESP32 GPIO 17
   - Display RST → ESP32 GPIO 16
   - Display BUSY → ESP32 GPIO 4

3. **Connect LED wires to ESP32-S3:**
   - Plug LED wires into the correct ESP32-S3 pins
   - LED VCC → ESP32 5V pin
   - LED DIN → ESP32 GPIO 15
   - LED GND → ESP32 GND

4. **Verify all connections:**
   - Double-check each connection against the pin assignment table
   - Ensure all Dupont connectors are firmly seated on the pins
   - Verify no adjacent pins are accidentally bridged

**⚠️ Common wiring mistakes:**
- Swapping CLK and DT on encoder (encoder will count backward)
- Connecting LED to 3.3V instead of 5V (LED won't light)
- Forgetting to bridge 5V-OUT pad (LED has no power)

---

### Step 17: Test Before Final Assembly

**⚠️ CRITICAL:** Test the device before closing the enclosure. Debugging is much harder after assembly.

**Procedure:**

1. **Visual inspection:**
   - Check all Dupont connectors are firmly seated
   - Verify no loose connections
   - Ensure no adjacent pins are accidentally bridged

2. **Power on test:**
   - Connect USB-C cable to ESP32-S3
   - Device should boot (check Serial Monitor at 115200 baud)
   - You should see boot logs and "CryptoBar" initialization

3. **Display test:**
   - Display should show the CryptoBar splash screen
   - If display shows nothing: check wiring and ribbon cable
   - If display shows garbage: likely wrong display version (need V2)

4. **LED test:**
   - LED should light up during boot (it will light up blue)
   - If LED doesn't light:
     - Check 5V-OUT pad is bridged
     - Check GPIO 15 connection
     - Verify LED polarity (arrow direction)
     - **⚠️ Make sure the soldering is not broken - these wires are thin and will break if rotated too many times**

5. **Encoder test:**
   - Rotate encoder clockwise/counter-clockwise
   - Display should respond (menu navigation)
   - Press encoder button
   - Display should respond (enter/select action)

6. **Full function test:**
   - Configure WiFi (if not already done)
   - Verify price display updates
   - Test menu navigation
   - Test all display modes

**If any test fails, DO NOT proceed to final assembly.** Refer to [Troubleshooting](#-troubleshooting) section.

![Wiring complete before closure](../images/assembly/step17_wiring_complete.jpg)
*All components wired and tested before final enclosure assembly - verify LED lights up, display works, and encoder responds*

---

### Step 18: Install Knob

**Components:**
- Matte_Black_Knob_V1 (3D printed part)

**Procedure:**

1. **Align knob with encoder shaft:**
   - The knob has a round hole (does not have a D-shaped hole)
   - Our knob does not have set screws

2. **Press knob onto shaft:**
   - Apply firm, even pressure
   - Knob should slide down until it bottoms out on the encoder nut
   - **Do not force** - if too tight, sand the inside of the knob slightly

3. **Secure knob:**
   - The knob should be a tight fit
   - If too loose, apply a very tiny dot of CA glue

4. **Test rotation:**
   - Rotate knob smoothly
   - Should have tactile clicks (from encoder detents)
   - Knob should not wobble or come loose

---

### Step 19: Final Assembly

**Final assembly:**

1. **Align front case and back cover:**
   - All wires should be neatly routed inside
   - No wires should be pinched

2. **Close enclosure:**
   - Gently press front and back together
   - Make sure the back cover sits flush in the front case

3. **Install M2 screws:**
   - Use 4x M2 x 5mm flat head screws
   - Screw into the M2 heat set inserts (from Step 9)
   - Tighten with hex driver (do not over-tighten)

4. **Final inspection:**
   - All parts should be flush
   - No gaps in the enclosure
   - USB-C port accessible
   - Knob rotates smoothly

**🎉 Your CryptoBar is now complete! 🎉**

![Finished CryptoBar](../images/assembly/step19_finished.jpg)
*Completed CryptoBar showing splash screen - ready to configure and use!*

**Next steps:**
- Configure WiFi (device creates "CryptoBar-XXXXXX" AP on first boot)
- [Customize display settings](DISPLAY_GUIDE.md)
- [Set up OTA updates](OTA_UPDATE_GUIDE.md)
- [Explore configuration options](SETTINGS_MENU_GUIDE.md)

---

## 🔍 Troubleshooting

### Display Issues

**Display shows nothing:**
- ✅ Check ribbon cable is fully inserted and locked
- ✅ Verify all 8 display connections (especially CS, DC, RST, BUSY)
- ✅ Check 3.3V power to display
- ✅ Verify 5V-out jumper is bridged on ESP32-S3 (even though display uses 3.3V, check anyway)
- ✅ Test with serial monitor - look for display init messages

**Display shows garbage/random pixels:**
- ✅ Wrong display version (need WaveShare V2, not V1)
- ✅ Incorrect wiring (verify all 8 pins against GPIO table)
- ✅ Bad solder joint on CS/DC/RST pins
- ✅ Verify SPI pins (MOSI=GPIO11, SCK=GPIO12)

**Display is very slow:**
- ✅ Normal behavior for e-ink displays
- ✅ Full refresh takes 2-3 seconds
- ✅ Partial refresh takes 0.5-1 second

**Display ghosting:**
- ✅ Normal for e-ink - use full refresh mode
- ✅ Try full refresh instead of partial in settings

### LED Issues

**LED doesn't light up:**
- ✅ **Critical:** Check 5V-OUT pad is bridged on ESP32
- ✅ **Critical:** Verify LED connected to 5V (not 3.3V)
- ✅ Verify GPIO 15 connection
- ✅ Check LED polarity (arrow direction matters)
- ✅ Test with multimeter: 5V pin should measure 5V when USB connected
- ✅ Test with LED brightness setting (not set to Low or 0)
- ✅ Verify WS2812B polarity (check LED markings)

**LED wrong color:**
- ✅ Normal - LED color is software controlled
- ✅ Default boot color may vary by firmware version
- ✅ Check firmware version (V0.99h+ required for party mode)
- ✅ Verify RGB order (some strips use GRB instead of RGB)

**LED flickers:**
- ✅ Loose connection on data or ground wire
- ✅ Check solder joints
- ✅ Verify data wire not damaged

### Encoder Issues

**Encoder doesn't respond:**
- ✅ **Critical:** Verify using GPIO 1/2 (not GPIO 5/6!)
- ✅ Check all 5 wires (VCC, GND, SW, DT, CLK)
- ✅ Verify GPIO 1, 2, 21 connections
- ✅ Check 3.3V power to encoder module
- ✅ Test button press (GPIO 21 should go LOW when pressed)
- ✅ Test with serial monitor debug output
- ✅ Verify ribbon cable orientation

**Encoder counts backward:**
- ✅ CLK and DT wires are swapped
- ✅ Swap GPIO 1 and GPIO 2 connections
- ✅ Or modify `ENC_DIR_INVERT` in code

**Encoder skips counts:**
- ✅ Loose connection on CLK or DT
- ✅ Bad solder joint
- ✅ Damaged encoder (replace)
- ✅ Adjust `ENC_COUNTS_PER_DETENT` in encoder_pcnt.cpp

**Encoder very sensitive / jumpy:**
- ✅ Adjust `ENC_COUNTS_PER_DETENT` in encoder_pcnt.cpp
- ✅ Increase `ENC_DIR_LOCK_MS` for more filtering

### Boot Issues

**ESP32 doesn't boot:**
- ✅ Check USB cable (must be data cable, not charge-only)
- ✅ Verify 3.3V power LED on ESP32 is lit
- ✅ Check Serial Monitor for error messages

**Firmware upload fails:**
- ✅ Wrong USB driver (install CP210x or CH340 driver)
- ✅ Wrong board selected in PlatformIO
- ✅ Boot mode issue: Hold BOOT button during upload

### WiFi Issues

**Can't find CryptoBar access point:**
- ✅ Long-press encoder for 12 seconds to force WiFi portal
- ✅ Check WiFi scanning on phone (2.4GHz only)
- ✅ Move closer to device

**Can't connect to home WiFi:**
- ✅ Verify WiFi credentials (case-sensitive)
- ✅ Check 2.4GHz network (ESP32 doesn't support 5GHz)
- ✅ Verify router compatibility (WPA2 recommended)

### API / Price Fetch Issues

**Yellow LED / "API Failure" message:**
- ✅ Verify internet connection (ping test)
- ✅ Check firewall settings
- ✅ Wait 30-60 seconds for automatic retry
- ✅ CoinGecko may be rate-limited (use 3min+ update interval)

**Price not updating:**
- ✅ Check update interval setting (3min recommended)
- ✅ Verify time is synchronized (check clock display)
- ✅ Review serial monitor for API error messages

### Assembly Issues

**Heat set inserts crooked:**
- ✅ Reheat with soldering iron and straighten
- ✅ If too damaged, fill hole with epoxy and re-drill
- ✅ Last resort: re-print the part

**Screws won't thread:**
- ✅ Insert may have melted plastic inside threads
- ✅ Use screw to "chase" the threads (screw in/out several times)
- ✅ If stripped: use larger screw or add threadlocker

**Parts don't fit:**
- ✅ 3D printer tolerances vary
- ✅ Sand/file parts slightly for better fit
- ✅ Check print quality (no warping or elephant's foot)

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
- **Custom Colors**: Print enclosure in different colors

---

## 📚 Additional Resources

- **[Main README](../../README.md)** - Project overview and quick start
- **[Display Guide](DISPLAY_GUIDE.md)** - UI layout, navigation, screen elements explained
- **[LED Display Guide](LED_DISPLAY_GUIDE.md)** - LED colors, animations, troubleshooting
- **[Settings Menu Guide](SETTINGS_MENU_GUIDE.md)** - Configuration options explained
- **[OTA Update Guide](OTA_UPDATE_GUIDE.md)** - Wireless firmware updates
- **[Developer Guide](DEVELOPER_GUIDE.md)** - Code architecture and development
- **[Release Notes](../release-notes/)** - Technical version details
- **[3D Printing Guide](../../hardware/3d-printed-case/README.md)** - STL files and print settings

---

## ❓ Need Help?

- 🐛 [Report hardware issues](https://github.com/max05210238/CryptoBar/issues)
- 💬 [Ask questions](https://github.com/max05210238/CryptoBar/discussions)

---

**Last Updated:** 2025-12-27
**Hardware Version:** V0.99q compatible
**Estimated Build Time:** 2-4 hours (including 3D printing)
**Estimated Cost:** ~$35 USD (with own 3D printer)

---

*Made with ❤️ for the crypto and maker communities*
