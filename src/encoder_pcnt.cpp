// CryptoBar V0.97 (refactor-only)
// Move PCNT rotary encoder decode out of main.cpp (no behavior change).

#include "encoder_pcnt.h"

// ==================== Encoder (PCNT hardware decode) =====================
// V0.97: PCNT hardware decoder for encoder rotation (baseline).
//
// Implementation notes:
// - Use CLK as PCNT pulse input, DT as PCNT control input (direction).
// - Count BOTH edges of CLK (pos/neg = INC) and reverse direction when DT is LOW.
// - Convert raw counts into "detents" (typically 2 counts per detent when counting CLK edges only).
// - Rate-limit emitted steps per loop so UI won't "jump" after long blocking ops.

static const pcnt_unit_t    ENC_PCNT_UNIT = PCNT_UNIT_0;
static const pcnt_channel_t ENC_PCNT_CH   = PCNT_CHANNEL_0;

// Max PCNT filter is 1023 APB cycles (~12.8us @80MHz). Helps with short glitches/EMI.
static const uint16_t ENC_PCNT_FILTER_VAL = 1023;

// Usually 2 counts per detent when counting both edges on CLK only.
static const int ENC_COUNTS_PER_DETENT = 2;

// If direction is reversed, set to 1.
static const int ENC_DIR_INVERT = 1;

// Ignore quick reversal as bounce.
static const uint32_t ENC_DIR_LOCK_MS = 150;

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

 // Count both edges on CLK
  cfg.pos_mode = PCNT_COUNT_INC;
  cfg.neg_mode = PCNT_COUNT_INC;

 // Reverse direction when DT is LOW
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

  Serial.println("[ENC] PCNT enabled");
}

void encoderPcntPoll(bool appRunning, volatile int* stepAccum, portMUX_TYPE* mux) {
  if (!stepAccum || !mux) return;

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
  if (cnt == 0) return;

 // Clear immediately so the counter won't overflow even if loop blocks.
  pcnt_counter_clear(ENC_PCNT_UNIT);

  int delta = (int)cnt;
  if (ENC_DIR_INVERT) delta = -delta;

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

  if (steps != 0) s_encBacklog += steps;

 // Rate limit so menus don't "teleport" after a long blocking call.
  int emit = s_encBacklog;
  const int MAX_EMIT = 3;
  if (emit > MAX_EMIT) emit = MAX_EMIT;
  if (emit < -MAX_EMIT) emit = -MAX_EMIT;

 // Bounce guard: ignore a quick single-step reversal caused by encoder chatter.
  if (emit != 0) {
    int dir = (emit > 0) ? 1 : -1;
    uint32_t nowMs = millis();

    if (s_lastEncDir != 0 && dir != s_lastEncDir && (nowMs - s_lastEncStepMs) < ENC_DIR_LOCK_MS && abs(emit) <= 2) {
 // Treat as bounce.
 // IMPORTANT: also discard the pending opposite-direction backlog.
      emit = 0;
      s_encBacklog = 0;
      s_encDetentAccum = 0;
    } else {
      s_lastEncDir = dir;
      s_lastEncStepMs = nowMs;
    }
  }

  if (emit != 0) {
    s_encBacklog -= emit;

    portENTER_CRITICAL(mux);
 *stepAccum += emit;
    portEXIT_CRITICAL(mux);
  }
}
