// led_status.cpp
// CryptoBar V0.97: move LED logic (NeoPixel + breathe thresholds + re-assert) out of main.cpp
#include <Arduino.h>
#include <math.h>
#include <sys/time.h>
#include <Adafruit_NeoPixel.h>

#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "led_status.h"

// ===== NeoPixel instances =====
static Adafruit_NeoPixel* s_pixel = nullptr;      // external WS2812
static Adafruit_NeoPixel* s_boardPixel = nullptr; // onboard WS2812 (used only to clear)

// Protect NeoPixel show() calls if we animate from a background task.
static SemaphoreHandle_t s_showMutex = nullptr;

// Animator task (optional)
static TaskHandle_t s_animTaskHandle = nullptr;
static uint16_t s_animHz = 30;
static volatile bool s_ctxAppRunning = false;
static volatile bool s_ctxPriceOk = false;

static inline void ensureShowMutex() {
  if (!s_showMutex) s_showMutex = xSemaphoreCreateMutex();
}

static inline void showStripLocked(Adafruit_NeoPixel* strip) {
  if (!strip) return;
  if (s_showMutex) xSemaphoreTake(s_showMutex, portMAX_DELAY);
  strip->show();
  if (s_showMutex) xSemaphoreGive(s_showMutex);
}

static inline bool timeIsValidForSync() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (tv.tv_sec > 1600000000); // ~2020-09-13
}

static inline uint64_t epochMs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

// ===== Master brightness =====
static float s_masterBrightness = 0.5f;

// ===== Current logical RGB (unscaled) =====
static uint8_t s_logicalR = 0;
static uint8_t s_logicalG = 0;
static uint8_t s_logicalB = 0;

// ===== NeoPixel "re-assert" cache =====
static uint32_t s_lastNeoPixelColor = 0xFFFFFFFF;
static uint32_t s_lastNeoPixelShowMs = 0;
static const uint32_t LED_NEOPIXEL_REASSERT_MS = 1500;

// Keep onboard WS2812 forced-off (some boards/libraries may light it back up).
static uint32_t s_lastBoardOffShowMs = 0;
static const uint32_t LED_BOARD_OFF_REASSERT_MS = 2000;

// ===== LED Color Constants =====
static const uint8_t COLOR_GREEN_R = 0,   COLOR_GREEN_G = 150, COLOR_GREEN_B = 0;
static const uint8_t COLOR_RED_R   = 150, COLOR_RED_G   = 0,   COLOR_RED_B   = 0;
static const uint8_t COLOR_BLUE_R  = 0,   COLOR_BLUE_G  = 0,   COLOR_BLUE_B  = 150;
static const uint8_t COLOR_PURPLE_R = 100, COLOR_PURPLE_G = 0,   COLOR_PURPLE_B = 120;
static const uint8_t COLOR_YELLOW_R = 150, COLOR_YELLOW_G = 150, COLOR_YELLOW_B = 0;
static const uint8_t COLOR_CYAN_R   = 0,   COLOR_CYAN_G   = 150, COLOR_CYAN_B   = 150;
static const uint8_t COLOR_GRAY_R   = 60,  COLOR_GRAY_G   = 60,  COLOR_GRAY_B   = 60;
static const uint8_t COLOR_WHITE_R  = 30,  COLOR_WHITE_G  = 30,  COLOR_WHITE_B  = 30;

// ===== Trend + animation state =====
enum LedTrend { LED_NEUTRAL, LED_UP, LED_DOWN };
static LedTrend s_ledTrend = LED_NEUTRAL;

enum LedAnimMode : uint8_t {
  LED_ANIM_SOLID = 0,
  LED_ANIM_BREATHE_SLOW = 1,
  LED_ANIM_BREATHE_FAST = 2,
  LED_ANIM_PARTY = 3  // Rainbow gradient for +20% gains
};
static LedAnimMode s_ledAnimMode = LED_ANIM_SOLID;

static uint8_t  s_ledBaseR = 60;
static uint8_t  s_ledBaseG = 60;
static uint8_t  s_ledBaseB = 60;

static uint32_t s_ledAnimStartMs  = 0;
static uint32_t s_ledAnimLastMs   = 0;

