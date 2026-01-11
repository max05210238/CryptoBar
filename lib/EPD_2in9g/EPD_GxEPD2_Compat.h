/**
 * GxEPD2 Compatibility Wrapper for EPD_2in9g Driver
 *
 * This class provides a GxEPD2-like API to minimize code changes
 * in the CryptoBar project while using the WaveShare EPD_2in9g driver.
 */

#ifndef EPD_GXEPD2_COMPAT_H
#define EPD_GXEPD2_COMPAT_H

#include "DEV_Config.h"
#include "EPD_2in9g.h"
#include "GUI_Paint.h"
#include <Adafruit_GFX.h>

// Color definitions for compatibility
#define GxEPD_BLACK   EPD_2IN9G_BLACK
#define GxEPD_WHITE   EPD_2IN9G_WHITE
#define GxEPD_YELLOW  EPD_2IN9G_YELLOW
#define GxEPD_RED     EPD_2IN9G_RED

class EPD_GxEPD2_Compat : public Adafruit_GFX {
private:
    UBYTE *imageBuffer;
    bool initialized;
    uint16_t _width;
    uint16_t _height;
    uint8_t _rotation;

public:
    static const uint16_t HEIGHT = EPD_2IN9G_HEIGHT;  // 296
    static const uint16_t WIDTH = EPD_2IN9G_WIDTH;    // 128

    EPD_GxEPD2_Compat(int8_t cs, int8_t dc, int8_t rst, int8_t busy)
        : Adafruit_GFX(HEIGHT, WIDTH), imageBuffer(nullptr), initialized(false),
          _width(HEIGHT), _height(WIDTH), _rotation(0) {
        // Pin configuration is in DEV_Config.h, ignore constructor pins
        // Landscape mode: 296 (width) x 128 (height)
    }

    ~EPD_GxEPD2_Compat() {
        if (imageBuffer) {
            free(imageBuffer);
        }
    }

    void init(uint32_t serial_diag_bitrate = 0) {
        if (initialized) return;

        Serial.println("EPD: Initializing hardware...");

        // Initialize hardware
        DEV_Module_Init();
        EPD_2IN9G_Init();

        // Allocate image buffer
        // Each pixel is 2 bits (4 colors), so WIDTH/4 bytes per row
        UWORD imagesize = ((EPD_2IN9G_WIDTH % 4 == 0) ? (EPD_2IN9G_WIDTH / 4) : (EPD_2IN9G_WIDTH / 4 + 1)) * EPD_2IN9G_HEIGHT;
        Serial.printf("EPD: Allocating %d bytes for image buffer\n", imagesize);

        imageBuffer = (UBYTE *)malloc(imagesize);
        if (!imageBuffer) {
            Serial.println("ERROR: Failed to allocate display buffer!");
            return;
        }

        // Initialize paint library for landscape mode
        // Image size: 296x128 (landscape), rotated 270° to fit physical 128x296 (portrait)
        Serial.println("EPD: Initializing Paint library (landscape 296x128, rotated 270°)");
        Paint_NewImage(imageBuffer, EPD_2IN9G_HEIGHT, EPD_2IN9G_WIDTH, 270, EPD_2IN9G_WHITE);
        Paint_SelectImage(imageBuffer);

        // Fill buffer with white
        Serial.println("EPD: Clearing buffer to white...");
        Paint_Clear(EPD_2IN9G_WHITE);

        initialized = true;
        Serial.println("EPD: Initialization complete!");
    }

    void setFullWindow() {
        // No-op for this driver (always full window)
    }

    void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
        // Partial refresh not supported on 4-color display
        // Silently ignore
    }

    void firstPage() {
        // Clear buffer for new frame
        if (imageBuffer) {
            Paint_Clear(EPD_2IN9G_WHITE);
        }
    }

    bool nextPage() {
        // Display and return false (single page mode)
        if (imageBuffer) {
            EPD_2IN9G_Display(imageBuffer);
        }
        return false;
    }

    void clearScreen(uint8_t value = 0xFF) {
        EPD_2IN9G_Clear(EPD_2IN9G_WHITE);
    }

    void display() {
        if (imageBuffer) {
            EPD_2IN9G_Display(imageBuffer);
        }
    }

    void hibernate() {
        EPD_2IN9G_Sleep();
    }

    // Adafruit GFX required override
    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (!imageBuffer) return;
        Paint_SetPixel(x, y, color);
    }

    // Fill rectangle
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        for (int16_t i = x; i < x + w; i++) {
            for (int16_t j = y; j < y + h; j++) {
                drawPixel(i, j, color);
            }
        }
    }

    // Draw line
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        Paint_DrawLine(x0, y0, x1, y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    // Draw rectangle
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        Paint_DrawRectangle(x, y, x + w - 1, y + h - 1, color, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }

    // Clear display
    void fillScreen(uint16_t color) {
        if (imageBuffer) {
            Paint_Clear(color);
        }
    }

    uint16_t width() const { return _width; }
    uint16_t height() const { return _height; }
};

#endif // EPD_GXEPD2_COMPAT_H
