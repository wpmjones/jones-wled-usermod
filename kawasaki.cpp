#include "wled.h"

/*
 * Kawasaki — idling engine glow.
 * A steady green pool with quick, irregular flares on top, like a lumpy idle.
 * Centre index is set in Settings > Usermods.
 */

static uint16_t kawasaki_center_led = 0;

static const uint32_t kawasaki_green = RGBW32(90, 220, 0, 0);   // Kawasaki lime

static const char _data_FX_MODE_KAWASAKI[] PROGMEM =
  "Kawasaki@Idle rate,Span,Lumpiness,Base glow;;!;1;sx=150,ix=30,c1=140,c2=60";

static void modeKawasaki() {
  uint16_t half_span = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;

  // Gap between combustion flares, and how irregular that gap is.
  uint16_t base_gap = 320 - ((uint32_t)SEGMENT.speed * 300) / 255;   // ~20-320ms
  uint16_t jitter   = ((uint32_t)base_gap * SEGMENT.custom1) / 255;

  // SEGENV.step = when the current flare started
  // SEGENV.aux0 = how long to wait before the next one
  // SEGENV.aux1 = peak strength of the current flare
  uint32_t since_flare = strip.now - SEGENV.step;
  if (since_flare >= SEGENV.aux0) {
    SEGENV.step = strip.now;
    SEGENV.aux0 = base_gap + (jitter ? random(jitter) : 0);
    SEGENV.aux1 = random(160, 256);         // vary the punch of each firing
    since_flare = 0;
  }

  // Fast attack, sharp decay.
  const uint16_t flare_ms = 90;
  uint8_t flare = 0;
  if (since_flare < flare_ms) {
    flare = 255 - ((uint32_t)since_flare * 255) / flare_ms;
    flare = scale8(flare, flare);           // squared, for a snappier falloff
    flare = scale8(flare, SEGENV.aux1);
  }

  uint16_t engine = SEGMENT.custom2 + flare;   // steady idle glow plus the flare
  if (engine > 255) engine = 255;

  SEGMENT.fill(0);

  for (int offset = -(int)half_span; offset <= (int)half_span; offset++) {
    int led_idx = (int)kawasaki_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    uint16_t distance = abs(offset);
    uint8_t  falloff  = 255 - ((uint32_t)distance * 255) / half_span;
    uint8_t  level    = scale8(falloff, (uint8_t)engine);

    SEGMENT.setPixelColor(led_idx, RGBW32(scale8(R(kawasaki_green), level),
                                          scale8(G(kawasaki_green), level),
                                          scale8(B(kawasaki_green), level), 0));
  }
}

class KawasakiUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeKawasaki, _data_FX_MODE_KAWASAKI);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = kawasaki_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], kawasaki_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the Kawasaki (0-711)</i>');"));
    }
};

const char KawasakiUsermod::_name[]   PROGMEM = "Kawasaki";
const char KawasakiUsermod::_center[] PROGMEM = "centerLed";

static KawasakiUsermod kawasaki_usermod;
REGISTER_USERMOD(kawasaki_usermod);