// thresholds / tuning
static const float    LED_EVENT_SLOW_THRESH = 5.0f;
static const float    LED_EVENT_FAST_THRESH = 10.0f;
static const float    LED_PARTY_ENTER_THRESH = 20.0f;  // +20% triggers party mode
static const float    LED_PARTY_EXIT_THRESH = 15.0f;   // <+15% exits party mode
static const uint32_t LED_BREATHE_SLOW_PERIOD_MS = 2400;
static const uint32_t LED_BREATHE_FAST_PERIOD_MS = 900;
static const uint32_t LED_PARTY_PERIOD_MS = 2500;      // 2.5 seconds per rainbow cycle
static const float    LED_BREATHE_MIN_FRAC = 0.15f;    // Minimum brightness still visible (within master brightness)
static const uint32_t LED_ANIM_MIN_UPDATE_MS = 33;     // ~30Hz animation rate

static inline float absf(float x) { return (x < 0.0f) ? -x : x; }

void ledStatusBegin(uint8_t neopixelPin, uint16_t neopixelCount,
                    uint8_t boardPin, uint16_t boardCount) {
  ensureShowMutex();

 // Allocate once
  if (!s_pixel) {
    s_pixel = new Adafruit_NeoPixel(neopixelCount, neopixelPin, NEO_GRB + NEO_KHZ800);
    s_pixel->begin();
    s_pixel->clear();
    showStripLocked(s_pixel);
  }

  if (!s_boardPixel) {
    s_boardPixel = new Adafruit_NeoPixel(boardCount, boardPin, NEO_GRB + NEO_KHZ800);
    s_boardPixel->begin();
    s_boardPixel->clear();
    showStripLocked(s_boardPixel);
  }

 // Reset caches
  s_lastNeoPixelColor = 0xFFFFFFFF;
  s_lastNeoPixelShowMs = 0;
  s_ledAnimStartMs = millis();
  s_ledAnimLastMs  = 0;
}

void ledStatusSetMasterBrightness(float master01) {
  if (master01 < 0.0f) master01 = 0.0f;
  if (master01 > 1.0f) master01 = 1.0f;
  s_masterBrightness = master01;
}

float ledStatusGetMasterBrightness() { return s_masterBrightness; }

void ledStatusGetLogicalRgb(uint8_t* r, uint8_t* g, uint8_t* b) {
  if (r) *r = s_logicalR;
  if (g) *g = s_logicalG;
  if (b) *b = s_logicalB;
}

// Master brightness scaling (LED brightness setting = master limit; animation only varies within this limit)
static inline uint8_t scaleBright(uint8_t v, float animFactor = 1.0f) {
  float m = s_masterBrightness * animFactor;
  if (m <= 0.001f) return 0;
  if (m > 1.0f) m = 1.0f;
  return (uint8_t)((float)v * m + 0.5f);
}

// animFactor: 0.0..1.0 (SOLID=1.0; used for breathing animation)
static void setLedScaled(uint8_t r, uint8_t g, uint8_t b, float animFactor = 1.0f) {
  if (!s_pixel) return;

 // Master OFF -> force off
  if (s_masterBrightness <= 0.001f) {
    r = g = b = 0;
    animFactor = 1.0f;
  }

  s_logicalR = r;
  s_logicalG = g;
  s_logicalB = b;

  uint8_t sr = scaleBright(r, animFactor);
  uint8_t sg = scaleBright(g, animFactor);
  uint8_t sb = scaleBright(b, animFactor);

  uint32_t packed = ((uint32_t)sr << 16) | ((uint32_t)sg << 8) | (uint32_t)sb;
  uint32_t nowMs = millis();
  if (packed == s_lastNeoPixelColor && (uint32_t)(nowMs - s_lastNeoPixelShowMs) < LED_NEOPIXEL_REASSERT_MS) return;

  s_pixel->setPixelColor(0, s_pixel->Color(sr, sg, sb));
  showStripLocked(s_pixel);
  s_lastNeoPixelColor = packed;
  s_lastNeoPixelShowMs = nowMs;
}

