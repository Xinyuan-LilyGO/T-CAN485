/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-06-23 21:08:06
 * @LastEditTime: 2026-06-23 21:32:44
 * @License: GPL 3.0
 */
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "nvs_flash.h"
#include "sd_protocol_defs.h"
#include "sdmmc_cmd.h"
#include "t_can485_config.h"

namespace {

constexpr char kWifiStaSsid[] = "LilyGo-AABB";
constexpr char kWifiStaPassword[] = "xinyuandianzi";
constexpr char kWifiApSsid[] = "T-CAN485";
constexpr char kWifiApPassword[] = "12345678";
constexpr int kWifiApChannel = 6;
constexpr int kWifiApMaxConnection = 4;
constexpr int kWifiInfoPeriodMs = 1000;
constexpr int kTimeUpdatePeriodMs = 20000;

constexpr char kMountPoint[] = "/sdcard";
constexpr int kSdPollPeriodMs = 3000;
constexpr int kSdSpiMaxFreqKhz = 10000;

constexpr uart_port_t kRs485UartPort = UART_NUM_1;
constexpr int kRs485BaudRate = 115200;
constexpr int kRs485RxBufferSize = 4096;
constexpr int kRs485TxBufferSize = 4096;
constexpr int kRs485PayloadSize = 240;
constexpr uint8_t kRs485Header0 = 0xAA;
constexpr uint8_t kRs485Header1 = 0x55;
constexpr size_t kRs485HeaderSize = 2;
constexpr size_t kRs485SequenceSize = 4;
constexpr size_t kRs485LengthSize = 2;
constexpr size_t kRs485CrcSize = 2;
constexpr size_t kRs485PacketOverhead =
    kRs485HeaderSize + kRs485SequenceSize + kRs485LengthSize + kRs485CrcSize;
constexpr size_t kRs485PacketSize = kRs485PacketOverhead + kRs485PayloadSize;
constexpr int kRs485TxDoneWaitMs = 20;
constexpr char kRs485TestChar = 'R';

constexpr int kCanDataLength = 8;
constexpr int kCanTxWaitMs = 0;
constexpr int kCanTxIntervalMs = 20;
constexpr int kCanBusOffDelayMs = 500;
constexpr int kCanRecoverRetryIntervalMs = 1000;
constexpr int kCanPollMs = 100;
constexpr int kCanMaxReceiveFramesPerLoop = 32;
constexpr uint32_t kCanTestId = 0x0F1;
constexpr char kCanTestChar = 'C';
constexpr int kCanTxQueueDepth = 16;
constexpr int kCanRxQueueDepth = 32;

constexpr std::array<int, 2> kLoopGpios = {5, 33};
constexpr int kGpioLoopPeriodMs = 1000;
constexpr int kWs2812LedCount = t_can485::device::ws2812::kLedCount;
constexpr int kWs2812ResolutionHz = 10 * 1000 * 1000;
constexpr uint8_t kWs2812Brightness = 32;
constexpr int kWs2812PeriodMs = 1000;
constexpr int kPrintIntervalMs = 3000;
constexpr int kTaskStackSize = 6 * 1024;
constexpr UBaseType_t kTaskPriority = 5;

enum class TestMode {
  kOff,
  kSend,
  kReceive,
};

struct Rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

volatile TestMode g_rs485_mode = TestMode::kOff;
volatile TestMode g_can_mode = TestMode::kOff;
volatile bool g_wifi_sta_connected = false;

bool g_sd_mounted = false;
uint64_t g_sd_size_mb = 0;
uint32_t g_sd_sector_size = 0;
uint64_t g_sd_sector_count = 0;
char g_sd_type[24] = "Not mounted";
char g_sd_name[16] = "-";
char g_sd_error[96] = "No card";
char g_sta_ip[16] = "0.0.0.0";
char g_sta_ssid[33] = "-";
int8_t g_sta_rssi = 0;
char g_ap_ip[16] = "192.168.4.1";
char g_ap_ssid[33] = "T-CAN485";
char g_time_text[32] = "syncing";
uint32_t g_time_age_seconds = 0;
char g_status_json[2048] = {};

size_t g_rs485_total_size = 0;
size_t g_rs485_bytes_this_time = 0;
bool g_rs485_data_ok = true;
uint32_t g_rs485_crc_error_count = 0;
uint32_t g_rs485_sequence_error_count = 0;
uint32_t g_rs485_tx_sequence = 0;
uint32_t g_rs485_expected_sequence = 0;
bool g_rs485_has_expected_sequence = false;
std::vector<uint8_t> g_rs485_rx_stream;

size_t g_can_total_size = 0;
size_t g_can_bytes_this_time = 0;
bool g_can_data_ok = true;
uint32_t g_can_bus_error_count = 0;
char g_can_state_text[24] = "off";
bool g_can_recovering = false;
int64_t g_can_last_recover_us = 0;
bool g_sd_spi_bus_ready = false;

const char* ModeName(TestMode mode)
{
  switch (mode) {
    case TestMode::kSend:
      return "send";
    case TestMode::kReceive:
      return "receive";
    default:
      return "stop";
  }
}

const char* CanStateName(twai_state_t state)
{
  switch (state) {
    case TWAI_STATE_STOPPED:
      return "stopped";
    case TWAI_STATE_RUNNING:
      return "running";
    case TWAI_STATE_BUS_OFF:
      return "bus_off";
    case TWAI_STATE_RECOVERING:
      return "recovering";
    default:
      return "unknown";
  }
}

uint8_t ScaleColor(uint8_t value)
{
  return static_cast<uint8_t>(
      (static_cast<uint16_t>(value) * kWs2812Brightness) / UINT8_MAX);
}

void InitBoardPins()
{
  const uint64_t pin_mask =
      (1ULL << t_can485::gpio::me2107::kEnable) |
      (1ULL << t_can485::gpio::rs485::kCallback) |
      (1ULL << t_can485::gpio::rs485::kEnable) |
      (1ULL << t_can485::gpio::can::kSpeedMode);
  gpio_config_t config = {};
  config.pin_bit_mask = pin_mask;
  config.mode = GPIO_MODE_OUTPUT;
  config.pull_up_en = GPIO_PULLUP_DISABLE;
  config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  config.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&config));
  ESP_ERROR_CHECK(gpio_set_level(
      static_cast<gpio_num_t>(t_can485::gpio::me2107::kEnable), 1));
  ESP_ERROR_CHECK(gpio_set_level(
      static_cast<gpio_num_t>(t_can485::gpio::rs485::kEnable), 1));
  ESP_ERROR_CHECK(gpio_set_level(
      static_cast<gpio_num_t>(t_can485::gpio::rs485::kCallback), 1));
  ESP_ERROR_CHECK(gpio_set_level(
      static_cast<gpio_num_t>(t_can485::gpio::can::kSpeedMode), 0));
}

