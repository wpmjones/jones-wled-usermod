#include "wled.h"

/*
 * DeLorean — the Back to the Future time-travel sequence, on a loop.
 * Flux capacitor charging -> electrical arcs -> white flash -> two fire
 * trails racing outward and burning out.
 * Centre index is set in Settings > Usermods.
 */

static uint16_t delorean_center_led = 0;

static const char _data_FX_MODE_DELOREAN[] PROGMEM =
  "DeLorean@Interval,Span,Arcs,Idle glow,Flux rate;;!;1;sx=159,ix=39,c1=170,c2=120,c3=16";

// Phase lengths in ms. The flux-capacitor idle fills whatever is left of the cycle.
static const uint16_t delorean_arc_ms   = 1400;
static const uint16_t delorean_flash_ms =  140;
static const uint16_t delorean_fire_ms  = 1600;

// Cheap deterministic hash, so arcs jump around without needing stored state.
static uint16_t arcHash(uint32_t v) {
  v ^= v >> 13;
  v *= 0x9E3779B1UL;
  v ^= v >> 11;
  return (uint16_t)(v >> 8);
}

// Orange ramp for the fire trails.
static void trailColor(uint8_t heat, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (heat < 128) {
    r = (uint8_t)(((uint16_t)heat * 255) / 128);
    g = (uint8_t)(((uint16_t)heat *  40) / 128);
    b = 0;
  } else {
    r = 255;
    g = 40 + (uint8_t)(((uint16_t)(heat - 128) * 190) / 127);
    b =      (uint8_t)(((uint16_t)(heat - 128) *  90) / 127);
  }
}

