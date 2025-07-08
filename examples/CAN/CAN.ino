/*
 * @Description: 
        CAN Test
        Two devices burning this program simultaneously can 
    observe each other sending information on the serial port: '1,2,3,4,5,6,7,8'.
    (ESP32 Arduino IDE Library Version: arduino_esp32_v3.0.1)
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2024-01-18 18:55:08
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-24 10:09:19
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include "driver/twai.h"
#include "pin_config.h"

// Intervall:
#define TRANSMIT_RATE_MS 1000
#define POLLING_RATE_MS 1000

size_t CycleTime = 0;

uint64_t CAN_Count = 0;

static const uint8_t TWAI_Data[] = {
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
};

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

    pinMode(ME2107_EN, OUTPUT);
    digitalWrite(ME2107_EN, HIGH);

    // High speed mode
    pinMode(CAN_SPEED_MODE, OUTPUT);
    digitalWrite(CAN_SPEED_MODE, LOW);

    CAN_Drive_Initialization();
}

void loop()
{
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

    if (millis() > CycleTime)
    {
        Twai_Send_Message();
        CycleTime = millis() + 1000; // 100ms
    }
}