/*
 * @Description: 
        Factory testing
        Connect both RS485 and CAN buses simultaneously, wait for all initializations to complete, 
    then press the BOOT button on one of the boards to activate the test program. You can observe
    the communication between the two devices in the computer window. If there is an error in 
    the RS485 communication, all communication will stop and a red light will turn on.
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-08-16 15:19:01
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-24 10:34:14
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include "pin_config.h"
#include <HardwareSerial.h>
#include "FastLED.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "driver/twai.h"

#define WIFI_SSID "xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428"
// #define WIFI_SSID "LilyGo-AABB"
// #define WIFI_PASSWORD "xinyuandianzi"

#define WIFI_CONNECT_WAIT_MAX (5000)

#define NUM_LEDS 1
#define DATA_PIN WS2812B_DATA

// Intervall:
#define TRANSMIT_RATE_MS 1000
#define POLLING_RATE_MS 1000

bool Wifi_Connection_Failure_Flag = false;

static size_t CycleTime = 0;
static uint8_t Uart_Buf[105] = {0};

static uint32_t Uart_Count = 0;

static bool SelfLocking_Flag = false;

static uint8_t Uart_Data[] = {

    0x0A, // 设备标头识别

    // 动态数据计数
    0B00000000, // 数据高2位
    0B00000000, // 数据高1位
    0B00000000, // 数据低2位
    0B00000000, // 数据低1位

    // 100个数据包
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
};

const char *fileDownloadUrl = "http://music.163.com/song/media/outer/url?id=26122999.mp3";

CRGB leds[NUM_LEDS];

void Wifi_STA_Test(void)
{
    String text;
    int wifi_num = 0;

    Serial.println("\nScanning wifi");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    wifi_num = WiFi.scanNetworks();
    if (wifi_num == 0)
    {
        text = "\nWiFi scan complete !\nNo wifi discovered.\n";
    }
    else
    {
        text = "\nWiFi scan complete !\n";
        text += wifi_num;
        text += " wifi discovered.\n\n";

        for (int i = 0; i < wifi_num; i++)
        {
            text += (i + 1);
            text += ": ";
            text += WiFi.SSID(i);
            text += " (";
            text += WiFi.RSSI(i);
            text += ")";
            text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
            delay(10);
        }
    }

    Serial.println(text);

    delay(3000);
    text.clear();

    text = "Connecting to ";
    Serial.print("Connecting to ");
    text += WIFI_SSID;
    text += "\n";

    Serial.print(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t last_tick = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        text += ".";
        delay(100);

        if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX)
        {
            Wifi_Connection_Failure_Flag = true;
            break;
        }
    }

    if (!Wifi_Connection_Failure_Flag)
    {
        text += "\nThe connection was successful ! \nTakes ";
        Serial.print("\nThe connection was successful ! \nTakes ");
        text += millis() - last_tick;
        Serial.print(millis() - last_tick);
        text += " ms\n";
        Serial.println(" ms\n");
    }
    else
    {
        Serial.printf("\nWifi test error!\n");
    }
}

bool Uart_Check_Dynamic_Data(uint8_t *uart_data, uint32_t check_data)
{
    uint32_t temp;

    temp = (uint32_t)uart_data[1] << 24 | (uint32_t)uart_data[2] << 16 |
           (uint32_t)uart_data[3] << 8 | (uint32_t)uart_data[4];

    if (temp == check_data)
    {
        return true;
    }

    return false;
}

bool Uart_Check_Static_Data(uint8_t *uart_data)
{
    if (memcmp(&uart_data[5], &Uart_Data[5], 100) == 0)
    {
        return true;
    }
    return false;
}

void SD_Test()
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

void CAN_Drive_Initialization()
{
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        Serial.println("Driver installed");
    }
    else
    {
        Serial.println("Failed to install driver");
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        Serial.println("Driver started");
    }
    else
    {
        Serial.println("Failed to start driver");
    }

    // 配置
    uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
                                TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                                TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_DATA |
                                TWAI_ALERT_RX_QUEUE_FULL;

    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
    {
        Serial.println("CAN Alerts reconfigured");
    }
    else
    {
        Serial.println("Failed to reconfigure alerts");
    }
}

void Twai_Send_Message()
{
    // Configure message to transmit
    twai_message_t message;
    message.extd = 0; // 标准帧
    message.rtr = 0;  // 非远程帧
    message.identifier = 0xF1;
    // message.data_length_code = 1;
    // message.data[0] = CAN_Count++;
    message.data_length_code = 8;
    message.data[0] = 1;
    message.data[1] = 2;
    message.data[2] = 3;
    message.data[3] = 4;
    message.data[4] = 5;
    message.data[5] = 6;
    message.data[6] = 7;
    message.data[7] = 8;

    // Queue message for transmission
    if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
    {
        printf("Message queued for transmission\n");
    }
    else
    {
        printf("Failed to queue message for transmission\n");
    }
}

void Twai_Receive_Message(twai_message_t &message)
{
    // Process received message
    if (message.extd)
    {
        Serial.println("Message is in Extended Format");
    }
    else
    {
        Serial.println("Message is in Standard Format");
    }
    Serial.printf("ID: 0x%X\n", message.identifier);
    if (!(message.rtr))
    {
        for (int i = 0; i < message.data_length_code; i++)
        {
            Serial.printf("Data [%d] = %d\n", i, message.data[i]);
        }
        Serial.println("");
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(0, INPUT_PULLUP);

    pinMode(ME2107_EN, OUTPUT);
    digitalWrite(ME2107_EN, HIGH);

    pinMode(RS485_CALLBACK, OUTPUT);
    pinMode(RS485_EN, OUTPUT);
    digitalWrite(RS485_EN, HIGH);
    digitalWrite(RS485_CALLBACK, HIGH); // 禁用输出接收到的信号 禁用回调

    // High speed mode
    pinMode(CAN_SPEED_MODE, OUTPUT);
    digitalWrite(CAN_SPEED_MODE, LOW);

    CAN_Drive_Initialization();

    // 初始化串口，并重新定义引脚
    // 参数包括串行通信的波特率、串行模式、使用的 RX 引脚和 TX 引脚。
    Serial1.begin(921600, SERIAL_8N1, RS485_RX, RS485_TX);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // GRB ordering is typical
    FastLED.setBrightness(20);

    leds[0] = CRGB::Red;
    FastLED.show();
    delay(1000);

    leds[0] = CRGB::Green;
    FastLED.show();
    delay(1000);

    leds[0] = CRGB::Blue;
    FastLED.show();
    delay(1000);

    leds[0] = CRGB::White;
    FastLED.show();
    delay(1000);

    Wifi_STA_Test();
    delay(2000);

    if (!Wifi_Connection_Failure_Flag)
    {
        // 初始化HTTP客户端
        HTTPClient http;
        http.begin(fileDownloadUrl);
        // 获取重定向的URL
        const char *headerKeys[] = {"Location"};
        http.collectHeaders(headerKeys, 1);

        // 记录下载开始时间
        size_t startTime = millis();
        // 无用时间
        size_t uselessTime = 0;

        // 发起GET请求
        int httpCode = http.GET();

        while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND)
        {
            String newUrl = http.header("Location");
            Serial.printf("Redirecting to: %s\n", newUrl.c_str());
            http.end(); // 关闭旧的HTTP连接

            // 使用新的URL重新发起GET请求
            http.begin(newUrl);
            httpCode = http.GET();
        }

        if (httpCode == HTTP_CODE_OK)
        {
            // 获取文件大小
            size_t fileSize = http.getSize();
            Serial.printf("Starting file download...\n");
            Serial.printf("file size: %f Mb\n", fileSize / 1024.0 / 1024.0);

            // 读取HTTP响应
            WiFiClient *stream = http.getStreamPtr();

            size_t temp_count_s = 0;
            size_t temp_fileSize = fileSize;
            uint8_t *buf_1 = (uint8_t *)heap_caps_malloc(100 * 1024, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            // uint8_t buf_1[4096] = {0};
            CycleTime = millis() + 1000; // 开始计时
            bool temp_count_flag = true;
            while (http.connected() && (fileSize > 0 || fileSize == -1))
            {
                // 获取可用数据的大小
                size_t availableSize = stream->available();
                if (availableSize)
                {
                    temp_fileSize -= stream->read(buf_1, min(availableSize, (size_t)(100 * 1024)));

                    if (millis() > CycleTime)
                    {
                        size_t temp_time_1 = millis();
                        temp_count_s++;
                        Serial.printf("Download speed: %f Kb/s\n", ((fileSize - temp_fileSize) / 1024.0) / temp_count_s);
                        Serial.printf("Remaining file size: %f Mb\n\n", temp_fileSize / 1024.0 / 1024.0);

                        CycleTime = millis() + 1000;
                        size_t temp_time_2 = millis();

                        uselessTime = uselessTime + (temp_time_2 - temp_time_1);
                    }
                }
                // delay(1);

                if (temp_fileSize == 0)
                {
                    break;
                }

                if (temp_count_s > 30)
                {
                    temp_count_flag = false;
                    break;
                }
            }

            // 关闭HTTP客户端
            http.end();

            // 记录下载结束时间并计算总花费时间
            size_t endTime = millis();

            if (temp_count_flag == true)
            {
                Serial.printf("Download completed!\n");
                Serial.printf("Total download time: %f s\n", (endTime - startTime - uselessTime) / 1000.0);
                Serial.printf("Average download speed: %f Kb/s\n", (fileSize / 1024.0) / ((endTime - startTime - uselessTime) / 1000.0));
            }
            else
            {
                Serial.printf("Download incomplete!\n");
                Serial.printf("Download time: %f s\n", (endTime - startTime - uselessTime) / 1000.0);
                Serial.printf("Average download speed: %f Kb/s\n", ((fileSize - temp_fileSize) / 1024.0) / ((endTime - startTime - uselessTime) / 1000.0));
            }
        }
        else
        {
            Serial.printf("Failed to download\n");
            Serial.printf("Error httpCode: %d \n", httpCode);
        }
    }
    else
    {
        Serial.print("Not connected to the network");
    }
    delay(1000);

    leds[0] = CRGB::Purple;
    FastLED.show();
    delay(1000);

    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS); // SPI boots
    SD_Test();

    leds[0] = CRGB::Black;
    FastLED.show();

    while (Serial1.available() > 0) // 清空缓存
    {
        Serial1.read();
    }
}

void loop()
{
    if (digitalRead(0) == 0)
    {
        delay(300);

        leds[0] = CRGB::Blue;
        FastLED.show();
        delay(1000);
        Serial1.write(Uart_Data, 105);

        Uart_Count++;
        leds[0] = CRGB::Black;
        FastLED.show();

        Twai_Send_Message();
        Serial.println("CAN send done");
    }

    while (Serial1.available() > 0)
    {
        delay(1000); // 接收等待
        Serial1.read(Uart_Buf, sizeof(Uart_Buf));

        if (Uart_Buf[0] == 0x0A)
        {
            if (Uart_Check_Dynamic_Data(Uart_Buf, Uart_Count) == false) // 动态数据校验
            {
                leds[0] = CRGB::Red;
                FastLED.show();
                while (1)
                {
                    Serial.printf("Check Dynamic Data Failed\n");
                    Serial.printf("Check Data: %d\n", Uart_Count);
                    Serial.printf("Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                              (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
                    Serial.printf("Received Buf[1]: %#X\n", Uart_Buf[1]);
                    Serial.printf("Received Buf[2]: %#X\n", Uart_Buf[2]);
                    Serial.printf("Received Buf[3]: %#X\n", Uart_Buf[3]);
                    Serial.printf("Received Buf[4]: %#X\n", Uart_Buf[4]);
                    delay(1000);
                }
            }
            else if (Uart_Check_Static_Data(Uart_Buf) == false) // 静态数据校验
            {
                leds[0] = CRGB::Pink;
                FastLED.show();
                Serial.printf("Check Static Data Failed\n");
                for (int i = 0; i < 100; i++)
                {
                    Serial.printf("Received Buf[%d]: %#X\n", i + 5, Uart_Buf[i + 5]);
                }
                delay(1000);
            }
            else
            {
                leds[0] = CRGB::Green;
                FastLED.show();
                delay(500);

                Serial.printf("Check Data Successful\n");
                Serial.printf("Check Data: %d\n", Uart_Count);
                Serial.printf("Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                          (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
                Serial.printf("Received Buf[1]: %#X\n", Uart_Buf[1]);
                Serial.printf("Received Buf[2]: %#X\n", Uart_Buf[2]);
                Serial.printf("Received Buf[3]: %#X\n", Uart_Buf[3]);
                Serial.printf("Received Buf[4]: %#X\n", Uart_Buf[4]);

                Serial.printf("Received Buf[105]: %#X\n", Uart_Buf[104]);

                Uart_Count++;

                Uart_Data[1] = Uart_Count >> 24;
                Uart_Data[2] = Uart_Count >> 16;
                Uart_Data[3] = Uart_Count >> 8;
                Uart_Data[4] = Uart_Count;

                delay(1000);
                Serial1.write(Uart_Data, 105);

                Uart_Count++;

                leds[0] = CRGB::Black;
                FastLED.show();
            }
        }
        else
        {
            leds[0] = CRGB::Orange;
            FastLED.show();
            delay(500);
            Serial.printf("Check Header Failed\n");
            Serial.printf("Received Header: %#X\n", Uart_Buf[0]);
            Serial.printf("Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                      (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
            Serial.printf("Received Buf[1]: %#X\n", Uart_Buf[1]);
            Serial.printf("Received Buf[2]: %#X\n", Uart_Buf[2]);
            Serial.printf("Received Buf[3]: %#X\n", Uart_Buf[3]);
            Serial.printf("Received Buf[4]: %#X\n", Uart_Buf[4]);
            leds[0] = CRGB::Black;
            FastLED.show();
        }

        Twai_Send_Message();
        Serial.println("CAN send done");
    }

    // 通信报警检测
    uint32_t alerts_triggered;
    twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS));
    // 总线状态信息
    twai_status_info_t twai_status_info;
    twai_get_status_info(&twai_status_info);

    switch (alerts_triggered)
    {
    case TWAI_ALERT_ERR_PASS:
        Serial.println("\nAlert: TWAI controller has become error passive.");
        delay(1000);
        break;
    case TWAI_ALERT_BUS_ERROR:
        Serial.println("\nAlert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus.");
        Serial.printf("Bus error count: %d\n", twai_status_info.bus_error_count);
        delay(1000);
        break;
    case TWAI_ALERT_TX_FAILED:
        Serial.println("\nAlert: The Transmission failed.");
        Serial.printf("TX buffered: %d\t", twai_status_info.msgs_to_tx);
        Serial.printf("TX error: %d\t", twai_status_info.tx_error_counter);
        Serial.printf("TX failed: %d\n", twai_status_info.tx_failed_count);
        delay(1000);
        break;
    case TWAI_ALERT_TX_SUCCESS:
        Serial.println("\nAlert: The Transmission was successful.");
        Serial.printf("TX buffered: %d\t", twai_status_info.msgs_to_tx);
        break;
    case TWAI_ALERT_RX_QUEUE_FULL:
        Serial.println("\nAlert: The RX queue is full causing a received frame to be lost.");
        Serial.printf("RX buffered: %d\t", twai_status_info.msgs_to_rx);
        Serial.printf("RX missed: %d\t", twai_status_info.rx_missed_count);
        Serial.printf("RX overrun %d\n", twai_status_info.rx_overrun_count);
        delay(1000);
        break;

    default:
        break;
    }

    switch (twai_status_info.state)
    {
    case TWAI_STATE_RUNNING:
        Serial.println("\nTWAI_STATE_RUNNING");
        break;
    case TWAI_STATE_BUS_OFF:
        Serial.println("\nTWAI_STATE_BUS_OFF");
        twai_initiate_recovery();
        delay(1000);
        break;
    case TWAI_STATE_STOPPED:
        Serial.println("\nTWAI_STATE_STOPPED");
        twai_start();
        delay(1000);
        break;
    case TWAI_STATE_RECOVERING:
        Serial.println("\nTWAI_STATE_RECOVERING");
        delay(1000);
        break;

    default:
        break;
    }

    // 如果TWAI有信息接收到
    if (alerts_triggered & TWAI_ALERT_RX_DATA)
    {
        twai_message_t rx_buf;
        while (twai_receive(&rx_buf, 0) == ESP_OK)
        {
            Twai_Receive_Message(rx_buf);
        }
    }
}