void GpioLoopTask(void* param)
{
  (void)param;
  std::array<bool, kLoopGpios.size()> output_enabled = {};

  for (size_t i = 0; i < kLoopGpios.size(); ++i) {
    const int gpio = kLoopGpios[i];
    if (!GPIO_IS_VALID_OUTPUT_GPIO(gpio)) {
      printf("[gpio_loop] GPIO%d is not output capable, skip\n", gpio);
      continue;
    }

    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << gpio;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
      printf("[gpio_loop] GPIO%d config failed: %s\n", gpio,
             esp_err_to_name(err));
      continue;
    }
    output_enabled[i] = true;
  }

  int level = 0;
  while (true) {
    level = level == 0 ? 1 : 0;
    for (size_t i = 0; i < kLoopGpios.size(); ++i) {
      if (!output_enabled[i]) {
        continue;
      }
      ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(kLoopGpios[i]),
                                     level));
    }
    printf("[gpio_loop] level=%d\n", level);
    vTaskDelay(pdMS_TO_TICKS(kGpioLoopPeriodMs));
  }
}

esp_err_t InitWs2812(led_strip_handle_t* led_strip)
{
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = t_can485::gpio::ws2812::kData;
  strip_config.max_leds = kWs2812LedCount;
  strip_config.led_model = LED_MODEL_WS2812;
  strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
  rmt_config.resolution_hz = kWs2812ResolutionHz;
  rmt_config.mem_block_symbols = 64;
  return led_strip_new_rmt_device(&strip_config, &rmt_config, led_strip);
}

void SetWs2812Color(led_strip_handle_t led_strip, const Rgb& color)
{
  ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, ScaleColor(color.red),
                                      ScaleColor(color.green),
                                      ScaleColor(color.blue)));
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void Ws2812Task(void* param)
{
  (void)param;
  led_strip_handle_t led_strip = nullptr;
  ESP_ERROR_CHECK(InitWs2812(&led_strip));
  constexpr std::array<Rgb, 4> kColors = {
      Rgb{255, 0, 0},
      Rgb{0, 255, 0},
      Rgb{0, 0, 255},
      Rgb{255, 255, 255},
  };

  while (true) {
    for (const auto& color : kColors) {
      SetWs2812Color(led_strip, color);
      vTaskDelay(pdMS_TO_TICKS(kWs2812PeriodMs));
    }
  }
}

bool InitSdSpiBus()
{
  if (g_sd_spi_bus_ready) {
    return true;
  }

  spi_bus_config_t bus_config = {};
  bus_config.mosi_io_num = t_can485::gpio::sd::kMosi;
  bus_config.miso_io_num = t_can485::gpio::sd::kMiso;
  bus_config.sclk_io_num = t_can485::gpio::sd::kSclk;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = 4000;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  esp_err_t err = spi_bus_initialize(static_cast<spi_host_device_t>(host.slot),
                                     &bus_config, SDSPI_DEFAULT_DMA);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    return false;
  }
  g_sd_spi_bus_ready = true;
  return true;
}

