/*
 * @Description: t_can485_config
 * @Author: LILYGO
 * @Date: 2026-06-23
 * @License: GPL 3.0
 */
#pragma once

#include "driver/gpio.h"

////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////

// RS485
#define RS485_TX GPIO_NUM_22
#define RS485_RX GPIO_NUM_21
#define RS485_CALLBACK GPIO_NUM_17
#define RS485_EN GPIO_NUM_19

// WS2812B
#define WS2812B_DATA GPIO_NUM_4

// CAN
#define CAN_TX GPIO_NUM_27
#define CAN_RX GPIO_NUM_26
#define CAN_SPEED_MODE GPIO_NUM_23

// RS485 and CAN boost power supply
#define ME2107_EN GPIO_NUM_16

// SD
#define SD_MISO GPIO_NUM_2
#define SD_MOSI GPIO_NUM_15
#define SD_SCLK GPIO_NUM_14
#define SD_CS GPIO_NUM_13

////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////
