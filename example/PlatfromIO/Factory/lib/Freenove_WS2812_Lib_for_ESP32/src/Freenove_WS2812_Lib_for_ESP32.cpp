// 
// 
// 
/**
 * Brief	A library for controlling ws2812 in esp32 platform.
 * Author	SuhaylZhao
 * Company	Freenove
 * Date		2020-07-31
 */


#include "Freenove_WS2812_Lib_for_ESP32.h"


Freenove_ESP32_WS2812::Freenove_ESP32_WS2812(u16 n /*= 8*/, u8 pin_gpio /*= 2*/, u8 chn /*= 0*/, LED_TYPE t /*= TYPE_GRB*/)
{
	ledCounts = n;
	pin = pin_gpio;
	rmt_chn = chn;
	br = 255;
	setLedType(t);
}

bool Freenove_ESP32_WS2812::begin()
{	
	config = RMT_DEFAULT_CONFIG_TX(pin, rmt_chn);
	rmt_config(&config);
	rmt_driver_install(config.channel, 0, 0);

	strip_config = LED_STRIP_DEFAULT_CONFIG(ledCounts, (led_strip_dev_t)config.channel);
	
	strip = led_strip_new_rmt_ws2812(&strip_config);
	if (!strip) {
		return false;
	}
	return true;
}

void Freenove_ESP32_WS2812::setLedCount(u16 n)
{
	ledCounts = n;
	begin();
}

void Freenove_ESP32_WS2812::setLedType(LED_TYPE t)
{
	rOffset = (t >> 4) & 0x03;
	gOffset = (t >> 2) & 0x03;
	bOffset = t & 0x03;
}

void Freenove_ESP32_WS2812::setBrightness(u8 brightness)
{
	br = constrain(brightness, 0, 255);
}

esp_err_t Freenove_ESP32_WS2812::setLedColorData(u16 index, u32 rgb)
{
	return setLedColorData(index, rgb >> 16, rgb >> 8, rgb);
}

esp_err_t Freenove_ESP32_WS2812::setLedColorData(u16 index, u8 r, u8 g, u8 b)
{
	u8 p[3];
	p[rOffset] = r * br / 255;
	p[gOffset] = g * br / 255;
	p[bOffset] = b * br / 255;
	return strip->set_pixel(strip, index, p[0], p[1], p[2]);
}

esp_err_t Freenove_ESP32_WS2812::setLedColor(u16 index, u32 rgb)
{
	return setLedColor(index, rgb >> 16, rgb >> 8, rgb);
}

esp_err_t Freenove_ESP32_WS2812::setLedColor(u16 index, u8 r, u8 g, u8 b)
{
	setLedColorData(index, r, g, b);
	return show();
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColorData(u32 rgb)
{
	for (int i = 0; i < ledCounts; i++)
	{
		setLedColorData(i, rgb);
	}
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColorData(u8 r, u8 g, u8 b)
{
	for (int i = 0; i < ledCounts; i++)
	{
		setLedColorData(i, r, g, b);
	}
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColor(u32 rgb)
{
	setAllLedsColorData(rgb);
	show();
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColor(u8 r, u8 g, u8 b)
{
	setAllLedsColorData(r, g, b);
	show();
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::show()
{
	return strip->refresh(strip, 100);
}

uint32_t Freenove_ESP32_WS2812::Wheel(byte pos)
{
	u32 WheelPos = pos % 0xff;
	if (WheelPos < 85) {
		return ((255 - WheelPos * 3) << 16) | ((WheelPos * 3) << 8);
	}
	if (WheelPos < 170) {
		WheelPos -= 85;
		return (((255 - WheelPos * 3) << 8) | (WheelPos * 3));
	}
	WheelPos -= 170;
	return ((WheelPos * 3) << 16 | (255 - WheelPos * 3));
}

uint32_t Freenove_ESP32_WS2812::hsv2rgb(uint32_t h, uint32_t s, uint32_t v)
{
	u8 r, g, b;
	h %= 360; // h -> [0,360]
	uint32_t rgb_max = v * 2.55f;
	uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

	uint32_t i = h / 60;
	uint32_t diff = h % 60;

	// RGB adjustment amount by hue
	uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

	switch (i) {
	case 0:
		r = rgb_max;
		g = rgb_min + rgb_adj;
		b = rgb_min;
		break;
	case 1:
		r = rgb_max - rgb_adj;
		g = rgb_max;
		b = rgb_min;
		break;
	case 2:
		r = rgb_min;
		g = rgb_max;
		b = rgb_min + rgb_adj;
		break;
	case 3:
		r = rgb_min;
		g = rgb_max - rgb_adj;
		b = rgb_max;
		break;
	case 4:
		r = rgb_min + rgb_adj;
		g = rgb_min;
		b = rgb_max;
		break;
	default:
		r = rgb_max;
		g = rgb_min;
		b = rgb_max - rgb_adj;
		break;
	}
	return (uint32_t)(r << 16 | g << 8 | b);
}
