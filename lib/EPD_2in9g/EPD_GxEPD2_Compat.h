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

        // Initialize paint library - Try 0° rotation (native portrait)
        // Width=128, Height=296, Rotate=0
        Serial.println("EPD: Initializing Paint library (portrait 128x296, rotate=0)");
        Paint_NewImage(imageBuffer, EPD_2IN9G_WIDTH, EPD_2IN9G_HEIGHT, 0, EPD_2IN9G_WHITE);
        Paint_SelectImage(imageBuffer);

        // Fill buffer with white
        Serial.println("EPD: Clearing buffer to white...");
        Paint_Clear(EPD_2IN9G_WHITE);

        // Clear physical screen
        Serial.println("EPD: Clearing physical screen...");
        EPD_2IN9G_Clear(EPD_2IN9G_WHITE);

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
        Serial.println("EPD: firstPage() - clearing buffer");
        if (imageBuffer) {
            Paint_Clear(EPD_2IN9G_WHITE);
        }
    }

    bool nextPage() {
        // Display and return false (single page mode)
        Serial.println("EPD: nextPage() - displaying buffer");
        if (imageBuffer) {
            // TEST: Draw a test pattern using WaveShare Paint library directly
            Serial.println("EPD: Drawing test pattern with Paint library...");
            Paint_DrawRectangle(5, 5, 291, 123, EPD_2IN9G_BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
            Paint_DrawLine(10, 10, 100, 100, EPD_2IN9G_RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
            Paint_DrawString_EN(20, 40, "Test Pattern", &Font24, EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
            Paint_DrawString_EN(20, 70, "Paint Library", &Font20, EPD_2IN9G_YELLOW, EPD_2IN9G_WHITE);

            EPD_2IN9G_Display(imageBuffer);
        }
        return false;
    }

    void clearScreen(uint8_t value = 0xFF) {
        Serial.println("EPD: clearScreen()");
        EPD_2IN9G_Clear(EPD_2IN9G_WHITE);
    }

    void display() {
        Serial.println("EPD: display() - refreshing screen");
        if (imageBuffer) {
            EPD_2IN9G_Display(imageBuffer);
        }
    }

    void hibernate() {
        EPD_2IN9G_Sleep();
    }

    void setRotation(uint8_t r) {
        Serial.printf("EPD: setRotation(%d) - ignored (always landscape)\n", r);
        // Rotation is handled by Paint library, ignore this call
        _rotation = 0;  // Always 0 for our setup
    }

    // Adafruit GFX required override
    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        static int pixelCount = 0;
        if (!imageBuffer) return;

        // Debug: print first 10 pixels
        if (pixelCount < 10) {
            Serial.printf("EPD: drawPixel(%d, %d, color=%d)\n", x, y, color);
            pixelCount++;
        }

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
        Serial.printf("EPD: fillScreen(color=%d)\n", color);
        if (imageBuffer) {
            Paint_Clear(color);
        }
    }

    uint16_t width() const { return _width; }
    uint16_t height() const { return _height; }
};

#endif // EPD_GXEPD2_COMPAT_H