void ledStatusService() {
  uint32_t nowMs = millis();

 // Keep onboard WS2812 dark.
  if (s_boardPixel && (uint32_t)(nowMs - s_lastBoardOffShowMs) >= LED_BOARD_OFF_REASSERT_MS) {
    s_boardPixel->setPixelColor(0, 0);
    showStripLocked(s_boardPixel);
    s_lastBoardOffShowMs = nowMs;
  }

 // Re-assert last external NeoPixel color even if callers aren't changing state.
  if (!s_pixel) return;
  if (s_masterBrightness <= 0.001f) return;

  if ((uint32_t)(nowMs - s_lastNeoPixelShowMs) >= LED_NEOPIXEL_REASSERT_MS) {
 // Re-send the last *rendered* color (preserves whatever brightness/anim factor was last shown).
    uint32_t packed = s_lastNeoPixelColor;
    if (packed == 0xFFFFFFFF) packed = 0; // safety
    uint8_t sr = (packed >> 16) & 0xFF;
    uint8_t sg = (packed >> 8) & 0xFF;
    uint8_t sb = (packed) & 0xFF;
    s_pixel->setPixelColor(0, s_pixel->Color(sr, sg, sb));
    showStripLocked(s_pixel);
    s_lastNeoPixelShowMs = nowMs;
  }
}

void setLed(uint8_t r, uint8_t g, uint8_t b) {
  setLedScaled(r, g, b, 1.0f);
}

void fadeLedTo(uint8_t r, uint8_t g, uint8_t b, int steps, int delayMs) {
  if (!s_pixel) return;

  int startR = s_logicalR;
  int startG = s_logicalG;
  int startB = s_logicalB;

  for (int i = 1; i <= steps; ++i) {
    int cr = startR + (int)((int)r - startR) * i / steps;
    int cg = startG + (int)((int)g - startG) * i / steps;
    int cb = startB + (int)((int)b - startB) * i / steps;

 // fade 不吃呼吸 factor（保持單純跟隨 master 亮度）
    uint8_t sr = scaleBright((uint8_t)cr, 1.0f);
    uint8_t sg = scaleBright((uint8_t)cg, 1.0f);
    uint8_t sb = scaleBright((uint8_t)cb, 1.0f);

    s_pixel->setPixelColor(0, s_pixel->Color(sr, sg, sb));
    showStripLocked(s_pixel);
    s_lastNeoPixelShowMs = millis();
    delay(delayMs);
  }

  s_logicalR = r;
  s_logicalG = g;
  s_logicalB = b;

 // Sync cache (avoid one-frame mismatch)
  s_lastNeoPixelColor = ((uint32_t)scaleBright(r, 1.0f) << 16) |
                        ((uint32_t)scaleBright(g, 1.0f) << 8)  |
                        (uint32_t)scaleBright(b, 1.0f);
  s_lastNeoPixelShowMs = millis();
}

void setLedOff()      { setLed(0, 0, 0); }
void setLedGreen()    { setLed(COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B); }
void setLedRed()      { setLed(COLOR_RED_R, COLOR_RED_G, COLOR_RED_B); }
void setLedBlue()     { setLed(COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B); }
void setLedPurple()   { setLed(COLOR_PURPLE_R, COLOR_PURPLE_G, COLOR_PURPLE_B); }
void setLedYellow()   { setLed(COLOR_YELLOW_R, COLOR_YELLOW_G, COLOR_YELLOW_B); }
void setLedCyan()     { setLed(COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B); }
void setLedWhiteLow() { setLed(COLOR_WHITE_R, COLOR_WHITE_G, COLOR_WHITE_B); }

// HSV to RGB conversion for rainbow party mode
// h: hue (0.0 - 360.0), s: saturation (0.0 - 1.0), v: value/brightness (0.0 - 1.0)
static void hsvToRgb(float h, float s, float v, uint8_t* r, uint8_t* g, uint8_t* b) {
  // Normalize hue to 0-360 range
  while (h < 0.0f) h += 360.0f;
  while (h >= 360.0f) h -= 360.0f;

  float c = v * s;  // chroma
  float hPrime = h / 60.0f;
  float x = c * (1.0f - absf(fmodf(hPrime, 2.0f) - 1.0f));
  float m = v - c;

  float rF = 0, gF = 0, bF = 0;

  if (hPrime >= 0.0f && hPrime < 1.0f) {
    rF = c; gF = x; bF = 0;
  } else if (hPrime >= 1.0f && hPrime < 2.0f) {
    rF = x; gF = c; bF = 0;
  } else if (hPrime >= 2.0f && hPrime < 3.0f) {
    rF = 0; gF = c; bF = x;
  } else if (hPrime >= 3.0f && hPrime < 4.0f) {
    rF = 0; gF = x; bF = c;
  } else if (hPrime >= 4.0f && hPrime < 5.0f) {
    rF = x; gF = 0; bF = c;
  } else {
    rF = c; gF = 0; bF = x;
  }

  *r = (uint8_t)((rF + m) * 255.0f + 0.5f);
  *g = (uint8_t)((gF + m) * 255.0f + 0.5f);
  *b = (uint8_t)((bF + m) * 255.0f + 0.5f);
}

