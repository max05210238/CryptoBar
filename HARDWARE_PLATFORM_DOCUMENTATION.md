# Hardware Platform Technical Documentation
## ESP32-S3 + WaveShare 2.9" 4-Color E-Paper Display

**Version**: 1.0
**Date**: 2026-01-11
**Purpose**: Complete technical reference for developing applications on this hardware platform
**Target Projects**: IquaSense, and other ESP32-S3 + 4-color e-paper projects

---

## Table of Contents

1. [Hardware Overview](#hardware-overview)
2. [Pin Mapping](#pin-mapping)
3. [Display Driver Deep Dive](#display-driver-deep-dive)
4. [Critical Technical Discoveries](#critical-technical-discoveries)
5. [Complete Code Examples](#complete-code-examples)
6. [Known Limitations](#known-limitations)
7. [Troubleshooting Guide](#troubleshooting-guide)

---

## Hardware Overview

### Main Components

#### 1. Microcontroller: ESP32-S3-DevKitC-1
- **Chip**: ESP32-S3-WROOM-1
- **Flash**: 8MB
- **PSRAM**: 2MB (optional, but recommended)
- **WiFi**: 2.4 GHz 802.11 b/g/n
- **Bluetooth**: BLE 5.0
- **CPU**: Dual-core Xtensa LX7, up to 240 MHz
- **GPIO**: Multiple GPIO pins available

#### 2. Display: WaveShare 2.9" e-Paper Module (G)
- **Model**: GDEY029F51 (Good Display controller)
- **Physical Resolution**: 128 (width) × 296 (height) pixels (portrait orientation)
- **Logical Resolution**: 296 (width) × 128 (height) pixels (landscape, with 90° rotation)
- **Colors**: 4-color (Black, White, Red, Yellow)
- **Color Encoding**: 2 bits per pixel
  - `0x0` = Black
  - `0x1` = White
  - `0x2` = Yellow
  - `0x3` = Red
- **Refresh Time**: 15-20 seconds (full refresh only)
- **Partial Refresh**: NOT supported
- **Interface**: SPI
- **Working Voltage**: 3.3V
- **Operating Temperature**: 0°C ~ 50°C

#### 3. LED: WS2812B RGB LED
- **Type**: Addressable RGB LED (NeoPixel compatible)
- **Control**: Single-wire protocol
- **Voltage**: 5V
- **Current per LED**: ~60mA at full brightness

#### 4. User Input: Rotary Encoder
- **Type**: Mechanical rotary encoder with push button
- **Pins**: CLK, DT, SW (switch)
- **Quadrature Output**: Yes
- **Detents**: Typical 20 detents per rotation
- **ESP32-S3 Support**: PCNT (Pulse Counter) hardware support

---

## Pin Mapping

### Complete Pin Assignment

| Function | GPIO Pin | Description | Notes |
|----------|----------|-------------|-------|
| **E-Paper Display (SPI)** | | | |
| SCK | GPIO 12 | SPI Clock | |
| MOSI | GPIO 11 | SPI Data Out | |
| CS | GPIO 10 | Chip Select | Active low |
| DC | GPIO 17 | Data/Command | Low=Command, High=Data |
| RST | GPIO 16 | Reset | Active low |
| BUSY | GPIO 4 | Busy Signal | Low=idle, High=busy |
| | | | |
| **WS2812 LED** | | | |
| External LED | GPIO 48 | External WS2812 | |
| Onboard LED | GPIO 38 | Built-in WS2812 (optional) | |
| | | | |
| **Rotary Encoder** | | | |
| CLK | GPIO 2 | Encoder A (PCNT compatible) | |
| DT | GPIO 1 | Encoder B (PCNT compatible) | |
| SW | GPIO 3 | Push Button | Active low (internal pullup) |

### Pin Configuration Code

```cpp
// E-Paper SPI pins (lib/EPD_2in9g/DEV_Config.h)
#define EPD_SCK_PIN     12
#define EPD_MOSI_PIN    11
#define EPD_CS_PIN      10
#define EPD_RST_PIN     16
#define EPD_DC_PIN      17
#define EPD_BUSY_PIN    4

// LED pins (include/app_state.h)
#define LED_PIN_EXTERNAL  48
#define LED_PIN_ONBOARD   38

// Encoder pins (include/app_state.h)
#define ENCODER_CLK_PIN   2   // PCNT compatible
#define ENCODER_DT_PIN    1   // PCNT compatible
#define ENCODER_SW_PIN    3   // Button
```

---

## Display Driver Deep Dive

### Library Architecture

The display driver consists of three layers:

```
┌─────────────────────────────────────────┐
│  Application Code (UI/UX)              │
│  - Draw text, shapes, graphics         │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│  EPD_GxEPD2_Compat.h                   │  ← Compatibility wrapper
│  - Adafruit GFX API compatibility      │
│  - Provides: drawPixel(), fillRect()   │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│  GUI_Paint.h/cpp (WaveShare)           │  ← Paint library
│  - Paint_DrawString_EN()               │
│  - Paint_DrawRectangle()               │
│  - Paint_DrawLine()                    │
│  - Paint_SetPixel()                    │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│  EPD_2in9g.h/cpp (WaveShare)           │  ← Hardware driver
│  - EPD_2IN9G_Init()                    │
│  - EPD_2IN9G_Display()                 │
│  - EPD_2IN9G_Clear()                   │
│  - SPI communication                   │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│  DEV_Config.h/cpp (Hardware)           │  ← Hardware abstraction
│  - GPIO control                        │
│  - SPI initialization                  │
│  - Delay functions                     │
└─────────────────────────────────────────┘
```

### Critical Files

#### 1. Hardware Configuration (`lib/EPD_2in9g/DEV_Config.h/cpp`)

**Purpose**: Hardware abstraction layer for GPIO and SPI.

**Key Functions**:
```cpp
void DEV_Module_Init(void);           // Initialize hardware
void DEV_SPI_WriteByte(UBYTE value);  // Write single byte via SPI
void DEV_Digital_Write(int pin, int value);  // GPIO write
int DEV_Digital_Read(int pin);        // GPIO read
void DEV_Delay_ms(UDOUBLE ms);        // Delay in milliseconds
```

**Modified for CryptoBar**: Pin definitions updated to match our pin mapping.

---

#### 2. Display Driver (`lib/EPD_2in9g/EPD_2in9g.h/cpp`)

**Purpose**: Low-level e-paper display control.

**Key Functions**:
```cpp
void EPD_2IN9G_Init(void);                    // Initialize display controller
void EPD_2IN9G_Clear(UBYTE color);            // Clear entire screen to one color
void EPD_2IN9G_Display(const UBYTE *Image);   // Display image buffer
void EPD_2IN9G_Sleep(void);                   // Put display into deep sleep
```

**Display Specifications**:
```cpp
#define EPD_2IN9G_WIDTH   128   // Physical width (portrait)
#define EPD_2IN9G_HEIGHT  296   // Physical height (portrait)

// Color definitions
#define EPD_2IN9G_BLACK   0x0
#define EPD_2IN9G_WHITE   0x1
#define EPD_2IN9G_YELLOW  0x2
#define EPD_2IN9G_RED     0x3
```

**Initialization Sequence**:
```cpp
EPD_2IN9G_Init();              // Initialize display hardware
EPD_2IN9G_Clear(EPD_2IN9G_WHITE);  // Clear to white (takes ~20 seconds)
```

**Display Update**:
```cpp
// After drawing to image buffer
EPD_2IN9G_Display(imageBuffer);  // Refresh display (takes 15-20 seconds)
```

---

#### 3. Paint Library (`lib/EPD_2in9g/GUI_Paint.h/cpp`)

**Purpose**: Drawing primitives and buffer management.

**Key Structure**:
```cpp
typedef struct {
    UBYTE *Image;           // Pointer to image buffer
    UWORD Width;            // Logical width (after rotation)
    UWORD Height;           // Logical height (after rotation)
    UWORD WidthMemory;      // Physical width (buffer)
    UWORD HeightMemory;     // Physical height (buffer)
    UWORD Color;            // Default foreground color
    UWORD Rotate;           // Rotation: 0, 90, 180, 270
    UWORD Mirror;           // Mirror mode
    UWORD WidthByte;        // Bytes per row (critical!)
    UWORD HeightByte;       // Rows in buffer
    UWORD Scale;            // Color depth (2, 4, 7, 16)
} PAINT;
```

**Initialization** (CRITICAL - This took hours to figure out!):
```cpp
// Allocate buffer
UWORD imagesize = ((EPD_2IN9G_WIDTH % 4 == 0) ?
                   (EPD_2IN9G_WIDTH / 4) :
                   (EPD_2IN9G_WIDTH / 4 + 1)) * EPD_2IN9G_HEIGHT;
// Result: imagesize = 9,472 bytes

UBYTE *imageBuffer = (UBYTE *)malloc(imagesize);

// Initialize Paint library with PHYSICAL dimensions
// IMPORTANT: Use physical width/height, NOT rotated dimensions!
Paint_NewImage(imageBuffer,
               EPD_2IN9G_WIDTH,   // 128 (physical width)
               EPD_2IN9G_HEIGHT,  // 296 (physical height)
               90,                // Rotation (0, 90, 180, 270)
               EPD_2IN9G_WHITE);  // Background color

// CRITICAL: Set scale to 4 for 4-color mode (2 bits per pixel)
Paint_SetScale(4);

// Select the image for drawing
Paint_SelectImage(imageBuffer);

// Clear buffer
Paint_Clear(EPD_2IN9G_WHITE);
```

**Why Scale=4 is Critical**:
- Scale determines bits per pixel and buffer layout
- Scale=2: 1 bit per pixel (black/white only), WidthByte = Width/8
- **Scale=4: 2 bits per pixel (4 colors), WidthByte = Width/4** ← Correct for our display
- Scale=7: 4 bits per pixel (16 colors, for other displays)

**Buffer Calculation**:
```cpp
// With Scale=4 and physical width=128:
WidthByte = 128 / 4 = 32 bytes per row
Total buffer size = 32 bytes/row × 296 rows = 9,472 bytes
```

**Drawing Functions**:
```cpp
// Pixel operations
void Paint_SetPixel(UWORD x, UWORD y, UWORD Color);

// Shapes
void Paint_DrawLine(UWORD x1, UWORD y1, UWORD x2, UWORD y2,
                    UWORD Color, DOT_PIXEL LineWidth, LINE_STYLE LineStyle);
void Paint_DrawRectangle(UWORD x1, UWORD y1, UWORD x2, UWORD y2,
                         UWORD Color, DOT_PIXEL LineWidth, DRAW_FILL Filled);
void Paint_DrawCircle(UWORD xCenter, UWORD yCenter, UWORD Radius,
                      UWORD Color, DOT_PIXEL LineWidth, DRAW_FILL Filled);

// Text
void Paint_DrawString_EN(UWORD x, UWORD y, const char *pString,
                         sFONT* Font, UWORD ForeColor, UWORD BackColor);
void Paint_DrawNum(UWORD x, UWORD y, int32_t Number,
                   sFONT* Font, UWORD ForeColor, UWORD BackColor);

// Buffer operations
void Paint_Clear(UWORD Color);  // Clear entire buffer
void Paint_ClearWindows(UWORD x1, UWORD y1, UWORD x2, UWORD y2, UWORD Color);
```

**Available Fonts**:
```cpp
extern sFONT Font8;   // 5x8 pixels
extern sFONT Font12;  // 7x12 pixels
extern sFONT Font16;  // 11x16 pixels
extern sFONT Font20;  // 14x20 pixels
extern sFONT Font24;  // 17x24 pixels
```

---

#### 4. Compatibility Wrapper (`lib/EPD_2in9g/EPD_GxEPD2_Compat.h`)

**Purpose**: Provide Adafruit GFX API compatibility for existing code.

**Why This Exists**:
- CryptoBar originally used GxEPD2 library (Adafruit GFX compatible)
- This wrapper allows existing UI code to work without modification
- For new projects (like IquaSense), you can use Paint library directly

**Class Definition**:
```cpp
class EPD_GxEPD2_Compat : public Adafruit_GFX {
private:
    UBYTE *imageBuffer;
    bool initialized;
    uint16_t _width;   // 296 (landscape)
    uint16_t _height;  // 128 (landscape)
    uint8_t _rotation;

public:
    static const uint16_t HEIGHT = EPD_2IN9G_HEIGHT;  // 296
    static const uint16_t WIDTH = EPD_2IN9G_WIDTH;    // 128

    EPD_GxEPD2_Compat(int8_t cs, int8_t dc, int8_t rst, int8_t busy);
    void init(uint32_t serial_diag_bitrate = 0);
    void setRotation(uint8_t r);
    void setFullWindow(void);
    void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void firstPage();
    bool nextPage();
    void clearScreen(uint8_t value = 0xFF);

    // Adafruit GFX overrides
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
};
```

**Typical Usage Pattern**:
```cpp
// Initialization
EPD_GxEPD2_Compat display(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
display.init();

// Drawing
display.firstPage();
do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.setTextColor(GxEPD_BLACK);
    display.print("Hello World");
} while (display.nextPage());
```

**For IquaSense**: You can skip this wrapper and use Paint library directly for better performance.

---

## Critical Technical Discoveries

### Discovery 1: Scale=4 Must Be Set

**Problem**: Display only rendered on left ~40% of screen, right side showed noise.

**Root Cause**: `Paint_NewImage()` hardcodes `Paint.Scale = 2` (1 bit per pixel), but our display requires `Scale = 4` (2 bits per pixel for 4 colors).

**Solution**:
```cpp
Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT, 90, EPD_2IN9G_WHITE);
Paint_SetScale(4);  // CRITICAL! Must set scale to 4 after Paint_NewImage()
```

**Why This Matters**:
- Scale determines `WidthByte` calculation in `Paint_SetPixel()`
- Wrong scale → wrong address calculation → garbage on screen
- With Scale=2: WidthByte = 296/8 = 37 bytes (wrong!)
- With Scale=4: WidthByte = 128/4 = 32 bytes (correct!)

**Debugging History**:
- Spent several hours trying different rotation angles
- Finally examined `GUI_Paint.cpp` source code
- Found that `Paint_NewImage()` hardcodes `Paint.Scale = 2` at line 102
- Adding `Paint_SetScale(4)` immediately after fixed the issue

---

### Discovery 2: Use Physical Dimensions, Not Rotated

**Problem**: Confusing parameter order in `Paint_NewImage()`.

**Incorrect** (what we tried first):
```cpp
// Passing rotated dimensions - WRONG!
Paint_NewImage(imageBuffer,
               EPD_2IN9G_HEIGHT,  // 296 (rotated width)
               EPD_2IN9G_WIDTH,   // 128 (rotated height)
               270,
               EPD_2IN9G_WHITE);
```

**Correct**:
```cpp
// Passing physical dimensions - CORRECT!
Paint_NewImage(imageBuffer,
               EPD_2IN9G_WIDTH,   // 128 (physical width)
               EPD_2IN9G_HEIGHT,  // 296 (physical height)
               90,                // Rotation angle
               EPD_2IN9G_WHITE);
```

**Why**:
- `Paint_NewImage()` expects **physical buffer dimensions**
- Rotation is applied internally by the Paint library
- Passing rotated dimensions confuses the coordinate transformation

---

### Discovery 3: 90° Rotation for Correct Orientation

**Problem**: Display showed image upside down with 270° rotation.

**Testing Results**:
- 0°: Portrait mode (128×296) - not desired
- 90°: Landscape mode (296×128) - **correct orientation** ✓
- 180°: Portrait mode, upside down
- 270°: Landscape mode, upside down

**Final Configuration**:
```cpp
Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT,
               90,  // 90° rotation for correct landscape orientation
               EPD_2IN9G_WHITE);
```

---

### Discovery 4: No Partial Refresh Support

**Important Limitation**: 4-color e-paper does NOT support partial refresh.

**Implications**:
- Every screen update is a full refresh (15-20 seconds)
- Cannot update just a portion of the screen
- Display flashes through all 4 colors during refresh
- Not suitable for applications requiring frequent updates (>1/minute)

**Best Practices**:
- Design UI for infrequent updates (5+ minutes between refreshes)
- Avoid real-time displays (clocks, counters)
- Consider LED indicators for real-time status

---

## Complete Code Examples

### Example 1: Minimal Working Example

```cpp
#include "lib/EPD_2in9g/DEV_Config.h"
#include "lib/EPD_2in9g/EPD_2in9g.h"
#include "lib/EPD_2in9g/GUI_Paint.h"

UBYTE *imageBuffer = NULL;

void setup() {
    Serial.begin(115200);

    // 1. Initialize hardware
    DEV_Module_Init();
    EPD_2IN9G_Init();

    // 2. Allocate image buffer (9,472 bytes)
    UWORD imagesize = ((EPD_2IN9G_WIDTH % 4 == 0) ?
                       (EPD_2IN9G_WIDTH / 4) :
                       (EPD_2IN9G_WIDTH / 4 + 1)) * EPD_2IN9G_HEIGHT;
    imageBuffer = (UBYTE *)malloc(imagesize);
    if (!imageBuffer) {
        Serial.println("ERROR: Failed to allocate buffer!");
        return;
    }

    // 3. Initialize Paint library (CRITICAL STEPS!)
    Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT,
                   90, EPD_2IN9G_WHITE);
    Paint_SetScale(4);  // MUST SET SCALE=4 for 4-color mode!
    Paint_SelectImage(imageBuffer);

    // 4. Clear display (takes ~20 seconds)
    Serial.println("Clearing display...");
    Paint_Clear(EPD_2IN9G_WHITE);
    EPD_2IN9G_Clear(EPD_2IN9G_WHITE);

    // 5. Draw something
    Serial.println("Drawing test pattern...");
    Paint_DrawRectangle(5, 5, 291, 123, EPD_2IN9G_BLACK,
                        DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(50, 40, "Hello World!", &Font24,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
    Paint_DrawString_EN(50, 70, "4-Color Display", &Font20,
                        EPD_2IN9G_RED, EPD_2IN9G_WHITE);

    // 6. Display on screen (takes 15-20 seconds)
    Serial.println("Updating display...");
    EPD_2IN9G_Display(imageBuffer);

    Serial.println("Done!");

    // 7. Sleep mode (optional, saves power)
    // EPD_2IN9G_Sleep();
}

void loop() {
    // Nothing to do - display is static
}
```

---

### Example 2: Displaying Sensor Data

```cpp
#include "lib/EPD_2in9g/DEV_Config.h"
#include "lib/EPD_2in9g/EPD_2in9g.h"
#include "lib/EPD_2in9g/GUI_Paint.h"

UBYTE *imageBuffer = NULL;

void initDisplay() {
    DEV_Module_Init();
    EPD_2IN9G_Init();

    UWORD imagesize = ((EPD_2IN9G_WIDTH % 4 == 0) ?
                       (EPD_2IN9G_WIDTH / 4) :
                       (EPD_2IN9G_WIDTH / 4 + 1)) * EPD_2IN9G_HEIGHT;
    imageBuffer = (UBYTE *)malloc(imagesize);

    Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT,
                   90, EPD_2IN9G_WHITE);
    Paint_SetScale(4);
    Paint_SelectImage(imageBuffer);
    Paint_Clear(EPD_2IN9G_WHITE);
    EPD_2IN9G_Clear(EPD_2IN9G_WHITE);
}

void displaySensorData(float temperature, int humidity, float pressure) {
    // Clear buffer
    Paint_Clear(EPD_2IN9G_WHITE);

    // Title
    Paint_DrawString_EN(10, 10, "Sensor Readings", &Font20,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);

    // Temperature
    char temp[32];
    sprintf(temp, "Temp: %.1f C", temperature);
    UWORD color = (temperature > 30.0) ? EPD_2IN9G_RED : EPD_2IN9G_BLACK;
    Paint_DrawString_EN(10, 40, temp, &Font16, color, EPD_2IN9G_WHITE);

    // Humidity
    char hum[32];
    sprintf(hum, "Humidity: %d%%", humidity);
    Paint_DrawString_EN(10, 65, hum, &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);

    // Pressure
    char press[32];
    sprintf(press, "Pressure: %.1f hPa", pressure);
    Paint_DrawString_EN(10, 90, press, &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);

    // Update display
    EPD_2IN9G_Display(imageBuffer);
}

void setup() {
    Serial.begin(115200);
    initDisplay();
}

void loop() {
    // Read sensors (pseudo-code)
    float temp = readTemperature();
    int hum = readHumidity();
    float press = readPressure();

    // Update display
    displaySensorData(temp, hum, press);

    // Wait 5 minutes before next update
    delay(5 * 60 * 1000);
}
```

---

### Example 3: Multi-Page Layout

```cpp
void displayWaterQualityPage1() {
    Paint_Clear(EPD_2IN9G_WHITE);

    // Header
    Paint_DrawString_EN(10, 5, "Water Quality Monitor", &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
    Paint_DrawLine(5, 25, 291, 25, EPD_2IN9G_BLACK,
                   DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // TDS Value
    Paint_DrawString_EN(10, 35, "TDS:", &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
    char tds[16];
    sprintf(tds, "%d ppm", getTDSValue());
    UWORD tdsColor = (getTDSValue() > 300) ? EPD_2IN9G_YELLOW : EPD_2IN9G_BLACK;
    Paint_DrawString_EN(80, 35, tds, &Font20, tdsColor, EPD_2IN9G_WHITE);

    // pH Value
    Paint_DrawString_EN(10, 65, "pH:", &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
    char ph[16];
    sprintf(ph, "%.1f", getPHValue());
    Paint_DrawString_EN(80, 65, ph, &Font20,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);

    // Temperature
    Paint_DrawString_EN(10, 95, "Temp:", &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
    char temp[16];
    sprintf(temp, "%.1f C", getTemperature());
    Paint_DrawString_EN(80, 95, temp, &Font20,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);

    EPD_2IN9G_Display(imageBuffer);
}

void displayWaterQualityPage2() {
    Paint_Clear(EPD_2IN9G_WHITE);

    // Header
    Paint_DrawString_EN(10, 5, "Filter Status", &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
    Paint_DrawLine(5, 25, 291, 25, EPD_2IN9G_BLACK,
                   DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Filter lifespan
    int filterLife = getFilterLifePercent();
    char life[32];
    sprintf(life, "Filter: %d%%", filterLife);
    UWORD lifeColor = (filterLife < 20) ? EPD_2IN9G_RED :
                      (filterLife < 50) ? EPD_2IN9G_YELLOW : EPD_2IN9G_BLACK;
    Paint_DrawString_EN(10, 35, life, &Font20, lifeColor, EPD_2IN9G_WHITE);

    // Progress bar
    Paint_DrawRectangle(10, 65, 286, 85, EPD_2IN9G_BLACK,
                        DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    int barWidth = (filterLife * 270) / 100;
    if (barWidth > 0) {
        Paint_DrawRectangle(13, 68, 13 + barWidth, 82, lifeColor,
                            DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // Warning message
    if (filterLife < 20) {
        Paint_DrawString_EN(10, 100, "!! REPLACE FILTER !!", &Font16,
                            EPD_2IN9G_RED, EPD_2IN9G_WHITE);
    }

    EPD_2IN9G_Display(imageBuffer);
}
```

---

## Known Limitations

### 1. No Partial Refresh
- **Impact**: Every update refreshes entire screen (15-20 seconds)
- **Workaround**: Design for infrequent updates
- **Not suitable for**: Real-time clocks, counters, animations

### 2. Slow Refresh Time
- **Full refresh**: 15-20 seconds
- **Flashing**: Display cycles through all colors during refresh
- **Workaround**: Update every 5+ minutes

### 3. Limited Lifespan
- **E-paper lifetime**: ~100,000 refresh cycles (typical)
- **At 5-min intervals**: ~1 year of continuous operation
- **At 10-min intervals**: ~2 years of continuous operation
- **Recommendation**: Use longer intervals when possible

### 4. Temperature Sensitivity
- **Operating range**: 0°C ~ 50°C
- **Below 0°C**: Display may not refresh properly
- **Above 50°C**: Risk of permanent damage
- **Best range**: 15°C ~ 35°C

### 5. Limited Color Palette
- **4 colors only**: Black, White, Red, Yellow
- **No grayscale**: Cannot display gradients
- **Color mixing**: Not supported (no orange, green, purple, etc.)

### 6. Fixed Resolution
- **296×128 pixels**: Relatively low resolution
- **Small fonts**: May be hard to read from distance
- **Images**: Limited detail possible

---

## Troubleshooting Guide

### Problem: Display shows nothing (blank screen)

**Possible Causes**:
1. **Wiring issue**: Check all 6 SPI connections
2. **Power issue**: Ensure 3.3V supply is stable
3. **Initialization failure**: Check serial output for errors

**Solutions**:
```cpp
// Add debug output
Serial.println("Init hardware...");
DEV_Module_Init();
Serial.println("Init display...");
EPD_2IN9G_Init();
Serial.println("Init complete!");

// Test BUSY pin
Serial.print("BUSY pin state: ");
Serial.println(digitalRead(EPD_BUSY_PIN));
```

---

### Problem: Display shows partial image (only left side)

**Cause**: `Paint_SetScale(4)` not called after `Paint_NewImage()`.

**Solution**:
```cpp
Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT, 90, EPD_2IN9G_WHITE);
Paint_SetScale(4);  // ADD THIS LINE!
```

---

### Problem: Display is upside down

**Cause**: Wrong rotation angle.

**Solution**: Change rotation from 270° to 90° (or vice versa):
```cpp
Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT,
               90,  // Try 90 or 270
               EPD_2IN9G_WHITE);
```

---

### Problem: Text is garbled or missing

**Possible Causes**:
1. **Buffer overflow**: String too long for display width
2. **Wrong font**: Font not initialized
3. **Coordinates out of bounds**: X/Y exceeds display dimensions

**Solutions**:
```cpp
// Check string length
const char* text = "Very long text...";
int textWidth = strlen(text) * Font16.Width;
if (textWidth > 296) {
    // Text too long, truncate or use smaller font
}

// Validate coordinates
void drawTextSafe(int x, int y, const char* text) {
    if (x < 0 || x >= 296 || y < 0 || y >= 128) {
        Serial.println("WARNING: Coordinates out of bounds!");
        return;
    }
    Paint_DrawString_EN(x, y, text, &Font16, EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
}
```

---

### Problem: Display refresh hangs/freezes

**Cause**: BUSY pin stuck high (display controller stuck).

**Solutions**:
1. **Hardware reset**:
```cpp
void resetDisplay() {
    digitalWrite(EPD_RST_PIN, LOW);
    delay(200);
    digitalWrite(EPD_RST_PIN, HIGH);
    delay(200);
    EPD_2IN9G_Init();
}
```

2. **Check for busy timeout** (in `EPD_2in9g.cpp`):
```cpp
void EPD_2IN9G_ReadBusy(void) {
    unsigned long start = millis();
    while(DEV_Digital_Read(EPD_BUSY_PIN)) {
        if (millis() - start > 30000) {  // 30 second timeout
            Serial.println("ERROR: Busy timeout!");
            return;
        }
        delay(100);
    }
}
```

---

### Problem: Colors don't match expected

**Color Verification**:
```cpp
// Draw test pattern to verify colors
void drawColorTest() {
    Paint_Clear(EPD_2IN9G_WHITE);

    Paint_DrawRectangle(10, 10, 80, 50, EPD_2IN9G_BLACK,
                        DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(15, 20, "BLACK", &Font16,
                        EPD_2IN9G_WHITE, EPD_2IN9G_BLACK);

    Paint_DrawRectangle(90, 10, 160, 50, EPD_2IN9G_RED,
                        DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(100, 20, "RED", &Font16,
                        EPD_2IN9G_WHITE, EPD_2IN9G_RED);

    Paint_DrawRectangle(170, 10, 240, 50, EPD_2IN9G_YELLOW,
                        DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(175, 20, "YELLOW", &Font16,
                        EPD_2IN9G_BLACK, EPD_2IN9G_YELLOW);

    EPD_2IN9G_Display(imageBuffer);
}
```

---

### Problem: Memory allocation failure

**Symptoms**:
```
ERROR: Failed to allocate display buffer!
```

**Cause**: Not enough free heap (9,472 bytes required).

**Solutions**:
```cpp
// Check free heap before allocation
Serial.print("Free heap: ");
Serial.println(ESP.getFreeHeap());

// Use PSRAM if available
#ifdef BOARD_HAS_PSRAM
imageBuffer = (UBYTE *)ps_malloc(imagesize);
#else
imageBuffer = (UBYTE *)malloc(imagesize);
#endif

if (!imageBuffer) {
    Serial.println("Allocation failed! Trying to free memory...");
    // Free other buffers, restart, etc.
}
```

---

## Performance Considerations

### Refresh Time Breakdown

Typical full refresh sequence:
1. **Clear phase**: 5-8 seconds (display cycles colors)
2. **Draw phase**: 5-8 seconds (writes new image)
3. **Finalization**: 2-4 seconds (stabilization)
4. **Total**: 15-20 seconds

### Power Consumption

- **Active refresh**: ~30-50mA @ 3.3V
- **Idle (display on)**: ~0.1mA @ 3.3V
- **Deep sleep**: ~0.01mA @ 3.3V (with `EPD_2IN9G_Sleep()`)

**Battery life estimation**:
```
Assuming 5-minute updates:
- 12 updates/hour × 20 seconds/update = 240 seconds/hour active
- Active: 240s × 40mA = 2.67 mAh/hour
- Idle: 3360s × 0.1mA = 0.09 mAh/hour
- Total: ~2.76 mAh/hour
- 1000mAh battery → ~360 hours (~15 days)
```

---

## Bill of Materials (BOM)

| Component | Model/Spec | Quantity | Notes |
|-----------|------------|----------|-------|
| Microcontroller | ESP32-S3-DevKitC-1 (8MB Flash, 2MB PSRAM) | 1 | |
| E-Paper Display | WaveShare 2.9" Module (G) / GDEY029F51 | 1 | 4-color |
| RGB LED | WS2812B | 1-2 | NeoPixel compatible |
| Rotary Encoder | EC11 or compatible | 1 | With push button |
| Resistors | 10kΩ | 3 | Pull-up for encoder |
| Capacitor | 100µF | 1 | Power supply smoothing |
| Wires | Jumper wires or custom PCB | - | 22-26 AWG |

---

## Recommended Development Workflow

### 1. Hardware Setup
- Connect all pins according to pin mapping
- Verify connections with multimeter
- Test power supply stability (3.3V ±5%)

### 2. Software Setup
```bash
# PlatformIO project
platformio init --board esp32-s3-devkitc-1
```

**platformio.ini**:
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1

lib_deps =
    adafruit/Adafruit GFX Library@^1.11.5
    adafruit/Adafruit NeoPixel@^1.11.0
```

### 3. Test Sequence
1. **Pin test**: Verify all GPIO pins respond
2. **SPI test**: Send test data, check MOSI with oscilloscope
3. **Display init**: Initialize display, check for BUSY signal
4. **Clear test**: Clear display to white
5. **Draw test**: Draw simple shapes (rectangle, line)
6. **Text test**: Display text in all 4 colors
7. **Full UI test**: Test complete user interface

### 4. Debugging Tools
- **Serial monitor**: Primary debug output
- **Multimeter**: Verify voltages and connections
- **Logic analyzer**: Inspect SPI signals (optional)
- **Oscilloscope**: Check signal quality (optional)

---

## Source Code Repository

All driver code is located in:
```
lib/EPD_2in9g/
├── DEV_Config.h          - Hardware configuration
├── DEV_Config.cpp        - Hardware implementation
├── EPD_2in9g.h           - Display driver header
├── EPD_2in9g.cpp         - Display driver implementation
├── GUI_Paint.h           - Paint library header
├── GUI_Paint.cpp         - Paint library implementation
├── Debug.h               - Debug macros
├── fonts.h               - Font declarations
├── font8.cpp             - 5×8 font
├── font12.cpp            - 7×12 font
├── font16.cpp            - 11×16 font
├── font20.cpp            - 14×20 font
├── font24.cpp            - 17×24 font
└── EPD_GxEPD2_Compat.h   - GxEPD2 compatibility wrapper (optional)
```

**Original Source**: [WaveShare GitHub](https://github.com/waveshareteam/e-Paper/tree/master/E-paper_Separate_Program/2.9inch_e-Paper_G/ESP32)

**Modifications for CryptoBar**:
1. Pin definitions updated in `DEV_Config.h`
2. Power pin removed (not used in our hardware)
3. Compatibility wrapper added (`EPD_GxEPD2_Compat.h`)

---

## Quick Start Checklist for IquaSense

When starting IquaSense development:

- [ ] Copy `lib/EPD_2in9g/` directory to new project
- [ ] Update pin definitions if different hardware
- [ ] Test display with minimal example
- [ ] Design UI layout on paper (296×128 pixels)
- [ ] Implement UI drawing functions
- [ ] Add ESP-NOW communication
- [ ] Add sensor data processing
- [ ] Test with real water filter data
- [ ] Optimize update interval (target: 5-10 minutes)
- [ ] Add LED status indicators
- [ ] Implement web configuration portal
- [ ] Test long-term reliability

---

## Additional Resources

### Official Documentation
- [WaveShare 2.9" Module (G) Wiki](https://www.waveshare.com/wiki/2.9inch_e-Paper_Module_(G))
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Good Display GDEY029F51](https://www.good-display.com/product/468.html)

### Related Documents in This Repository
- `WAVESHARE_DRIVER_INTEGRATION.md` - Integration details
- `FIX_PARTIAL_DISPLAY_BUG.md` - Critical bug fix history
- `V0.99r-G_CUSTOMIZATION_CHECKLIST.md` - CryptoBar-specific modifications

---

**Document Version**: 1.0
**Last Updated**: 2026-01-11
**Author**: Claude (AI Assistant)
**Tested Hardware**: ESP32-S3-DevKitC-1 + WaveShare 2.9" Module (G)
**Status**: Production-ready, tested and verified

---

## End of Documentation

This document should provide everything needed to develop applications on this hardware platform. For IquaSense development, focus on:
1. Display driver (already working)
2. ESP-NOW communication (new)
3. Water quality UI design (new)
4. Web configuration portal (adapt from CryptoBar)

Good luck with IquaSense! 🚰
