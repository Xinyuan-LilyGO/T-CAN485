#include <Arduino.h>
#include <HardwareSerial.h>
#include "pin_config.h"

// RS485
#define RS485_TX 22
#define RS485_RX 21
#define RS485_RE_PIN 17   //MAX13487E RE pin
#define RS485_SHUTDOWN_PIN 19 //MAX13487E shutdown pin
// WS2812B
#define WS2812B_DATA 4
// CAN
#define CAN_TX 27
#define CAN_RX 26
// RS485 and CAN Boost power supply
#define BOOST_ENABLE_PIN 16
//SD
#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13

#define RS485 Serial1 

void setup()
{
    Serial.begin(115200);

    // enable boost power chip
    pinMode(BOOST_ENABLE_PIN, OUTPUT);
    digitalWrite(BOOST_ENABLE_PIN, HIGH);

    pinMode(RS485_RE_PIN, OUTPUT);  
    pinMode(RS485_SHUTDOWN_PIN, OUTPUT);  

    digitalWrite(RS485_SHUTDOWN_PIN, HIGH); // Shutdown Disable , 1:Enable RS485 , 0:Disable RS485
    digitalWrite(RS485_RE_PIN, HIGH);       // Receive Enable

    RS485.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);

}

void loop()
{
    uint8_t request[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09};
    delay(10);
    RS485.write(request, sizeof(request));
    RS485.flush();
    delay(100);
    uint8_t response[20];
    int index = 0;
    unsigned long startTime = millis();
    while ((millis() - startTime) < 500) {
        if (RS485.available()) {
            response[index++] = RS485.read();
            if (index >= sizeof(response)) break;
        }
    }

    Serial.print("Raw Modbus response: ");
    for (int i = 0; i < index; i++) {
        Serial.printf("%02X ", response[i]);
    }
    Serial.println();
}