const char* CardTypeName(const sdmmc_card_t* card)
{
  if (card == nullptr) {
    return "Unknown";
  }
  if (card->is_sdio) {
    return "SDIO";
  }
  if (card->is_mmc) {
    return "MMC";
  }
  return (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC";
}

bool ProbeSdCard()
{
  if (!InitSdSpiBus()) {
    g_sd_mounted = false;
    std::snprintf(g_sd_error, sizeof(g_sd_error), "%s", "SPI bus init failed");
    return false;
  }

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = kSdSpiMaxFreqKhz;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.host_id = static_cast<spi_host_device_t>(host.slot);
  slot_config.gpio_cs = static_cast<gpio_num_t>(t_can485::gpio::sd::kCs);

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 4;
  mount_config.allocation_unit_size = 16 * 1024;
  mount_config.disk_status_check_enable = true;

  sdmmc_card_t* card = nullptr;
  esp_err_t err = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slot_config,
                                          &mount_config, &card);
  if (err != ESP_OK) {
    g_sd_mounted = false;
    g_sd_size_mb = 0;
    g_sd_sector_size = 0;
    g_sd_sector_count = 0;
    std::snprintf(g_sd_type, sizeof(g_sd_type), "%s", "Not mounted");
    std::snprintf(g_sd_name, sizeof(g_sd_name), "%s", "-");
    std::snprintf(g_sd_error, sizeof(g_sd_error), "%s", esp_err_to_name(err));
    return false;
  }

  g_sd_mounted = true;
  g_sd_size_mb = static_cast<uint64_t>(card->csd.capacity) *
                 card->csd.sector_size / (1024ULL * 1024ULL);
  g_sd_sector_size = card->csd.sector_size;
  g_sd_sector_count = card->csd.capacity;
  std::snprintf(g_sd_type, sizeof(g_sd_type), "%s", CardTypeName(card));
  std::snprintf(g_sd_name, sizeof(g_sd_name), "%.5s",
                reinterpret_cast<const char*>(card->cid.name));
  std::snprintf(g_sd_error, sizeof(g_sd_error), "%s", "OK");
  esp_vfs_fat_sdcard_unmount(kMountPoint, card);
  return true;
}

void SdTask(void* param)
{
  (void)param;
  bool last_mounted = false;
  while (true) {
    ProbeSdCard();
    if (g_sd_mounted != last_mounted) {
      printf("[sd] %s\n", g_sd_mounted ? "inserted" : "removed");
      last_mounted = g_sd_mounted;
    }
    vTaskDelay(pdMS_TO_TICKS(kSdPollPeriodMs));
  }
}

uint16_t Crc16Modbus(const uint8_t* data, size_t len)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x0001) != 0) {
        crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001);
      } else {
        crc = static_cast<uint16_t>(crc >> 1);
      }
    }
  }
  return crc;
}

