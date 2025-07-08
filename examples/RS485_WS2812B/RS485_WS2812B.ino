/*
 * @Description:
        RS485 Test
        Connect two devices, and on the computer serial port, you can see that the two 
    devices are sending digital self addition signals to each other
    (ESP32 Arduino IDE Library Version: arduino_esp32_v3.0.1)
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-06-21 18:42:16
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-24 09:58:46
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include <HardwareSerial.h>
#include "pin_config.h"
#include "FastLED.h"

#define NUM_LEDS 1
#define DATA_PIN WS2812B_DATA

static size_t CycleTime = 0;
static size_t temp_send_data = 0;

CRGB leds[NUM_LEDS];

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(ME2107_EN, OUTPUT);
    digitalWrite(ME2107_EN, HIGH);

    pinMode(RS485_CALLBACK, OUTPUT);
    pinMode(RS485_EN, OUTPUT);
    digitalWrite(RS485_EN, HIGH);
    digitalWrite(RS485_CALLBACK, HIGH); // 禁用输出接收到的信号 禁用回调

    // 初始化串口，并重新定义引脚
    // 参数包括串行通信的波特率、串行模式、使用的 RX 引脚和 TX 引脚。
    Serial1.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // GRB ordering is typical
    FastLED.setBrightness(50);
}

void loop()
{
    if (millis() > CycleTime)
    {
        leds[0] = CRGB::Blue;
        FastLED.show();
        delay(500);
        temp_send_data++;
        Serial1.printf("%d", temp_send_data);
        leds[0] = CRGB::Black;
        FastLED.show();

        CycleTime = millis() + 3000;
    }

    if (Serial1.available() > 0)
    {
        uint8_t temp_data[min(100, Serial1.available())] = {0};
        leds[0] = CRGB::Green;
        FastLED.show();
        delay(500);
        Serial1.read(temp_data, sizeof(temp_data));
        Serial.printf("Receive Data: ");
        for (int i = 0; i < sizeof(temp_data); i++)
        {
            Serial.printf("%c", temp_data[i]);
        }
        Serial.printf("\n");
        leds[0] = CRGB::Black;
        FastLED.show();
    }
}
