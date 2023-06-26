# Freenove WS2812 Lib for ESP32

## Description
This is an Arduino library for controlling ws2812b led on esp32.

Based on the example program "led_strip" in IDF-SDK. The source code repository is here:
https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip


## Examples:

Here are some simple examples.

### Show Rainbow
This example make your strip show a flowing rainbow.
```
#include "Freenove_WS2812_Lib_for_ESP32.h"

#define LEDS_COUNT  8
#define LEDS_PIN	2
#define CHANNEL		0

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL);

void setup() {
  strip.begin();
}

void loop() {
  for (int j = 0; j < 255; j += 2) {
    for (int i = 0; i < LEDS_COUNT; i++) {
      strip.setLedColorData(i, strip.Wheel((i * 256 / LEDS_COUNT + j) & 255));
    }
    strip.show();
    delay(2);
  }  
}

```

## Usage
```
Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);
```
* Construction. Create a strip object.

```
strip.begin()
```
Initialization data, ready for communication.
```
strip.setLedColorData(id, color);
strip.setLedColorData(id, r, g, b);
```
* Send the color data of the specified LED to the controller. 
* Display color change after calling show function.
	* id: the index of led.
	* color: color value. egg, 0xFF0000 is RED color.
	* r,g,b: color value. 0-255.
```
strip.show();
```
* Immediately display the color data that has been sent.


```
strip.setLedColor(id, color);
strip.setLedColor(id, r, g, b);
```
* Send color data and display it immediately.
* It is equivalent to "strip.setLedColorData(id, color); strip.show();"
	* id: the index of led.
	* color: color value. egg, 0xFF0000 is RED color.
	* r,g,b: color value. 0-255.

```
strip.Wheel(i)
```
* A simple color picker.
	* i: 0-255.
<img src='extras/ColorWheel.jpg' width='100%'/>

