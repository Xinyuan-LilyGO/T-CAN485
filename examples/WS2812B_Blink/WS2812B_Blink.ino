/*
 * @Description: WS2812 Test
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-07-03 17:37:09
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-24 10:00:09
 * @License: GPL 3.0
 */
#include "FastLED.h"
#include "pin_config.h"

#define NUM_LEDS 1
#define DATA_PIN WS2812B_DATA

CRGB leds[NUM_LEDS];

void setup()
{
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // GRB ordering is typical
    FastLED.setBrightness(50);
}

void loop()
{
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);

    leds[0] = CRGB::Green;
    FastLED.show();
    delay(500);

    leds[0] = CRGB::Blue;
    FastLED.show();
    delay(500);

    leds[0] = CRGB::White;
    FastLED.show();
    delay(500);

    leds[0] = CRGB::Black;
    FastLED.show();
    delay(500);
}
