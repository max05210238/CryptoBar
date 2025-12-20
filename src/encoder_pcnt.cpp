// CryptoBar V0.99a
// Encoder performance optimization: fix slow response, direction issues, and erratic behavior.

#include "encoder_pcnt.h"

// ==================== Encoder (PCNT hardware decode) =====================
// V0.99a: Optimized PCNT hardware decoder for smooth Bourns PEC11R-S0024 encoder.
//
// Implementation notes:
// - Use proper quadrature decoding (X2 mode: CLK rising/falling edges)
// - GPIO 2 (CLK) and GPIO 1 (DT) - GPIO 5/6 don't support PCNT on ESP32-S3
// - Bourns PEC11R-S0024: 24 PPR, smooth (no detents)
// - Rate-limit emitted steps per loop so UI won't "jump" after long blocking ops.
//
// V0.99a improvements from V0.98:
// - Fixed GPIO pins: 5/6 → 1/2 (ESP32-S3 PCNT compatibility)
// - Fixed quadrature mode: PCNT_COUNT_INC → PCNT_COUNT_DEC (proper phase detection)
// - Swapped CLK/DT for correct direction
// - Optimized for smooth encoder: COUNTS=6 (1/8 rev = 1 step), FILTER=150, DIR_LOCK=10ms
// - Added EMI spike rejection for E-ink display interference
// - Added verbose debug mode for diagnostics

static const pcnt_unit_t    ENC_PCNT_UNIT = PCNT_UNIT_0;
static const pcnt_channel_t ENC_PCNT_CH   = PCNT_CHANNEL_0;

// V0.99a: Configuration for SMOOTH (no-detent) Bourns PEC11R-S0024
// Reduced filter for better responsiveness on smooth encoder
static const uint16_t ENC_PCNT_FILTER_VAL = 150;

// V0.99a: Set to 6 for smooth encoder (1/8 revolution = 1 step)
// Bourns PEC11R: 24 pulses/rev × 2 edges (X2 mode) = 48 counts/rev
// 48 counts/rev ÷ 8 = 6 counts per 1/8 revolution
static const int ENC_COUNTS_PER_DETENT = 6;

// Direction inverted: 0 = normal (CW = positive), 1 = reversed
static const int ENC_DIR_INVERT = 0;

// V0.99a: Minimal direction lock for smooth encoder
// Smooth encoders need fast direction changes during slow rotation
static const uint32_t ENC_DIR_LOCK_MS = 10;

// V0.99a: Debug mode disabled for production
// Set to 1 to enable basic debug, 2 for verbose mode
static const int ENC_DEBUG = 0;

static int      s_lastEncDir     = 0;
static uint32_t s_lastEncStepMs  = 0;
static int      s_encDetentAccum = 0;
static int      s_encBacklog     = 0;

static int s_clkPin = -1;
static int s_dtPin  = -1;

void encoderPcntBegin(int clkPin, int dtPin) {
  s_clkPin = clkPin;
  s_dtPin  = dtPin;

 // Ensure stable levels (mechanical encoders typically need pullups)
  pinMode(s_clkPin, INPUT_PULLUP);
  pinMode(s_dtPin,  INPUT_PULLUP);

  pcnt_config_t cfg = {};
  cfg.pulse_gpio_num = s_clkPin;
  cfg.ctrl_gpio_num  = s_dtPin;
  cfg.unit           = ENC_PCNT_UNIT;
  cfg.channel        = ENC_PCNT_CH;

 // Quadrature encoder configuration for proper phase detection
  // CLK rising edge: DT=HIGH → count down, DT=LOW → count up
  // CLK falling edge: DT=HIGH → count up, DT=LOW → count down
  cfg.pos_mode = PCNT_COUNT_DEC;
  cfg.neg_mode = PCNT_COUNT_INC;

 // Direction controlled by DT level
  cfg.lctrl_mode = PCNT_MODE_REVERSE;
  cfg.hctrl_mode = PCNT_MODE_KEEP;

  cfg.counter_h_lim = 32767;
  cfg.counter_l_lim = -32768;

  pcnt_unit_config(&cfg);

  pcnt_set_filter_value(ENC_PCNT_UNIT, ENC_PCNT_FILTER_VAL);
  pcnt_filter_enable(ENC_PCNT_UNIT);

  pcnt_counter_pause(ENC_PCNT_UNIT);
  pcnt_counter_clear(ENC_PCNT_UNIT);
  pcnt_counter_resume(ENC_PCNT_UNIT);

  s_encDetentAccum = 0;
  s_encBacklog     = 0;
  s_lastEncDir     = 0;
  s_lastEncStepMs  = 0;

  Serial.println("[ENC] V0.99a PCNT enabled");
  Serial.printf("[ENC] Config: Filter=%d APB, Counts/Detent=%d, DirInvert=%d, DirLock=%dms\n",
                ENC_PCNT_FILTER_VAL, ENC_COUNTS_PER_DETENT, ENC_DIR_INVERT, ENC_DIR_LOCK_MS);
  if (ENC_DEBUG >= 2) {
    Serial.println("[ENC] VERBOSE DEBUG MODE - prints every poll (ENC_DEBUG=2)");
  } else if (ENC_DEBUG == 1) {
    Serial.println("[ENC] DEBUG MODE ENABLED - will output encoder events");
  }
}

