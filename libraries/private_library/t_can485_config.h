/*
 * @Description: t_can485_config
 * @Author: LILYGO
 * @Date: 2026-06-23
 * @License: GPL 3.0
 */

#pragma once

namespace t_can485 {
namespace gpio {
namespace rs485 {
inline constexpr int kTx = 22;
inline constexpr int kRx = 21;
inline constexpr int kCallback = 17;
inline constexpr int kEnable = 19;
}  // namespace rs485

namespace ws2812 {
inline constexpr int kData = 4;
}  // namespace ws2812

namespace can {
inline constexpr int kTx = 27;
inline constexpr int kRx = 26;
inline constexpr int kSpeedMode = 23;
}  // namespace can

namespace me2107 {
inline constexpr int kEnable = 16;
}  // namespace me2107

namespace sd {
inline constexpr int kMiso = 2;
inline constexpr int kMosi = 15;
inline constexpr int kSclk = 14;
inline constexpr int kCs = 13;
}  // namespace sd

namespace button {
inline constexpr int kEsp32Boot = 0;
}  // namespace button
}  // namespace gpio

namespace device {
namespace ws2812 {
inline constexpr int kLedCount = 1;
}  // namespace ws2812
}  // namespace device
}  // namespace t_can485