void WriteLe16(uint8_t* data, uint16_t value)
{
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void WriteLe32(uint8_t* data, uint32_t value)
{
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint16_t ReadLe16(const uint8_t* data)
{
  return static_cast<uint16_t>(data[0]) |
         (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t ReadLe32(const uint8_t* data)
{
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

std::array<uint8_t, kRs485PacketSize> BuildRs485Packet(uint32_t sequence)
{
  std::array<uint8_t, kRs485PacketSize> packet = {};
  packet[0] = kRs485Header0;
  packet[1] = kRs485Header1;
  WriteLe32(&packet[2], sequence);
  WriteLe16(&packet[6], kRs485PayloadSize);
  for (size_t i = 0; i < kRs485PayloadSize; ++i) {
    packet[8 + i] = kRs485TestChar;
  }
  const uint16_t crc = Crc16Modbus(&packet[2],
                                   kRs485SequenceSize + kRs485LengthSize +
                                       kRs485PayloadSize);
  WriteLe16(&packet[8 + kRs485PayloadSize], crc);
  return packet;
}

size_t ProcessRs485Stream(const uint8_t* data, size_t len)
{
  g_rs485_rx_stream.insert(g_rs485_rx_stream.end(), data, data + len);
  size_t valid_payload_size = 0;

  while (g_rs485_rx_stream.size() >= kRs485PacketOverhead) {
    size_t header_pos = 0;
    while (header_pos + 1 < g_rs485_rx_stream.size() &&
           (g_rs485_rx_stream[header_pos] != kRs485Header0 ||
            g_rs485_rx_stream[header_pos + 1] != kRs485Header1)) {
      ++header_pos;
    }
    if (header_pos > 0) {
      g_rs485_rx_stream.erase(g_rs485_rx_stream.begin(),
                              g_rs485_rx_stream.begin() + header_pos);
    }
    if (g_rs485_rx_stream.size() < kRs485PacketOverhead) {
      break;
    }
    if (g_rs485_rx_stream[0] != kRs485Header0 ||
        g_rs485_rx_stream[1] != kRs485Header1) {
      break;
    }

    const uint16_t payload_len = ReadLe16(&g_rs485_rx_stream[6]);
    if (payload_len == 0 || payload_len > kRs485PayloadSize) {
      g_rs485_data_ok = false;
      g_rs485_rx_stream.erase(g_rs485_rx_stream.begin());
      continue;
    }

    const size_t packet_size = kRs485PacketOverhead + payload_len;
    if (g_rs485_rx_stream.size() < packet_size) {
      break;
    }

    const uint32_t sequence = ReadLe32(&g_rs485_rx_stream[2]);
    const uint16_t expected_crc =
        ReadLe16(&g_rs485_rx_stream[8 + payload_len]);
    const uint16_t actual_crc =
        Crc16Modbus(&g_rs485_rx_stream[2],
                    kRs485SequenceSize + kRs485LengthSize + payload_len);
    if (actual_crc != expected_crc) {
      ++g_rs485_crc_error_count;
      g_rs485_data_ok = false;
      g_rs485_rx_stream.erase(g_rs485_rx_stream.begin(),
                              g_rs485_rx_stream.begin() + packet_size);
      continue;
    }

    if (g_rs485_has_expected_sequence &&
        sequence != g_rs485_expected_sequence) {
      ++g_rs485_sequence_error_count;
      g_rs485_data_ok = false;
    }
    g_rs485_expected_sequence = sequence + 1;
    g_rs485_has_expected_sequence = true;
    valid_payload_size += payload_len;
    g_rs485_rx_stream.erase(g_rs485_rx_stream.begin(),
                            g_rs485_rx_stream.begin() + packet_size);
  }

  return valid_payload_size;
}

bool InitRs485()
{
  uart_config_t uart_config = {};
  uart_config.baud_rate = kRs485BaudRate;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  esp_err_t err = uart_driver_install(kRs485UartPort, kRs485RxBufferSize,
                                      kRs485TxBufferSize, 0, nullptr, 0);
  if (err != ESP_OK) {
    return false;
  }
  err = uart_param_config(kRs485UartPort, &uart_config);
  if (err != ESP_OK) {
    uart_driver_delete(kRs485UartPort);
    return false;
  }
  err = uart_set_pin(kRs485UartPort, t_can485::gpio::rs485::kTx,
                     t_can485::gpio::rs485::kRx, UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    uart_driver_delete(kRs485UartPort);
    return false;
  }
  return true;
}

void Rs485Task(void* param)
{
  (void)param;
  TestMode active_mode = TestMode::kOff;
  bool initialized = false;
  uint32_t last_print_ms = 0;

  while (true) {
    const TestMode requested_mode = g_rs485_mode;
    if (requested_mode != active_mode) {
      if (initialized) {
        uart_driver_delete(kRs485UartPort);
        initialized = false;
      }
      active_mode = requested_mode;
      if (active_mode != TestMode::kOff) {
        g_rs485_total_size = 0;
        g_rs485_bytes_this_time = 0;
        g_rs485_data_ok = true;
        g_rs485_crc_error_count = 0;
        g_rs485_sequence_error_count = 0;
        g_rs485_tx_sequence = 0;
        g_rs485_expected_sequence = 0;
        g_rs485_has_expected_sequence = false;
        g_rs485_rx_stream.clear();
        initialized = InitRs485();
        printf("[rs485] mode=%s init=%s\n", ModeName(active_mode),
               initialized ? "ok" : "failed");
      }
    }

    if (!initialized || active_mode == TestMode::kOff) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (active_mode == TestMode::kSend) {
      const auto packet = BuildRs485Packet(g_rs485_tx_sequence++);
      const int len = uart_write_bytes(kRs485UartPort, packet.data(),
                                       packet.size());
      if (len > 0) {
        uart_wait_tx_done(kRs485UartPort, pdMS_TO_TICKS(kRs485TxDoneWaitMs));
        g_rs485_bytes_this_time += len;
        g_rs485_total_size += len;
      }
    } else {
      size_t rx_len = 0;
      uart_get_buffered_data_len(kRs485UartPort, &rx_len);
      if (rx_len > 0) {
        std::string buffer(rx_len, '\0');
        const int read_len =
            uart_read_bytes(kRs485UartPort, buffer.data(), rx_len,
                            pdMS_TO_TICKS(20));
        if (read_len > 0) {
          const size_t valid_size = ProcessRs485Stream(
              reinterpret_cast<const uint8_t*>(buffer.data()), read_len);
          g_rs485_bytes_this_time += valid_size;
          g_rs485_total_size += valid_size;
        }
      }
    }

    const uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_print_ms >= kPrintIntervalMs) {
      if (active_mode == TestMode::kSend) {
        printf("[rs485 %s] total %zu B\n", ModeName(active_mode),
               g_rs485_total_size);
      } else {
        printf("[rs485 %s] total %zu B | crc error %lu | seq error %lu\n",
               ModeName(active_mode), g_rs485_total_size,
               static_cast<unsigned long>(g_rs485_crc_error_count),
               static_cast<unsigned long>(g_rs485_sequence_error_count));
      }
      g_rs485_bytes_this_time = 0;
      last_print_ms = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool InitCan()
{
  g_can_recovering = false;
  g_can_last_recover_us = 0;
  std::snprintf(g_can_state_text, sizeof(g_can_state_text), "%s", "running");

  twai_general_config_t general_config =
      TWAI_GENERAL_CONFIG_DEFAULT(
          static_cast<gpio_num_t>(t_can485::gpio::can::kTx),
          static_cast<gpio_num_t>(t_can485::gpio::can::kRx), TWAI_MODE_NORMAL);
  general_config.tx_queue_len = kCanTxQueueDepth;
  general_config.rx_queue_len = kCanRxQueueDepth;
  general_config.alerts_enabled =
      TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED;
  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_1MBITS();
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err =
      twai_driver_install(&general_config, &timing_config, &filter_config);
  if (err != ESP_OK) {
    return false;
  }
  err = twai_start();
  if (err != ESP_OK) {
    twai_driver_uninstall();
    return false;
  }
  return true;
}

void DeinitCan()
{
  twai_stop();
  twai_driver_uninstall();
  g_can_recovering = false;
  std::snprintf(g_can_state_text, sizeof(g_can_state_text), "%s", "off");
}

void ServiceCanState()
{
  twai_status_info_t status = {};
  if (twai_get_status_info(&status) != ESP_OK) {
    return;
  }
  g_can_bus_error_count = status.bus_error_count;
  std::snprintf(g_can_state_text, sizeof(g_can_state_text), "%s",
                CanStateName(status.state));

  uint32_t alerts = 0;
  if (twai_read_alerts(&alerts, 0) == ESP_OK &&
      (alerts & TWAI_ALERT_BUS_RECOVERED) != 0) {
    g_can_recovering = false;
  }

  const int64_t now = esp_timer_get_time();
  const bool can_retry_recover =
      now - g_can_last_recover_us >= kCanRecoverRetryIntervalMs * 1000;
  if (status.state == TWAI_STATE_BUS_OFF && !g_can_recovering &&
      can_retry_recover) {
    vTaskDelay(pdMS_TO_TICKS(kCanBusOffDelayMs));
    if (twai_initiate_recovery() == ESP_OK) {
      g_can_recovering = true;
      g_can_last_recover_us = now;
    }
  }
}

void CanTask(void* param)
{
  (void)param;
  TestMode active_mode = TestMode::kOff;
  bool initialized = false;
  uint32_t last_print_ms = 0;
  uint8_t tx_data[kCanDataLength] = {};
  for (int i = 0; i < kCanDataLength; ++i) {
    tx_data[i] = kCanTestChar;
  }

  while (true) {
    const TestMode requested_mode = g_can_mode;
    if (requested_mode != active_mode) {
      if (initialized) {
        DeinitCan();
        initialized = false;
      }
      active_mode = requested_mode;
      if (active_mode != TestMode::kOff) {
        g_can_total_size = 0;
        g_can_bytes_this_time = 0;
        g_can_data_ok = true;
        g_can_bus_error_count = 0;
        initialized = InitCan();
        printf("[can] mode=%s init=%s\n", ModeName(active_mode),
               initialized ? "ok" : "failed");
      }
    }

    if (!initialized || active_mode == TestMode::kOff) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    ServiceCanState();
    if (active_mode == TestMode::kSend) {
      twai_status_info_t status = {};
      if (twai_get_status_info(&status) == ESP_OK &&
          status.state != TWAI_STATE_BUS_OFF) {
        twai_message_t message = {};
        message.identifier = kCanTestId;
        message.data_length_code = kCanDataLength;
        std::memcpy(message.data, tx_data, sizeof(tx_data));
        if (twai_transmit(&message, pdMS_TO_TICKS(kCanTxWaitMs)) == ESP_OK) {
          g_can_bytes_this_time += kCanDataLength;
          g_can_total_size += kCanDataLength;
        }
      }
      vTaskDelay(pdMS_TO_TICKS(kCanTxIntervalMs));
    } else {
      twai_message_t message = {};
      int receive_count = 0;
      while (receive_count < kCanMaxReceiveFramesPerLoop &&
             twai_receive(&message, pdMS_TO_TICKS(kCanPollMs)) == ESP_OK) {
        ++receive_count;
        if ((message.flags & TWAI_MSG_FLAG_EXTD) == 0 &&
            (message.flags & TWAI_MSG_FLAG_RTR) == 0 &&
            message.identifier == kCanTestId &&
            message.data_length_code == kCanDataLength) {
          g_can_bytes_this_time += message.data_length_code;
          g_can_total_size += message.data_length_code;
        } else {
          g_can_data_ok = false;
        }
      }
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    const uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_print_ms >= kPrintIntervalMs) {
      printf("[can %s] total %zu B | bus error %lu | state %s\n",
             ModeName(active_mode), g_can_total_size,
             static_cast<unsigned long>(g_can_bus_error_count),
             g_can_state_text);
      g_can_bytes_this_time = 0;
      last_print_ms = now;
    }
  }
}

void StartSntp()
{
  setenv("TZ", "HKT-8", 1);
  tzset();
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_set_sync_interval(kTimeUpdatePeriodMs);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.google.com");
  esp_sntp_init();
}

void UpdateTimeText()
{
  std::time_t now = 0;
  std::time(&now);
  struct tm time_info = {};
  localtime_r(&now, &time_info);
  if (time_info.tm_year < (2024 - 1900)) {
    std::snprintf(g_time_text, sizeof(g_time_text), "%s", "syncing");
    return;
  }

  std::strftime(g_time_text, sizeof(g_time_text), "%Y-%m-%d %H:%M:%S",
                &time_info);
  g_time_age_seconds = 0;
}

void TimeTask(void* param)
{
  (void)param;
  uint32_t elapsed_ms = 0;
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    elapsed_ms += 1000;
    if (elapsed_ms >= kTimeUpdatePeriodMs) {
      UpdateTimeText();
      elapsed_ms = 0;
    } else if (std::strcmp(g_time_text, "syncing") != 0) {
      ++g_time_age_seconds;
    }
  }
}

void WifiInfoTask(void* param)
{
  (void)param;
  while (true) {
    wifi_ap_record_t ap_info = {};
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      g_wifi_sta_connected = true;
      std::snprintf(g_sta_ssid, sizeof(g_sta_ssid), "%.32s",
                    reinterpret_cast<const char*>(ap_info.ssid));
      g_sta_rssi = ap_info.rssi;
    } else {
      g_wifi_sta_connected = false;
      std::snprintf(g_sta_ssid, sizeof(g_sta_ssid), "%s", "-");
      g_sta_rssi = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(kWifiInfoPeriodMs));
  }
}

void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
                      void* event_data)
{
  (void)arg;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    g_wifi_sta_connected = false;
    std::snprintf(g_sta_ip, sizeof(g_sta_ip), "%s", "0.0.0.0");
    std::snprintf(g_sta_ssid, sizeof(g_sta_ssid), "%s", "-");
    g_sta_rssi = 0;
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    const ip_event_got_ip_t* event =
        static_cast<const ip_event_got_ip_t*>(event_data);
    g_wifi_sta_connected = true;
    std::snprintf(g_sta_ssid, sizeof(g_sta_ssid), "%s", kWifiStaSsid);
    std::snprintf(g_sta_ip, sizeof(g_sta_ip), IPSTR,
                  IP2STR(&event->ip_info.ip));
  }
}

void InitWifi()
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();

  esp_netif_ip_info_t ap_ip = {};
  esp_netif_get_ip_info(ap_netif, &ap_ip);
  std::snprintf(g_ap_ip, sizeof(g_ap_ip), IPSTR, IP2STR(&ap_ip.ip));

  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init_config));
  uint8_t ap_mac[6] = {};
  ESP_ERROR_CHECK(esp_read_mac(ap_mac, ESP_MAC_WIFI_SOFTAP));
  std::snprintf(g_ap_ssid, sizeof(g_ap_ssid),
                "%s_%02X%02X%02X%02X%02X%02X", kWifiApSsid, ap_mac[0],
                ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &WifiEventHandler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &WifiEventHandler, nullptr));

  wifi_config_t sta_config = {};
  std::snprintf(reinterpret_cast<char*>(sta_config.sta.ssid),
                sizeof(sta_config.sta.ssid), "%s", kWifiStaSsid);
  std::snprintf(reinterpret_cast<char*>(sta_config.sta.password),
                sizeof(sta_config.sta.password), "%s", kWifiStaPassword);
  sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  wifi_config_t ap_config = {};
  size_t ap_ssid_len = std::strlen(g_ap_ssid);
  if (ap_ssid_len > sizeof(ap_config.ap.ssid)) {
    ap_ssid_len = sizeof(ap_config.ap.ssid);
  }
  std::memcpy(ap_config.ap.ssid, g_ap_ssid, ap_ssid_len);
  ap_config.ap.ssid_len = ap_ssid_len;
  std::snprintf(reinterpret_cast<char*>(ap_config.ap.password),
                sizeof(ap_config.ap.password), "%s", kWifiApPassword);
  ap_config.ap.channel = kWifiApChannel;
  ap_config.ap.max_connection = kWifiApMaxConnection;
  ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  StartSntp();
}

