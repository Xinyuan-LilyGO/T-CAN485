// Freenove_WS2812_Lib_for_ESP32.h
/**
 * Brief	A library for controlling ws2812 in esp32 platform.
 * Author	SuhaylZhao
 * Company	Freenove
 * Date		2020-07-31
 */

#ifndef _FREENOVE_WS2812_LIB_FOR_ESP32_h
#define _FREENOVE_WS2812_LIB_FOR_ESP32_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif

#include "driver/rmt.h"
#include "led_strip.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

enum LED_TYPE
{					  //R  G  B
	TYPE_RGB = 0x06,  //00 01 10
	TYPE_RBG = 0x09,  //00 10 01
	TYPE_GRB = 0x12,  //01 00 10
	TYPE_GBR = 0x21,  //10 00 01
	TYPE_BRG = 0x18,  //01 10 00
	TYPE_BGR = 0x24	  //10 01 00
};

#define RMT_DEFAULT_CONFIG_TX(gpio, channel_id)      \
    {                                                \
        .rmt_mode = RMT_MODE_TX,                     \
        .channel = (rmt_channel_t)channel_id,        \
        .clk_div = 2,                               \
        .gpio_num = (gpio_num_t)gpio,                \
        .mem_block_num = 1,                          \
		{				\
			.tx_config = { \
				.loop_en = false,                        \
				.carrier_freq_hz = 38000,                \
				.carrier_duty_percent = 33,              \
				.carrier_level = RMT_CARRIER_LEVEL_HIGH, \
				.carrier_en = false,                     \
				.idle_level = RMT_IDLE_LEVEL_LOW,        \
				.idle_output_en = true,                  \
			}\
		},	\
    }

class Freenove_ESP32_WS2812
{
protected:
	
	u16 ledCounts;
	u8 rmt_chn;
	u8 pin;
	u8 br;
	
	u8 rOffset;
	u8 gOffset;
	u8 bOffset;

	rmt_config_t config;
	led_strip_config_t strip_config;
	led_strip_t *strip;

public:
	Freenove_ESP32_WS2812(u16 n = 8, u8 pin_gpio = 2, u8 chn = 0, LED_TYPE t = TYPE_GRB);

	bool begin();
	void setLedCount(u16 n);
	void setLedType(LED_TYPE t);
	void setBrightness(u8 brightness);

	esp_err_t setLedColorData(u16 index, u32 rgb);
	esp_err_t setLedColorData(u16 index, u8 r, u8 g, u8 b);

	esp_err_t setLedColor(u16 index, u32 rgb);
	esp_err_t setLedColor(u16 index, u8 r, u8 g, u8 b);

	esp_err_t setAllLedsColorData(u32 rgb);
	esp_err_t setAllLedsColorData(u8 r, u8 g, u8 b);

	esp_err_t setAllLedsColor(u32 rgb);
	esp_err_t setAllLedsColor(u8 r, u8 g, u8 b);

	esp_err_t show();

	uint32_t Wheel(byte pos);
	uint32_t hsv2rgb(uint32_t h, uint32_t s, uint32_t v);
};

#endif