static float breatheFactor(uint32_t nowMs, uint32_t periodMs) {
 // Smooth 0->1->0 wave using sin.
 // If wall-clock time is valid (SNTP), anchor phase to epoch time so multiple devices
 // stay in sync. Otherwise, fall back to boot-millis with a per-mode start point.
  if (periodMs == 0) return 1.0f;

  uint32_t phaseMs = 0;
  if (timeIsValidForSync()) {
    phaseMs = (uint32_t)(epochMs() % (uint64_t)periodMs);
  } else {
    phaseMs = (uint32_t)((nowMs - s_ledAnimStartMs) % periodMs);
  }

  float t = (float)phaseMs / (float)periodMs; // 0..1
  float wave = (sinf(TWO_PI * t) + 1.0f) * 0.5f; // 0..1
  return LED_BREATHE_MIN_FRAC + (1.0f - LED_BREATHE_MIN_FRAC) * wave; // min..1
}

void updateLedForPrice(double change24h, bool priceOk) {
 // If price invalid, show yellow (API fail / no data)
  if (!priceOk) {
    s_ledAnimMode = LED_ANIM_SOLID;
    s_ledBaseR = COLOR_YELLOW_R;
    s_ledBaseG = COLOR_YELLOW_G;
    s_ledBaseB = COLOR_YELLOW_B;
    setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, 1.0f);
    return;
  }

 // --- Party Mode: +20% celebration with rainbow gradient ---
  if (change24h >= LED_PARTY_ENTER_THRESH) {
    if (s_ledAnimMode != LED_ANIM_PARTY) {
      s_ledAnimMode = LED_ANIM_PARTY;
      s_ledAnimStartMs = millis();
      s_ledAnimLastMs = 0;
      Serial.println("[LED] 🎉 Party Mode activated! (+20%)");
    }
    // Party mode handles its own color updates, no base color needed
    return;
  }

  // Exit party mode when dropping below threshold
  if (s_ledAnimMode == LED_ANIM_PARTY && change24h < LED_PARTY_EXIT_THRESH) {
    Serial.println("[LED] Party Mode ended (below +15%)");
    s_ledAnimMode = LED_ANIM_SOLID;
    s_ledAnimStartMs = millis();
    s_ledAnimLastMs = 0;
    // Continue to normal trend logic below
  }

 // --- Trend (near-zero hysteresis) ---
  const float ENTER = 0.02f;  // Enter red/green only when exceeding 0.02%
  const float EXIT  = 0.005f; // Return to white/gray only when below 0.005%

  if (s_ledTrend == LED_NEUTRAL) {
    if (change24h >  ENTER) s_ledTrend = LED_UP;
    else if (change24h < -ENTER) s_ledTrend = LED_DOWN;
  } else if (s_ledTrend == LED_UP) {
    if (change24h < EXIT) s_ledTrend = LED_NEUTRAL;
  } else { // LED_DOWN
    if (change24h > -EXIT) s_ledTrend = LED_NEUTRAL;
  }

 // --- Event animation mode based on magnitude ---
  float mag = absf(change24h);
  LedAnimMode mode = LED_ANIM_SOLID;
  if (mag >= LED_EVENT_FAST_THRESH) mode = LED_ANIM_BREATHE_FAST;
  else if (mag >= LED_EVENT_SLOW_THRESH) mode = LED_ANIM_BREATHE_SLOW;

 // --- Base color ---
  uint8_t br = COLOR_GRAY_R, bg = COLOR_GRAY_G, bb = COLOR_GRAY_B; // neutral
  if (s_ledTrend == LED_UP)   { br = COLOR_GREEN_R; bg = COLOR_GREEN_G; bb = COLOR_GREEN_B; }
  if (s_ledTrend == LED_DOWN) { br = COLOR_RED_R;   bg = COLOR_RED_G;   bb = COLOR_RED_B; }

  bool changed = (mode != s_ledAnimMode) || (br != s_ledBaseR) || (bg != s_ledBaseG) || (bb != s_ledBaseB);
  s_ledAnimMode = mode;
  s_ledBaseR = br; s_ledBaseG = bg; s_ledBaseB = bb;

  if (changed) {
    s_ledAnimStartMs = millis();
    s_ledAnimLastMs  = 0;
  }

 // immediate apply:
 // - SOLID: full
 // - BREATHE: apply the *current* phase (prevents "long亮" if the main thread blocks)
  float f = 1.0f;
  if (s_ledAnimMode != LED_ANIM_SOLID) {
    uint32_t period = (s_ledAnimMode == LED_ANIM_BREATHE_FAST) ? LED_BREATHE_FAST_PERIOD_MS : LED_BREATHE_SLOW_PERIOD_MS;
    f = breatheFactor(millis(), period);
  }
  setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, f);
}

