#include "wled.h"

/*
 * Titanic — slow North Atlantic swell with icebergs drifting through.
 * Layered slow waves from deep blue to ice blue, pale bergs riding over the top.
 * Centre index is set in Settings > Usermods.
 */

static uint16_t titanic_center_led = 0;

// Water ramp: trough colour through to crest colour.
static const uint8_t titanic_deep_r =   0, titanic_deep_g =  18, titanic_deep_b =  80;
static const uint8_t titanic_ice_r  = 150, titanic_ice_g  = 225, titanic_ice_b  = 255;
// Iceberg colour: pale cyan-white.
static const uint8_t titanic_berg_r = 235, titanic_berg_g = 245, titanic_berg_b = 255;

static const char _data_FX_MODE_TITANIC[] PROGMEM =
  "Titanic@Current,Span,Icebergs,Swell;;!;1;sx=70,ix=79,c1=90,c2=200";

// 0-255 triangle wave. Layered at mismatched periods it reads as an organic swell.
static uint8_t triWave(uint32_t x, uint16_t period) {
  uint32_t p    = x % period;
  uint32_t half = period / 2;
  return (p < half) ? (p * 255) / half
                    : 255 - ((p - half) * 255) / half;
}

static void modeTitanic() {
  uint16_t half_span = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;
  uint16_t span_len  = half_span * 2 + 1;

  // Slow time bases. The water drifts; the bergs drift far slower.
  uint16_t time_div = 200 - ((uint32_t)SEGMENT.speed * 180) / 255;   // 20-200 ms per step
  uint32_t water_t  = strip.now / time_div;
  uint32_t berg_t   = strip.now / (time_div * 4);

  bool     bergs_on  = SEGMENT.custom1 > 10;      // slider bottom turns them off
  uint16_t berg_half = 2 + ((uint32_t)SEGMENT.custom1 * 14) / 255;
  const uint8_t berg_count = 3;

  SEGMENT.fill(0);

  for (uint16_t x = 0; x < span_len; x++) {
    int offset  = (int)x - (int)half_span;
    int led_idx = (int)titanic_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    // Three swells at different lengths and speeds, summed into one surface.
    uint16_t w1 = triWave(x * 3 + water_t,           37);
    uint16_t w2 = triWave(x * 2 + (water_t * 2) / 3, 61);
    uint16_t w3 = triWave(x     +  water_t / 2,      23);
    uint8_t  water = (uint8_t)((w1 + w2 + w3) / 3);

    // Swell slider compresses the surface toward its mean.
    int32_t centered = (int32_t)water - 128;
    centered = (centered * (int32_t)SEGMENT.custom2) / 255;
    water = (uint8_t)(128 + centered);

    uint8_t r = titanic_deep_r + (uint8_t)(((uint16_t)(titanic_ice_r - titanic_deep_r) * water) >> 8);
    uint8_t g = titanic_deep_g + (uint8_t)(((uint16_t)(titanic_ice_g - titanic_deep_g) * water) >> 8);
    uint8_t b = titanic_deep_b + (uint8_t)(((uint16_t)(titanic_ice_b - titanic_deep_b) * water) >> 8);

    // Icebergs drifting along the span, wrapping at the ends.
    if (bergs_on) {
      uint8_t berg = 0;
      for (uint8_t k = 0; k < berg_count; k++) {
        uint16_t berg_pos = (berg_t + k * (span_len / berg_count) + k * 7) % span_len;
        uint16_t d = (x > berg_pos) ? (x - berg_pos) : (berg_pos - x);
        if (d > span_len / 2) d = span_len - d;        // shortest way round
        if (d < berg_half) {
          uint8_t edge = 255 - (uint8_t)(((uint32_t)d * 255) / berg_half);
          edge = scale8(edge, edge);                   // soften the shoulders
          if (edge > berg) berg = edge;
        }
      }
      if (berg) {
        r += (uint8_t)(((uint16_t)(titanic_berg_r - r) * berg) >> 8);
        g += (uint8_t)(((uint16_t)(titanic_berg_g - g) * berg) >> 8);
        b += (uint8_t)(((uint16_t)(titanic_berg_b - b) * berg) >> 8);
      }
    }

    // Same centre-bright falloff as the other effects.
    uint16_t distance = (offset < 0) ? -offset : offset;
    uint8_t  falloff  = 255 - (uint8_t)(((uint32_t)distance * 255) / half_span);

    SEGMENT.setPixelColor(led_idx, RGBW32(scale8(r, falloff),
                                          scale8(g, falloff),
                                          scale8(b, falloff), 0));
  }
}

class TitanicUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeTitanic, _data_FX_MODE_TITANIC);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = titanic_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], titanic_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the Titanic (0-711)</i>');"));
    }
};

const char TitanicUsermod::_name[]   PROGMEM = "Titanic";
const char TitanicUsermod::_center[] PROGMEM = "centerLed";

static TitanicUsermod titanic_usermod;
REGISTER_USERMOD(titanic_usermod);