void encoderPcntPoll(bool appRunning, volatile int* stepAccum, portMUX_TYPE* mux) {
  if (!stepAccum || !mux) return;

  static uint32_t pollCount = 0;
  pollCount++;

 // During WiFi provisioning / transitional states, discard rotation so we don't apply
 // a pile of steps later when the UI resumes.
  if (!appRunning) {
    int16_t tmp = 0;
    pcnt_get_counter_value(ENC_PCNT_UNIT, &tmp);
    if (tmp != 0) pcnt_counter_clear(ENC_PCNT_UNIT);
    s_encDetentAccum = 0;
    s_encBacklog     = 0;
    return;
  }

  int16_t cnt = 0;
  pcnt_get_counter_value(ENC_PCNT_UNIT, &cnt);

  // Verbose debug mode: print every poll + GPIO raw values
  if (ENC_DEBUG >= 2) {
    static uint32_t lastDebugMs = 0;
    uint32_t nowMs = millis();

    // Print detailed info every 100ms to avoid flooding
    if (nowMs - lastDebugMs >= 100) {
      int clkLevel = digitalRead(s_clkPin);
      int dtLevel = digitalRead(s_dtPin);
      Serial.printf("[ENC] Poll #%lu @ %lums: PCNT=%d, CLK=%d, DT=%d, Accum=%d, Backlog=%d\n",
                    pollCount, nowMs, cnt, clkLevel, dtLevel, s_encDetentAccum, s_encBacklog);
      lastDebugMs = nowMs;
    }
  }

  if (cnt == 0) return;

  // V0.99a: EMI spike rejection - discard unrealistically large jumps
  // Bourns PEC11R at 30 RPM = ~10ms per detent, poll rate ~10ms
  // Maximum plausible counts per poll: ~16 (very fast rotation)
  // Anything larger is likely EMI from E-ink display
  const int EMI_SPIKE_THRESHOLD = 16;
  if (abs(cnt) > EMI_SPIKE_THRESHOLD) {
    if (ENC_DEBUG) {
      Serial.printf("[ENC] EMI SPIKE REJECTED: PCNT=%d (threshold=%d)\n",
                    cnt, EMI_SPIKE_THRESHOLD);
    }
    pcnt_counter_clear(ENC_PCNT_UNIT);
    return;
  }

 // Clear immediately so the counter won't overflow even if loop blocks.
  pcnt_counter_clear(ENC_PCNT_UNIT);

  int delta = (int)cnt;
  if (ENC_DIR_INVERT) delta = -delta;

  if (ENC_DEBUG && delta != 0) {
    Serial.printf("[ENC] Raw PCNT: %d (inverted: %d)\n", cnt, delta);
  }

  s_encDetentAccum += delta;

  int steps = 0;
  while (s_encDetentAccum >= ENC_COUNTS_PER_DETENT) {
    steps++;
    s_encDetentAccum -= ENC_COUNTS_PER_DETENT;
  }
  while (s_encDetentAccum <= -ENC_COUNTS_PER_DETENT) {
    steps--;
    s_encDetentAccum += ENC_COUNTS_PER_DETENT;
  }

  if (steps != 0) {
    s_encBacklog += steps;
    if (ENC_DEBUG) {
      Serial.printf("[ENC] Detent steps: %d, Backlog: %d, Accum: %d\n",
                    steps, s_encBacklog, s_encDetentAccum);
    }
  }

 // Rate limit so menus don't "teleport" after a long blocking call.
  int emit = s_encBacklog;
  const int MAX_EMIT = 3;
  if (emit > MAX_EMIT) emit = MAX_EMIT;
  if (emit < -MAX_EMIT) emit = -MAX_EMIT;

 // V0.99a: Improved bounce guard - less aggressive to allow legitimate quick direction changes
  // Only filter very fast reversals (< 30ms) with single step
  if (emit != 0) {
    int dir = (emit > 0) ? 1 : -1;
    uint32_t nowMs = millis();

    // V0.99a: Only filter if ALL conditions met:
    // 1. We have previous direction (not first movement)
    // 2. Direction changed
    // 3. Time since last < ENC_DIR_LOCK_MS (now 30ms instead of 150ms)
    // 4. Movement is single step (abs(emit) == 1, not <= 2)
    if (ENC_DIR_LOCK_MS > 0 &&  // Allow disabling lock by setting to 0
        s_lastEncDir != 0 &&
        dir != s_lastEncDir &&
        (nowMs - s_lastEncStepMs) < ENC_DIR_LOCK_MS &&
        abs(emit) == 1) {  // V0.99a: Only filter single-step reversals, not 2 steps

      // V0.99a: Only discard current emit, keep backlog for next poll
      // This is less aggressive than clearing everything
      if (ENC_DEBUG) {
        Serial.printf("[ENC] Bounce filter: quick reversal ignored (dir %d->%d, %dms)\n",
                      s_lastEncDir, dir, (int)(nowMs - s_lastEncStepMs));
      }
      emit = 0;
      // V0.99a: Don't clear s_encBacklog and s_encDetentAccum here - they might be legitimate
    } else {
      s_lastEncDir = dir;
      s_lastEncStepMs = nowMs;
    }
  }

  if (emit != 0) {
    s_encBacklog -= emit;

    if (ENC_DEBUG) {
      Serial.printf("[ENC] Emitting %d steps to UI (backlog remaining: %d)\n",
                    emit, s_encBacklog);
    }

    portENTER_CRITICAL(mux);
 *stepAccum += emit;
    portEXIT_CRITICAL(mux);
  }
}
