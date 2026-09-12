#include "wled.h"

/*
 * Captain America — shifting red/white/blue bands centred on the shield.
 * Brightest at the centre, fading out to the edges of the lit section.
 * Centre index is set in Settings > Usermods so it can be moved without recompiling.
 */

static uint16_t cap_center_led = 0;

static const uint32_t cap_band_colors[3] = {
  RGBW32(255,   0,   0, 0),   // red
  RGBW32(255, 255, 255, 0),   // white
  RGBW32(  0,  40, 255, 0)    // blue — a little green lifts it; pure blue reads dim beside white
};

static const char _data_FX_MODE_CAPTAIN_AMERICA[] PROGMEM =
  "Captain America@Shift speed,Span,Band width,,,Reverse;;!;1;sx=180,ix=63,c1=34";

static void modeCaptainAmerica() {
  // Band geometry.
  uint8_t  band_width  = 1 + ((uint32_t)SEGMENT.custom1 * 15) / 255;   // 1-16 LEDs per colour
  uint16_t band_period = band_width * 3;                               // one full red/white/blue cycle
  uint16_t half_span   = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;

  // Motion: advance the pattern one LED every step_ms.
  uint16_t step_ms = 2000 - ((uint32_t)SEGMENT.speed * 1960) / 255;    // ~0.5 to ~25 steps/sec
  uint16_t shift   = (strip.now / step_ms) % band_period;
  int      travel  = SEGMENT.check1 ? (int)shift : -(int)shift;

  SEGMENT.fill(0);

  for (int offset = -(int)half_span; offset <= (int)half_span; offset++) {
    int led_idx = (int)cap_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;   // wrap around the ring
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    // Which colour band this pixel currently falls in.
    int band_pos = (offset + travel) % (int)band_period;
    if (band_pos < 0) band_pos += band_period;
    uint32_t band_color = cap_band_colors[band_pos / band_width];

    // Linear falloff: full at the centre, zero at the edge of the section.
    uint16_t distance = abs(offset);
    uint8_t  level    = 255 - ((uint32_t)distance * 255) / half_span;

    SEGMENT.setPixelColor(led_idx, RGBW32(scale8(R(band_color), level),
                                          scale8(G(band_color), level),
                                          scale8(B(band_color), level), 0));
  }
}

class CaptainAmericaUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeCaptainAmerica, _data_FX_MODE_CAPTAIN_AMERICA);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = cap_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], cap_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the shield (0-711)</i>');"));
    }
};

const char CaptainAmericaUsermod::_name[]   PROGMEM = "CaptainAmerica";
const char CaptainAmericaUsermod::_center[] PROGMEM = "centerLed";

static CaptainAmericaUsermod captain_america_usermod;
REGISTER_USERMOD(captain_america_usermod);