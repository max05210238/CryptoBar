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

// ===== Trend + animation state =====
enum LedTrend { LED_NEUTRAL, LED_UP, LED_DOWN };
static LedTrend s_ledTrend = LED_NEUTRAL;

enum LedAnimMode : uint8_t { LED_ANIM_SOLID = 0, LED_ANIM_BREATHE_SLOW = 1, LED_ANIM_BREATHE_FAST = 2 };
static LedAnimMode s_ledAnimMode = LED_ANIM_SOLID;

static uint8_t  s_ledBaseR = 60;
static uint8_t  s_ledBaseG = 60;
static uint8_t  s_ledBaseB = 60;

static uint32_t s_ledAnimStartMs  = 0;
static uint32_t s_ledAnimLastMs   = 0;

// thresholds / tuning
static const float    LED_EVENT_SLOW_THRESH = 5.0f;
static const float    LED_EVENT_FAST_THRESH = 10.0f;
static const uint32_t LED_BREATHE_SLOW_PERIOD_MS = 2400;
static const uint32_t LED_BREATHE_FAST_PERIOD_MS = 900;
static const float    LED_BREATHE_MIN_FRAC = 0.15f;   // 最暗時仍可見（在 master 亮度內）

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

// Master 亮度縮放（設定中的 LED 亮度 = master 上限；動畫只會在此上限內變化）
static inline uint8_t scaleBright(uint8_t v, float animFactor = 1.0f) {
  float m = s_masterBrightness * animFactor;
  if (m <= 0.001f) return 0;
  if (m > 1.0f) m = 1.0f;
  return (uint8_t)((float)v * m + 0.5f);
}

// animFactor: 0.0..1.0（SOLID=1.0；呼吸燈用）
static void setLedScaled(uint8_t r, uint8_t g, uint8_t b, float animFactor = 1.0f) {
  if (!s_pixel) return;

 // Master OFF -> forced off
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

 // sync cache (avoid one-frame mismatch)
  s_lastNeoPixelColor = ((uint32_t)scaleBright(r, 1.0f) << 16) |
                        ((uint32_t)scaleBright(g, 1.0f) << 8)  |
                        (uint32_t)scaleBright(b, 1.0f);
  s_lastNeoPixelShowMs = millis();
}

void setLedOff()      { setLed(0,   0,   0); }
void setLedGreen()    { setLed(0,   150, 0); }
void setLedRed()      { setLed(150, 0,   0); }
void setLedBlue()     { setLed(0,   0,   150); }
void setLedPurple()   { setLed(100, 0,   120); }
void setLedWhiteLow() { setLed(30,  30,  30); }

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

void updateLedForPrice(float change24h, bool priceOk) {
 // If price invalid, show purple (API fail / no data)
  if (!priceOk) {
    s_ledAnimMode = LED_ANIM_SOLID;
    s_ledBaseR = 100; s_ledBaseG = 0; s_ledBaseB = 120;
    setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, 1.0f);
    return;
  }

 // --- Trend (near-zero hysteresis) ---
  const float ENTER = 0.02f;  // 超過 0.02% 才進入紅/綠
  const float EXIT  = 0.005f; // 小於 0.005% 才回到白/灰

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
  uint8_t br = 60, bg = 60, bb = 60; // neutral
  if (s_ledTrend == LED_UP)   { br = 0;   bg = 150; bb = 0; }
  if (s_ledTrend == LED_DOWN) { br = 150; bg = 0;   bb = 0; }

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

  if (!appRunning) return;

 // Price invalid: keep purple
  if (!priceOk) {
    setLedScaled(100, 0, 120, 1.0f);
    return;
  }

  if (s_ledAnimMode == LED_ANIM_SOLID) {
 // Keep solid in case master brightness changed
    setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, 1.0f);
    return;
  }

  uint32_t nowMs = millis();
 // Limit update rate to avoid spamming pixel.show()
  if (nowMs - s_ledAnimLastMs < 33) return; // ~30Hz
  s_ledAnimLastMs = nowMs;

  uint32_t period = (s_ledAnimMode == LED_ANIM_BREATHE_FAST) ? LED_BREATHE_FAST_PERIOD_MS : LED_BREATHE_SLOW_PERIOD_MS;
  float f = breatheFactor(nowMs, period);
  setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, f);
}

// ===== Background animator task =====

static void ledAnimTickFromTask() {
 // Keep service running even if app is not in normal mode (helps "board off" reassert).
  ledStatusService();

  bool appRunning = s_ctxAppRunning;
  bool priceOk = s_ctxPriceOk;

  if (!appRunning) return;

  if (!priceOk) {
    setLedScaled(100, 0, 120, 1.0f);
    return;
  }

  if (s_ledAnimMode == LED_ANIM_SOLID) {
    setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, 1.0f);
    return;
  }

  uint32_t nowMs = millis();
  uint32_t period = (s_ledAnimMode == LED_ANIM_BREATHE_FAST) ? LED_BREATHE_FAST_PERIOD_MS : LED_BREATHE_SLOW_PERIOD_MS;
  float f = breatheFactor(nowMs, period);
  setLedScaled(s_ledBaseR, s_ledBaseG, s_ledBaseB, f);
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