const char* StatusJson()
{
  const TestMode rs485_mode = g_rs485_mode;
  const TestMode can_mode = g_can_mode;

  std::snprintf(
      g_status_json, sizeof(g_status_json),
      "{"
      "\"time\":\"%s\",\"time_age_seconds\":%lu,"
      "\"wifi\":{\"sta\":%s,\"sta_ssid\":\"%s\",\"sta_ip\":\"%s\","
      "\"sta_rssi\":%d,\"ap_ssid\":\"%s\",\"ap_ip\":\"%s\"},"
      "\"sd\":{\"mounted\":%s,\"name\":\"%s\",\"type\":\"%s\","
      "\"size_mb\":%llu,\"sector_size\":%lu,\"sector_count\":%llu,"
      "\"error\":\"%s\"},"
      "\"rs485\":{\"mode\":\"%s\",\"total\":%u,\"ok\":%s,"
      "\"crc_errors\":%lu,\"sequence_errors\":%lu},"
      "\"can\":{\"mode\":\"%s\",\"total\":%u,\"ok\":%s,\"state\":\"%s\","
      "\"bus_errors\":%lu}"
      "}",
      g_time_text, static_cast<unsigned long>(g_time_age_seconds),
      g_wifi_sta_connected ? "true" : "false", g_sta_ssid, g_sta_ip,
      static_cast<int>(g_sta_rssi), g_ap_ssid, g_ap_ip,
      g_sd_mounted ? "true" : "false", g_sd_name, g_sd_type,
      static_cast<unsigned long long>(g_sd_size_mb),
      static_cast<unsigned long>(g_sd_sector_size),
      static_cast<unsigned long long>(g_sd_sector_count), g_sd_error,
      ModeName(rs485_mode),
      static_cast<unsigned>(g_rs485_total_size),
      g_rs485_data_ok ? "true" : "false",
      static_cast<unsigned long>(g_rs485_crc_error_count),
      static_cast<unsigned long>(g_rs485_sequence_error_count),
      ModeName(can_mode), static_cast<unsigned>(g_can_total_size),
      g_can_data_ok ? "true" : "false", g_can_state_text,
      static_cast<unsigned long>(g_can_bus_error_count));
  return g_status_json;
}

