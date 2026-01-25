// CryptoBar V0.99s - Hardware Display Detection Implementation
#include "display_detector.h"
#include "app_state.h"

// Detect E-ink by BUSY pin behavior
// E-ink: BUSY pin is actively driven by display (changes during reset)
// VFD:   BUSY pin is floating (pulled up, no changes)
bool detectEinkByBusy() {
  Serial.println("[Detect] Testing BUSY pin for E-ink signature...");

  // Step 1: Configure BUSY pin as INPUT_PULLUP
  pinMode(EPD_BUSY, INPUT_PULLUP);
  delay(10);  // Stabilization time

  // Step 2: Read initial BUSY level
  int level1 = digitalRead(EPD_BUSY);
  delay(5);
  int level2 = digitalRead(EPD_BUSY);
  delay(5);
  int level3 = digitalRead(EPD_BUSY);

  Serial.printf("[Detect] Initial BUSY levels: %d, %d, %d\n", level1, level2, level3);

  // Step 3: Trigger E-ink reset
  pinMode(EPD_RST, OUTPUT);
  digitalWrite(EPD_RST, LOW);   // Assert reset
  delay(10);
  digitalWrite(EPD_RST, HIGH);  // Release reset
  delay(50);                    // Wait for E-ink to respond

  // Step 4: Read BUSY after reset
  int busyAfterReset1 = digitalRead(EPD_BUSY);
  delay(50);
  int busyAfterReset2 = digitalRead(EPD_BUSY);
  delay(50);
  int busyAfterReset3 = digitalRead(EPD_BUSY);

  Serial.printf("[Detect] BUSY after reset: %d, %d, %d\n",
                busyAfterReset1, busyAfterReset2, busyAfterReset3);

  // Step 5: Check if BUSY signal changed
  // E-ink: BUSY will go LOW (busy) then HIGH (ready)
  // VFD:   BUSY stays HIGH (pulled up, floating)

  bool hasSignalChange = false;

  // Method 1: Check if any level is LOW (E-ink pulls BUSY low when busy)
  if (busyAfterReset1 == LOW || busyAfterReset2 == LOW || busyAfterReset3 == LOW) {
    hasSignalChange = true;
    Serial.println("[Detect] ✓ BUSY went LOW (E-ink busy signal detected)");
  }

  // Method 2: Check if there's any state transition
  if (busyAfterReset1 != busyAfterReset2 || busyAfterReset2 != busyAfterReset3) {
    hasSignalChange = true;
    Serial.println("[Detect] ✓ BUSY signal changed (E-ink state transition detected)");
  }

  // Method 3: Check if post-reset differs from pre-reset
  if ((level1 != busyAfterReset1) || (level2 != busyAfterReset2) || (level3 != busyAfterReset3)) {
    hasSignalChange = true;
    Serial.println("[Detect] ✓ BUSY changed after reset (E-ink detected)");
  }

  return hasSignalChange;
}

// Main detection function
DisplayType detectDisplayType() {
  Serial.println("========================================");
  Serial.println("[Detect] Starting automatic display detection...");
  Serial.println("[Detect] Shared GPIO: CS=10, MOSI=11, SCK=12, RST=16");
  Serial.println("[Detect] E-ink only: DC=17, BUSY=4");
  Serial.println("[Detect] Detection method: BUSY pin behavior");
  Serial.println("========================================");

  // Method 1: BUSY pin detection (primary)
  bool einkDetected = detectEinkByBusy();

  if (einkDetected) {
    Serial.println("========================================");
    Serial.println("[Detect] ✓ E-ink Display Detected");
    Serial.println("[Detect] Model: WaveShare 2.9\" B&W");
    Serial.println("[Detect] Mode: CryptoBar Standard");
    Serial.println("========================================");
    return DISPLAY_EINK;
  } else {
    Serial.println("========================================");
    Serial.println("[Detect] ✓ VFD Display Detected");
    Serial.println("[Detect] Model: PT6302 16-char Dot Matrix");
    Serial.println("[Detect] Mode: CryptoBar Retro");
    Serial.println("========================================");
    return DISPLAY_VFD;
  }
}