static void modeDeLorean() {
  uint16_t half_span = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;
  uint16_t span_len  = half_span * 2 + 1;

  const uint16_t event_ms = delorean_arc_ms + delorean_flash_ms + delorean_fire_ms;
  uint32_t cycle_ms = 22000 - ((uint32_t)SEGMENT.speed * 16000) / 255;   // 6-22 s
  uint32_t t        = strip.now % cycle_ms;
  uint32_t idle_ms  = cycle_ms - event_ms;

  uint32_t arc_start   = idle_ms;
  uint32_t flash_start = arc_start + delorean_arc_ms;
  uint32_t fire_start  = flash_start + delorean_flash_ms;

  SEGMENT.fill(0);

  for (int offset = -(int)half_span; offset <= (int)half_span; offset++) {
    int led_idx = (int)delorean_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    uint16_t distance = (offset < 0) ? -offset : offset;
    uint8_t  falloff  = 255 - (uint8_t)(((uint32_t)distance * 255) / half_span);

    uint8_t r = 0, g = 0, b = 0;

    if (t < arc_start) {
      // --- Phase 1: flux capacitor charging -------------------------------
      // Three pulses converging on the hub, speeding up as the charge builds.
      uint8_t charge = (uint8_t)(((uint32_t)t * 255) / idle_ms);

      uint8_t flux_rate = SEGMENT.custom3;
      if (flux_rate > 31) flux_rate = 31;               // custom3 is a 5-bit slider
      uint16_t slow_ms  = 1400 - (uint16_t)flux_rate * 30;
      uint16_t fast_ms  =  300 - (uint16_t)flux_rate *  6;
      uint16_t pulse_ms = slow_ms - ((uint32_t)(slow_ms - fast_ms) * charge) / 255;

      uint16_t pulse_w = 1 + half_span / 12;
      uint32_t phase   = strip.now % pulse_ms;

      uint8_t flux = 0;
      for (uint8_t k = 0; k < 3; k++) {
        uint32_t p          = (phase + (uint32_t)pulse_ms * k / 3) % pulse_ms;
        uint8_t  progress   = (uint8_t)((p * 255) / pulse_ms);
        uint16_t pulse_dist = half_span - ((uint32_t)progress * half_span) / 255;
        uint16_t d = (distance > pulse_dist) ? (distance - pulse_dist)
                                             : (pulse_dist - distance);
        if (d <= pulse_w) {
          uint8_t v = 255 - (uint8_t)(((uint32_t)d * 255) / (pulse_w + 1));
          v = scale8(v, v);
          if (v > flux) flux = v;
        }
      }
      if (distance <= 1 && flux < 110) flux = 110;      // the hub itself stays lit

      uint8_t bed = scale8(SEGMENT.custom2, 90 + scale8(charge, 165));
      bed = scale8(bed, falloff);

      uint8_t pulse_level = scale8(scale8(flux, 130 + scale8(charge, 125)), falloff);

      uint8_t br = scale8( 30, bed),         bg = scale8(110, bed),         bb = scale8(255, bed);
      uint8_t pr = scale8(190, pulse_level), pg = scale8(220, pulse_level), pb = scale8(255, pulse_level);
      r = (pr > br) ? pr : br;
      g = (pg > bg) ? pg : bg;
      b = (pb > bb) ? pb : bb;

    } else if (t < flash_start) {
      // --- Phase 2: electrical arcs ---------------------------------------
      uint8_t  build     = (uint8_t)(((t - arc_start) * 255) / delorean_arc_ms);
      uint32_t bucket    = strip.now / 55;
      uint8_t  arc_count = 2 + scale8(6, SEGMENT.custom1);
      uint16_t x         = (uint16_t)(offset + (int)half_span);   // 0-based position

      uint8_t arc = 0;
      for (uint8_t k = 0; k < arc_count; k++) {
        uint16_t arc_pos = arcHash(bucket * 7 + k) % span_len;
        uint16_t arc_w   = 1 + (arcHash(bucket * 13 + k + 99) % 4);
        uint16_t d       = (x > arc_pos) ? (x - arc_pos) : (arc_pos - x);
        if (d <= arc_w) {
          uint8_t a = 255 - (uint8_t)(((uint32_t)d * 255) / (arc_w + 1));
          if (a > arc) arc = a;
        }
      }

      uint8_t level = scale8(scale8(arc, 60 + scale8(build, 195)), falloff);
      uint8_t under = scale8(scale8(SEGMENT.custom2, 140 + scale8(build, 115)), falloff);

      uint8_t ar = scale8(150, level), ag = scale8(210, level), ab = scale8(255, level);
      uint8_t ur = scale8( 30, under), ug = scale8(110, under), ub = scale8(255, under);
      r = (ar > ur) ? ar : ur;
      g = (ag > ug) ? ag : ug;
      b = (ab > ub) ? ab : ub;

    } else if (t < fire_start) {
      // --- Phase 3: the flash ---------------------------------------------
      uint8_t fade  = 255 - (uint8_t)(((t - flash_start) * 255) / delorean_flash_ms);
      uint8_t level = scale8(fade, 140 + scale8(falloff, 115));
      r = level; g = level; b = level;

    } else {
      // --- Phase 4: fire trails -------------------------------------------
      uint8_t  fire_p   = (uint8_t)(((t - fire_start) * 255) / delorean_fire_ms);
      uint16_t head     = ((uint32_t)fire_p * half_span) / 255;
      uint16_t tail_len = 6 + half_span / 4;

      if (distance <= head) {
        uint16_t behind = head - distance;
        if (behind < tail_len) {
          uint8_t heat = 255 - (uint8_t)(((uint32_t)behind * 255) / tail_len);
          heat = scale8(heat, 255 - fire_p);      // the whole thing burns out
          trailColor(heat, r, g, b);
        }
      }
    }

    SEGMENT.setPixelColor(led_idx, RGBW32(r, g, b, 0));
  }
}

class DeLoreanUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeDeLorean, _data_FX_MODE_DELOREAN);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = delorean_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], delorean_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the DeLorean (0-711)</i>');"));
    }
};

const char DeLoreanUsermod::_name[]   PROGMEM = "DeLorean";
const char DeLoreanUsermod::_center[] PROGMEM = "centerLed";

static DeLoreanUsermod delorean_usermod;
REGISTER_USERMOD(delorean_usermod);