#include "wled.h"

/*
 * Vader — red breathing glow centred on the helmet corner.
 * Brightest at the corner LED, fading to nothing on both sides.
 * The corner index is set in Settings > Usermods, so it can be
 * dialled in without recompiling.
 */

static uint16_t vader_center_led = 0;

static const char _data_FX_MODE_VADER[] PROGMEM =
  "Vader@Breath rate,Glow width;!,!;!;1;sx=80,ix=90";

static void modeVader() {
  // Breathing envelope: triangle wave, speed slider sets the period.
  uint16_t breath_ms = 1500 + (uint16_t)(255 - SEGMENT.speed) * 20;
  uint16_t half_ms   = breath_ms / 2;
  uint32_t cycle_pos = strip.now % breath_ms;

  uint8_t breath;
  if (cycle_pos < half_ms) breath = (cycle_pos * 255) / half_ms;
  else                     breath = 255 - (((cycle_pos - half_ms) * 255) / half_ms);

  // Never fully extinguish — the respirator keeps running.
  breath = 40 + scale8(breath, 215);

  // How many LEDs the glow reaches on each side of the corner.
  uint16_t glow_width = 1 + ((uint32_t)SEGMENT.intensity * 126) / 255;

  uint32_t base_color = SEGCOLOR(0);
  if (base_color == 0) base_color = RGBW32(255, 0, 0, 0);

  SEGMENT.fill(0);

  for (int offset = -(int)glow_width; offset <= (int)glow_width; offset++) {
    int led_idx = (int)vader_center_led + offset;
    while (led_idx < 0)       led_idx += SEGLEN;   // wrap around the ring
    while (led_idx >= SEGLEN) led_idx -= SEGLEN;

    // Linear falloff: full at the corner, zero at the edge of the glow.
    uint16_t distance = abs(offset);
    uint8_t  falloff  = 255 - ((uint32_t)distance * 255) / glow_width;
    uint8_t  level    = scale8(falloff, breath);

    SEGMENT.setPixelColor(led_idx, RGBW32(scale8(R(base_color), level),
                                          scale8(G(base_color), level),
                                          scale8(B(base_color), level), 0));
  }
}

class VaderUsermod : public Usermod {
  private:
    static const char _name[];
    static const char _center[];

  public:
    void setup() override {
      strip.addEffect(255, &modeVader, _data_FX_MODE_VADER);
    }

    void loop() override {}

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_center)] = vader_center_led;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      bool config_complete = !top.isNull();
      config_complete &= getJsonValue(top[FPSTR(_center)], vader_center_led, 0);
      return config_complete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('"));
      settingsScript.print(FPSTR(_name));
      settingsScript.print(F(":centerLed',1,'<i>LED index at the helmet corner (0-711)</i>');"));
    }
};

const char VaderUsermod::_name[]   PROGMEM = "Vader";
const char VaderUsermod::_center[] PROGMEM = "centerLed";

static VaderUsermod vader_usermod;
REGISTER_USERMOD(vader_usermod);