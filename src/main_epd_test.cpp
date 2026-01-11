// CryptoBar V0.99r-G - WaveShare EPD 2.9G Driver Test
// Minimal test to verify EPD_2in9g driver works

#include <Arduino.h>
#include "../lib/EPD_2in9g/DEV_Config.h"
#include "../lib/EPD_2in9g/EPD_2in9g.h"
#include "../lib/EPD_2in9g/GUI_Paint.h"

// Image buffer
UBYTE *BlackImage;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== CryptoBar EPD 2.9G Test ===");

  // Initialize SPI and GPIOs
  Serial.println("Initializing GPIOs...");
  if (DEV_Module_Init() != 0) {
    Serial.println("ERROR: GPIO init failed!");
    return;
  }

  // Initialize EPD
  Serial.println("Initializing EPD...");
  EPD_2IN9G_Init();
  Serial.println("EPD Init OK");

  // Clear screen to white
  Serial.println("Clearing screen...");
  EPD_2IN9G_Clear(EPD_2IN9G_WHITE);
  DEV_Delay_ms(2000);

  // Create image buffer
  // Width  = 128 pixels / 4 = 32 bytes per row
  // Height = 296 pixels
  UWORD Imagesize = ((EPD_2IN9G_WIDTH % 4 == 0) ? (EPD_2IN9G_WIDTH / 4) : (EPD_2IN9G_WIDTH / 4 + 1)) * EPD_2IN9G_HEIGHT;
  Serial.printf("Allocating image buffer: %d bytes\n", Imagesize);

  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Serial.println("ERROR: Failed to allocate memory!");
    return;
  }

  // Initialize paint library (rotated 270 degrees to match landscape mode)
  Paint_NewImage(BlackImage, EPD_2IN9G_HEIGHT, EPD_2IN9G_WIDTH, 270, EPD_2IN9G_WHITE);
  Paint_SelectImage(BlackImage);
  Paint_Clear(EPD_2IN9G_WHITE);

  // Draw test pattern
  Serial.println("Drawing test pattern...");

  // Black border
  Paint_DrawRectangle(0, 0, EPD_2IN9G_HEIGHT - 1, EPD_2IN9G_WIDTH - 1, EPD_2IN9G_BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

  // Text in different colors
  Paint_DrawString_EN(10, 10, "CryptoBar", &Font24, EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
  Paint_DrawString_EN(10, 40, "WaveShare 2.9G", &Font20, EPD_2IN9G_BLACK, EPD_2IN9G_WHITE);
  Paint_DrawString_EN(10, 70, "4-Color Test", &Font16, EPD_2IN9G_RED, EPD_2IN9G_WHITE);
  Paint_DrawString_EN(10, 95, "Yellow Line", &Font16, EPD_2IN9G_YELLOW, EPD_2IN9G_WHITE);

  // Display the image
  Serial.println("Refreshing display...");
  EPD_2IN9G_Display(BlackImage);
  Serial.println("Display refresh complete!");

  // Enter sleep mode
  Serial.println("Entering sleep mode...");
  EPD_2IN9G_Sleep();

  free(BlackImage);
  BlackImage = NULL;

  Serial.println("\n=== Test Complete ===");
  Serial.println("If you see the test pattern on screen, the driver is working!");
}

void loop() {
  // Nothing to do
  delay(1000);
}