// Common LED animation update logic (shared by main loop and background task)
static void ledAnimUpdate(bool appRunning, bool priceOk) {
  if (!appRunning) return;

  // Price invalid: keep yellow
  if (!priceOk) {
    setLedScaled(COLOR_YELLOW_R, COLOR_YELLOW_G, COLOR_YELLOW_B, 1.0f);
    return;
  }

  // Party Mode: rainbow gradient
  if (s_ledAnimMode == LED_ANIM_PARTY) {
    uint32_t nowMs = millis();
    uint32_t t = nowMs % LED_PARTY_PERIOD_MS;
    float hue = (t / (float)LED_PARTY_PERIOD_MS) * 360.0f;

    uint8_t r, g, b;
    hsvToRgb(hue, 1.0f, 1.0f, &r, &g, &b);
    setLedScaled(r, g, b, 1.0f);
    return;
  }

  // Solid mode: maintain base color
  if (s_ledAnimMode == LED_ANIM_SOLID) {
    setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, 1.0f);
    return;
  }

  // Breathing animation
  uint32_t nowMs = millis();
  uint32_t period = (s_ledAnimMode == LED_ANIM_BREATHE_FAST) ? LED_BREATHE_FAST_PERIOD_MS : LED_BREATHE_SLOW_PERIOD_MS;
  float f = breatheFactor(nowMs, period);
  setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, f);
}

void ledAnimLoop(bool appRunning, bool priceOk) {
 // Always refresh shared context for the animator task (if enabled).
  s_ctxAppRunning = appRunning;
  s_ctxPriceOk = priceOk;

 // If the background animator task is running, the main loop doesn't need to animate.
 // (This avoids concurrent setPixelColor/show storms.)
  if (s_animTaskHandle) {
    ledStatusService();
    return;
  }

  uint32_t nowMs = millis();
  // Limit update rate to avoid spamming pixel.show()
  if (nowMs - s_ledAnimLastMs < LED_ANIM_MIN_UPDATE_MS) return;
  s_ledAnimLastMs = nowMs;

  ledAnimUpdate(appRunning, priceOk);
}

// ===== Background animator task =====

static void ledAnimTickFromTask() {
 // Keep service running even if app is not in normal mode (helps "board off" reassert).
  ledStatusService();

  bool appRunning = s_ctxAppRunning;
  bool priceOk = s_ctxPriceOk;

  ledAnimUpdate(appRunning, priceOk);
}

static void ledAnimTask(void* arg) {
  (void)arg;
  TickType_t last = xTaskGetTickCount();
  uint32_t hz = (s_animHz < 5) ? 5 : (s_animHz > 60 ? 60 : s_animHz);
  TickType_t every = pdMS_TO_TICKS(1000UL / hz);
  if (every < 1) every = 1;

  while (true) {
    ledAnimTickFromTask();
    vTaskDelayUntil(&last, every);
  }
}

void ledAnimStartTask(uint16_t hz, uint8_t core) {
  if (s_animTaskHandle) return;
  ensureShowMutex();

 // Clamp
  if (hz < 5) hz = 5;
  if (hz > 60) hz = 60;
  s_animHz = hz;

  if (core > 1) core = 0;

 // Create pinned task so LED breathing continues even if main loop is busy.
  xTaskCreatePinnedToCore(ledAnimTask, "ledAnim", 4096, nullptr, 1, &s_animTaskHandle, core);
}
