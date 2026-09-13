#include "wled.h"

/*
 * Rock 'n Roll — stage lighting for the guitar-and-amp model.
 * A synthetic 4/4 beat: coloured hits trading between the two sides,
 * with a white strobe on every downbeat.
 * Centre index is set in Settings > Usermods.
 */

static uint16_t rock_center_led = 0;

static const uint32_t rock_beat_colors[4] = {
  RGBW32(255,   0,   0, 0),   // red — matches the guitar
  RGBW32(255,   0,  90, 0),   // hot pink
  RGBW32(255, 120,   0, 0),   // amber
  RGBW32(  0, 120, 255, 0)    // stage blue
};

static const char _data_FX_MODE_ROCK_N_ROLL[] PROGMEM =
  "Rock 'n Roll@Tempo,Span,Strobe,Sustain,,Trade sides;;!;1;sx=118,ix=49,c1=170,c2=110,o1=1";

static void modeRockNRoll() {
  uint16_t half_span = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;

  // Synthetic 4/4 clock. Swap this for a mic input later and the rest still works.
  uint16_t bpm       = 60 + ((uint32_t)SEGMENT.speed * 130) / 255;   // 60-190 BPM
  uint16_t beat_ms   = 60000 / bpm;
  uint32_t beat_idx  = strip.now / beat_ms;
  uint16_t into_beat = strip.now % beat_ms;

  // Each hit rings out for part of the beat, then decays.
  uint16_t hit_ms = 60 + ((uint32_t)SEGMENT.custom2 * (beat_ms - 60)) / 255;
  uint8_t  hit    = 0;
  if (into_beat < hit_ms) {
    hit = 255 - ((uint32_t)into_beat * 255) / hit_ms;
    hit = scale8(hit, hit);
  }

  // White strobe on the downbeat of each bar.
  const uint16_t strobe_ms = 70;
  uint8_t strobe = 0;
  if ((beat_idx & 3) == 0 && into_beat < strobe_ms) {
    strobe = scale8(255 - ((uint32_t)into_beat * 255) / strobe_ms, SEGMENT.custom1);
  }

  uint32_t beat_color  = rock_beat_colors[beat_idx & 3];
  uint8_t  active_side = beat_idx & 1;

  SEGMENT.fill(0);

  for (int offset = -(int)half_span; offset <= (int)half_span; offset++) {
    int led_idx = (int)rock_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    uint16_t distance = abs(offset);
    uint8_t  falloff  = 255 - ((uint32_t)distance * 255) / half_span;

    // The two halves trade the lead, like a pair of stage lights.
    uint8_t side_gain = 255;
    if (SEGMENT.check1) {
      uint8_t side = (offset < 0) ? 0 : 1;
      side_gain = (side == active_side) ? 255 : 70;
    }

    uint8_t level = scale8(scale8(falloff, hit), side_gain);

    uint8_t r = scale8(R(beat_color), level);
    uint8_t g = scale8(G(beat_color), level);
    uint8_t b = scale8(B(beat_color), level);

    // Strobe rides on top of everything, white and full width.
    if (strobe) {
      uint8_t white = scale8(falloff, strobe);
      if (white > r) r = white;
      if (white > g) g = white;
      if (white > b) b = white;
    }

    SEGMENT.setPixelColor(led_idx, RGBW32(r, g, b, 0));
  }
}

class RockNRollUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeRockNRoll, _data_FX_MODE_ROCK_N_ROLL);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = rock_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], rock_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the guitar and amp (0-711)</i>');"));
    }
};

const char RockNRollUsermod::_name[]   PROGMEM = "RockNRoll";
const char RockNRollUsermod::_center[] PROGMEM = "centerLed";

static RockNRollUsermod rock_n_roll_usermod;
REGISTER_USERMOD(rock_n_roll_usermod);