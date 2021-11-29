#include <Arduino.h>
#include "config.h"
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>

HardwareSerial Serial485(2);

void SD_test(void)
{
  SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN))
  {
    Serial.println("SDCard MOUNT FAIL");
  }
  else
  {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
  }
}

void setup()
{
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);

  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);

  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

  Serial.begin(9600);
  Serial485.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  delay(5);
  Serial.println("test");
  Serial485.println("test");
  // put your setup code here, to run once:
  SD_test();
}

void loop()
{
  while (Serial.available())
    Serial485.write(Serial.read());
  while (Serial485.available())
    Serial.write(Serial485.read());
  // put your main code here, to run repeatedly:
}