constexpr char kIndexHtml[] = R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>T-CAN485 Control Center</title>
<style>
body{margin:0;font-family:Arial,sans-serif;background:#101418;color:#e8eef2}
header{padding:22px 24px;background:#17212b;border-bottom:1px solid #2a3a48}
h1{margin:0;font-size:24px}main{padding:18px;display:grid;gap:14px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:14px}
.card{background:#18222c;border:1px solid #2b3a47;border-radius:8px;padding:16px}
.label{color:#91a4b4;font-size:13px}.value{font-size:26px;margin-top:6px}
.row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
button{background:#263746;color:#e8eef2;border:1px solid #41566a;border-radius:6px;padding:10px 14px}
button.active{background:#1f8f5f;border-color:#32b878}
.ok{color:#63d68a}.bad{color:#ff7c7c}.small{font-size:14px;color:#a9bac7}
</style>
</head>
<body>
<header><h1>T-CAN485 Control Center</h1><div class="small">AP and STA are enabled</div></header>
<main>
<section class="grid">
<div class="card"><div class="label">AP</div><div class="value" id="ap">-</div><div class="small" id="apip"></div></div>
<div class="card"><div class="label">STA</div><div class="value" id="sta">-</div><div class="small" id="staip"></div></div>
<div class="card"><div class="label">Time</div><div class="value" id="time">-</div><div class="small" id="timeage"></div></div>
</section>
<section class="grid">
<div class="card"><div class="label">SD Card</div><div class="value" id="sd">-</div><div class="small" id="sdinfo"></div></div>
</section>
<section class="grid">
<div class="card"><div class="label">RS485</div><div class="value" id="rs485">-</div><div class="small" id="rs485t"></div><div class="row"><button onclick="setMode('rs485','send')">Send</button><button onclick="setMode('rs485','receive')">Receive</button><button onclick="setMode('rs485','stop')">Stop</button></div></div>
<div class="card"><div class="label">CAN</div><div class="value" id="can">-</div><div class="small" id="cant"></div><div class="row"><button onclick="setMode('can','send')">Send</button><button onclick="setMode('can','receive')">Receive</button><button onclick="setMode('can','stop')">Stop</button></div></div>
</section>
</main>
<script>
async function setMode(target,mode){await fetch(`/api/set?target=${target}&mode=${mode}`);refresh();}
async function refresh(){
 const s=await (await fetch('/api/status')).json();
 time.textContent=s.time; timeage.textContent=`updated ${s.time_age_seconds} s ago`; sta.textContent=s.wifi.sta?s.wifi.sta_ssid:'Connecting'; sta.className='value '+(s.wifi.sta?'ok':'bad');
 staip.textContent=s.wifi.sta?`${s.wifi.sta_ip} | RSSI ${s.wifi.sta_rssi} dBm`:s.wifi.sta_ip; ap.textContent=s.wifi.ap_ssid; apip.textContent=s.wifi.ap_ip;
 sd.textContent=s.sd.mounted?'Detected':'Not detected'; sd.className='value '+(s.sd.mounted?'ok':'bad'); sdinfo.textContent=`name ${s.sd.name} | ${s.sd.type} | ${s.sd.size_mb} MB | sector ${s.sd.sector_size} B | count ${s.sd.sector_count} | ${s.sd.error}`;
 const rs485Mode=s.rs485.mode.charAt(0).toUpperCase()+s.rs485.mode.slice(1);
 const canMode=s.can.mode.charAt(0).toUpperCase()+s.can.mode.slice(1);
 rs485.textContent=rs485Mode; rs485.className='value '+(s.rs485.ok?'ok':'bad'); rs485t.textContent=s.rs485.mode==='send'?`total ${s.rs485.total} B`:`total ${s.rs485.total} B | crc error ${s.rs485.crc_errors} | seq error ${s.rs485.sequence_errors}`;
 can.textContent=canMode; can.className='value '+(s.can.ok?'ok':'bad'); cant.textContent=`total ${s.can.total} B | bus error ${s.can.bus_errors}`;
}
setInterval(refresh,1000);refresh();
</script>
</body>
</html>
)HTML";

esp_err_t RootHandler(httpd_req_t* request)
{
  httpd_resp_set_type(request, "text/html");
  return httpd_resp_send(request, kIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t StatusHandler(httpd_req_t* request)
{
  const char* json = StatusJson();
  httpd_resp_set_type(request, "application/json");
  return httpd_resp_send(request, json, HTTPD_RESP_USE_STRLEN);
}

TestMode ParseMode(const char* mode)
{
  if (std::strcmp(mode, "send") == 0) {
    return TestMode::kSend;
  }
  if (std::strcmp(mode, "receive") == 0) {
    return TestMode::kReceive;
  }
  if (std::strcmp(mode, "stop") == 0) {
    return TestMode::kOff;
  }
  return TestMode::kOff;
}

esp_err_t SetHandler(httpd_req_t* request)
{
  char query[96] = {};
  char target[16] = {};
  char mode[16] = {};
  if (httpd_req_get_url_query_str(request, query, sizeof(query)) == ESP_OK) {
    httpd_query_key_value(query, "target", target, sizeof(target));
    httpd_query_key_value(query, "mode", mode, sizeof(mode));
  }

  if (std::strcmp(target, "rs485") == 0) {
    g_rs485_mode = ParseMode(mode);
  } else if (std::strcmp(target, "can") == 0) {
    g_can_mode = ParseMode(mode);
  }

  httpd_resp_set_type(request, "application/json");
  return httpd_resp_sendstr(request, "{\"ok\":true}");
}

void StartHttpServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 8192;

  httpd_handle_t server = nullptr;
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  httpd_uri_t root_uri = {};
  root_uri.uri = "/";
  root_uri.method = HTTP_GET;
  root_uri.handler = RootHandler;
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_uri));

  httpd_uri_t status_uri = {};
  status_uri.uri = "/api/status";
  status_uri.method = HTTP_GET;
  status_uri.handler = StatusHandler;
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &status_uri));

  httpd_uri_t set_uri = {};
  set_uri.uri = "/api/set";
  set_uri.method = HTTP_GET;
  set_uri.handler = SetHandler;
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &set_uri));
}

}  // namespace

extern "C" void app_main(void)
{
  printf("T-CAN485 general_test\n");
  InitBoardPins();
  InitWifi();
  UpdateTimeText();
  StartHttpServer();

  xTaskCreate(TimeTask, "time_task", kTaskStackSize, nullptr, kTaskPriority,
              nullptr);
  xTaskCreate(WifiInfoTask, "wifi_info_task", kTaskStackSize, nullptr,
              kTaskPriority, nullptr);
  xTaskCreate(Ws2812Task, "ws2812_task", kTaskStackSize, nullptr,
              kTaskPriority, nullptr);
  xTaskCreate(GpioLoopTask, "gpio_loop_task", kTaskStackSize, nullptr,
              kTaskPriority, nullptr);
  xTaskCreate(SdTask, "sd_task", kTaskStackSize, nullptr, kTaskPriority,
              nullptr);
  xTaskCreate(Rs485Task, "rs485_task", kTaskStackSize, nullptr, kTaskPriority,
              nullptr);
  xTaskCreate(CanTask, "can_task", kTaskStackSize, nullptr, kTaskPriority,
              nullptr);

  printf("HTTP control center: connect AP %s password %s, open http://%s\n",
         g_ap_ssid, kWifiApPassword, g_ap_ip);
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
