#include <Arduino.h>
#include "config.h"
#include <HardwareSerial.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <ESP32CAN.h>
#include <SPI.h>
#include <SD.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define LEDS_COUNT 1
#define LEDS_PIN 4
#define CHANNEL 0

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);

HardwareSerial Serial485(2);

CAN_device_t CAN_cfg;             // CAN Config
unsigned long previousMillis = 0; // will store last time a CAN Message was send
const int interval = 1000;        // interval at which send CAN Messages (milliseconds)
const int rx_queue_size = 10;     // Receive Queue size
HTTPClient http;

void WS2812Task(void *prarm);
void CANTask(void *prarm);
void WIFI_test(void *pvParameter);


void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                //listDir(fs, file., levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) {
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path)
{
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file) {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
            size_t toRead = len;
            if (toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}


void SD_test(void)
{
    SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SPI)) {
        Serial.println("SDCard MOUNT FAIL");
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        String str = "SDCard Size: " + String(cardSize) + "MB";
        Serial.println(str);
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/bledata/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/bledata", 0);

    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    testFileIO(SD, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));



}

void setup()
{
    pinMode(RS485_EN_PIN, OUTPUT);
    digitalWrite(RS485_EN_PIN, HIGH);

    pinMode(RS485_SE_PIN, OUTPUT);
    digitalWrite(RS485_SE_PIN, HIGH);

    pinMode(PIN_5V_EN, OUTPUT);
    digitalWrite(PIN_5V_EN, HIGH);

    pinMode(CAN_SE_PIN, OUTPUT);
    digitalWrite(CAN_SE_PIN, LOW);

    Serial.begin(115200);
    Serial485.begin(115200, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
    delay(5);
    Serial.println("factory test");
    // put your setup code here, to run once:
    SD_test();

    xTaskCreatePinnedToCore(WIFI_test, "WIFI_test", 1024 * 20, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(WS2812Task, "WS2812Task", 1024, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(CANTask, "CANTask", 1024 * 10, NULL, 3, NULL, 1);
}

void loop()
{
    while (Serial.available())
        Serial485.write(Serial.read());
    while (Serial485.available())
        Serial.write(Serial485.read());
}

void WS2812Task(void *prarm)
{
    strip.begin();
    strip.setBrightness(0x02);
    while (1) {
        for (int j = 0; j < 255; j += 2) {
            for (int i = 0; i < LEDS_COUNT; i++) {
                strip.setLedColorData(i, strip.Wheel((i * 256 / LEDS_COUNT + j) & 255));
            }
            strip.show();
            delay(5);
        }
    }
}

void CANTask(void *prarm)
{
    CAN_cfg.speed = CAN_SPEED_1000KBPS;
    CAN_cfg.tx_pin_id = GPIO_NUM_27;
    CAN_cfg.rx_pin_id = GPIO_NUM_26;
    CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
    // Init CAN Module
    ESP32Can.CANInit();

    Serial.print("CAN SPEED :");
    Serial.println(CAN_cfg.speed);

    while (1) {
        CAN_frame_t rx_frame;

        unsigned long currentMillis = millis();

        // Receive next CAN frame from queue
        if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {

            if (rx_frame.FIR.B.FF == CAN_frame_std) {
                printf("New standard frame");
            } else {
                printf("New extended frame");
            }

            if (rx_frame.FIR.B.RTR == CAN_RTR) {
                printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID, rx_frame.FIR.B.DLC);
            } else {
                printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID, rx_frame.FIR.B.DLC);
                for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
                    printf("0x%02X ", rx_frame.data.u8[i]);
                }
                printf("\n");
            }
        }
        // Send CAN Message
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            CAN_frame_t tx_frame;
            tx_frame.FIR.B.FF = CAN_frame_std;
            tx_frame.MsgID = 0x001;
            tx_frame.FIR.B.DLC = 8;
            tx_frame.data.u8[0] = 0x00;
            tx_frame.data.u8[1] = 0x01;
            tx_frame.data.u8[2] = 0x02;
            tx_frame.data.u8[3] = 0x03;
            tx_frame.data.u8[4] = 0x04;
            tx_frame.data.u8[5] = 0x05;
            tx_frame.data.u8[6] = 0x06;
            tx_frame.data.u8[7] = 0x07;

            ESP32Can.CANWriteFrame(&tx_frame);
        }
    }
}

void WIFI_test(void *pvParameter)
{
    bool isConnected = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("scan start");

    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }

        if (!isConnected) {
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
        while (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(500);
            Serial.print(".");
        }
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        isConnected = true;
    }

    Serial.println("");

    http.begin("https://www.baidu.com/");
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Serial.println(http.getString());
    }
    WiFi.disconnect();

    vTaskDelete(NULL);
}