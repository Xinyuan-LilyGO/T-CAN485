/*
 * @Description: SD card test procedure.
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-08-18 15:33:23
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-24 09:59:08
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "pin_config.h"

File testFile;

bool SelfLocking_Flag = false;

void setup()
{
    Serial.begin(115200);

    // pinMode(37, INPUT_PULLUP);// MISO pull-up resistor
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS); // SPI boots
}

void loop()
{
    uint8_t cardType = 0;
    uint64_t cardSize = 0;
    uint8_t numSectors = 0;

    if (!SD.begin(SD_CS, SPI, 40000000)) // SD boots
    {
        SelfLocking_Flag = false;

        Serial.println("Detecting SD card");

        Serial.println("SD card initialization failed !");
        delay(100);

        Serial.println(".");
        delay(100);

        Serial.println(".");
        delay(100);

        Serial.println(".");
        delay(100);

        Serial.println(".");
        delay(100);

        Serial.println(".");
        delay(100);

        Serial.println(".");
        delay(100);
    }
    else
    {
        delay(50); // Wait for the SD card

        if (SelfLocking_Flag == false)
        {
            SelfLocking_Flag = true;
            delay(50);
        }

        Serial.println("SD card initialization successful !");
        delay(100);

        cardType = SD.cardType();
        cardSize = SD.cardSize() / (1024 * 1024);
        numSectors = SD.numSectors();
        switch (cardType)
        {
        case CARD_NONE:
            Serial.println("No SD card attached");
            delay(100);

            break;
        case CARD_MMC:
            Serial.print("SD Card Type: ");
            Serial.println("MMC");
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            delay(100);

            break;
        case CARD_SD:
            Serial.print("SD Card Type: ");
            Serial.println("SDSC");
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            delay(100);

            break;
        case CARD_SDHC:
            Serial.print("SD Card Type: ");
            Serial.println("SDHC");
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
            delay(100);

            break;
        default:
            Serial.println("UNKNOWN");
            delay(100);

            break;
        }
    }
    SD.end();
}