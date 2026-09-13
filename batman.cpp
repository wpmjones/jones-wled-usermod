#include "wled.h"

/*
 * Batman — the 1966 Batmobile's atomic turbine exhaust.
 * A continuous churning flame: deep red at the edges, orange through the
 * middle, white-hot at the core. Centre index is set in Settings > Usermods.
 */

static uint16_t batman_center_led = 0;

static const char _data_FX_MODE_BATMAN[] PROGMEM =
  "Batman@Churn,Span,Turbulence,Core heat;;!;1;sx=150,ix=59,c1=115,c2=235";

// 0-255 triangle wave; layered at mismatched periods it reads as turbulence.
static uint8_t flameWave(uint32_t x, uint16_t period) {
  uint32_t p    = x % period;
  uint32_t half = period / 2;
  return (p < half) ? (p * 255) / half
                    : 255 - ((p - half) * 255) / half;
}

// Three-band fire ramp: black -> red -> orange -> hot white.
static void heatToColor(uint8_t heat, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (heat < 85) {
    r = (uint8_t)(((uint16_t)heat * 255) / 85);
    g = 0;
    b = 0;
  } else if (heat < 170) {
    r = 255;
    g = (uint8_t)(((uint16_t)(heat - 85) * 255) / 85);
    b = 0;
  } else {
    r = 255;
    g = 255;
    b = (uint8_t)(((uint16_t)(heat - 170) * 220) / 85);   // warm white, not pure
  }
}

static void modeBatman() {
  uint16_t half_span = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;

  uint16_t time_div = 60 - ((uint32_t)SEGMENT.speed * 50) / 255;   // 10-60 ms per step
  uint32_t flame_t  = strip.now / time_div;

  // Global roar: an overall surge refreshed ~25x a second.
  // Per-pixel randomness would read as static; this reads as a burn.
  if (SEGENV.aux0 == 0 || strip.now - SEGENV.step > 40) {
    SEGENV.step = strip.now;
    SEGENV.aux0 = random(200, 256);
  }
  uint8_t roar = (uint8_t)SEGENV.aux0;

  uint8_t turbulence = SEGMENT.custom1;

  SEGMENT.fill(0);

  for (int offset = -(int)half_span; offset <= (int)half_span; offset++) {
    int led_idx = (int)batman_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    uint16_t distance = (offset < 0) ? -offset : offset;

    // Heat falls off from the turbine mouth at the centre.
    uint8_t pos_heat = 255 - (uint8_t)(((uint32_t)distance * 255) / half_span);
    pos_heat = scale8(pos_heat, SEGMENT.custom2);

    // Three layers of turbulence rolling through the flame.
    uint16_t f1 = flameWave(distance * 5 + flame_t,     29);
    uint16_t f2 = flameWave(distance * 3 + flame_t * 2, 43);
    uint16_t f3 = flameWave(distance * 8 + flame_t / 2, 17);
    uint8_t  churn = (uint8_t)((f1 + f2 + f3) / 3);

    // Turbulence slider decides how much the churn bites into the heat.
    uint8_t modulation = (255 - turbulence) + scale8(churn, turbulence);
    uint8_t heat = scale8(scale8(pos_heat, modulation), roar);

    uint8_t r, g, b;
    heatToColor(heat, r, g, b);

    SEGMENT.setPixelColor(led_idx, RGBW32(r, g, b, 0));
  }
}

class BatmanUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeBatman, _data_FX_MODE_BATMAN);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = batman_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], batman_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the Batmobile (0-711)</i>');"));
    }
};

const char BatmanUsermod::_name[]   PROGMEM = "Batman";
const char BatmanUsermod::_center[] PROGMEM = "centerLed";

static BatmanUsermod batman_usermod;
REGISTER_USERMOD(batman_usermod);