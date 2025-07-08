<!--
 * @Description: None
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-09-11 16:13:14
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-24 11:22:40
 * @License: GPL 3.0
-->
<h1 align = "center">T-CAN485</h1>

## **[English](./README.md) | 中文**

## 版本迭代:
| Version                               | Update date                       |
| :-------------------------------: | :-------------------------------: |
| T-CAN485                      | -                         |

## 购买链接

| Product                     | SOC           |  FLASH  |  PSRAM   | Link                   |
| :------------------------: | :-----------: |:-------: | :---------: | :------------------: |
| T-CAN485   | ESP32 |   4M   | - |  [AliExpress](https://pt.aliexpress.com/item/1005003624034092.html)  |

## 目录
- [**English | 中文**](#english--中文)
- [版本迭代:](#版本迭代)
- [购买链接](#购买链接)
- [目录](#目录)
- [描述](#描述)
- [预览](#预览)
  - [PCB板](#pcb板)
  - [细节](#细节)
- [模块](#模块)
  - [1. MCU](#1-mcu)
  - [2. RS485](#2-rs485)
  - [3. CAN](#3-can)
  - [4. 升压芯片](#4-升压芯片)
- [快速开始](#快速开始)
  - [示例支持](#示例支持)
  - [Arduino](#arduino)
    - [ESP32](#esp32)
  - [firmware烧录](#firmware烧录)
- [引脚总览](#引脚总览)
- [相关测试](#相关测试)
- [常见问题](#常见问题)
- [项目](#项目)
- [资料](#资料)
- [依赖库](#依赖库)

## 描述

T-CAN485是一款基于ESP32芯片开发的核心板，搭载1个CAN总线和1个RS485设备，支持高电压输入（5V-12V），板载一颗可编程的WS2812的RGBLED灯珠，支持TF卡

## 预览

### PCB板

<p align="center" width="100%">
    <img src="image/T-CAN485-PINMAP-CN.jpg" alt="example">
</p>

### 细节

<p align="center" width="100%">
    <img src="image/T-CAN485-DETAIL-CN.jpg" alt="example">
</p>

## 模块

### 1. MCU

* 芯片：ESP32
* FLASH：4M
* 其他说明：更多资料请访问[乐鑫官方ESP32数据手册](https://www.espressif.com.cn/sites/default/files/documentation/esp32_datasheet_en.pdf)


### 2. RS485

* 芯片：MAX13487EESA+
* 总线通信协议：UART

### 3. CAN

* 芯片：SN65HVD231
* 总线通信协议：TWAI

### 4. 升压芯片

* 芯片：ME2107A50M5G

## 快速开始

### 示例支持

| Example | Support IDE And Version| Description | Picture |
| ------  | ------  | ------ | ------ | 
| [CAN](./examples/CAN) | `[Arduino IDE][arduino_esp32_v3.0.1]` |  |  |
| [Original Test](./examples/Original_Test) |`[Arduino IDE][arduino_esp32_v3.0.1]` | 出厂初始测试文件 |  |
| [RS485_WS2812B](./examples/RS485_WS2812B) |`[Arduino IDE][arduino_esp32_v3.0.1]` |  |  |
| [SD](./examples/SD) |`[Arduino IDE][arduino_esp32_v3.0.1]` |  |  |
| [WIFI_HTTP_Download_File](./examples/WIFI_HTTP_Download_File) |`[Arduino IDE][arduino_esp32_v3.0.1]` |  |  |
| [WS2812B_Blink](./examples/WS2812B_Blink) |`[Arduino IDE][arduino_esp32_v3.0.1]` |  |  |

| Firmware | Description | Picture |
| ------  | ------  | ------ |
| [Original Test V1.0.0](./firmware/[T-CAN485_V1.0][Original_Test]_firmware_V1.0.0) | 初始版本 |  |

### Arduino
1. 安装[Arduino](https://www.arduino.cc/en/software)，根据你的系统类型选择安装。

2. 打开项目文件夹的“example”目录，选择示例项目文件夹，打开以“.ino”结尾的文件即可打开Arduino IDE项目工作区。

3. 打开右上角“工具”菜单栏->选择“开发板”->“开发板管理器”，找到或者搜索“esp32”，下载作者名为“Espressif Systems”的开发板文件。接着返回“开发板”菜单栏，选择“ESP32 Arduino”开发板下的开发板类型，选择的开发板类型由“platformio.ini”文件中以[env]目录下的“board = xxx”标头为准，如果没有对应的开发板，则需要自己手动添加项目文件夹下“board”目录下的开发板。

4. 打开菜单栏“[文件](image/6.png)”->“[首选项](image/6.png)”，找到“[项目文件夹位置](image/7.png)”这一栏，将项目目录下的“lib”文件夹里的所有库文件连带文件夹复制粘贴到这个目录下的“lib”里边。

5. 在 "工具 "菜单中选择正确的设置，如下表所示。

#### ESP32
| Setting                               | Value                                 |
| :-------------------------------: | :-------------------------------: |
| Board                                 | ESP32 Dev Module           |
| Upload Speed                     | 921600                               |
| CPU Frequency                   | 240MHz (WiFi)                    |
| Flash Mode                         | QIO                        |
| Flash Size                           | 4MB (32Mb)                    |
| Core Debug Level                | None                                 |
| PSRAM                                | Disable                         |
| Arduino Runs On                  | Core 1                               |

6. 选择正确的端口。

7. 点击右上角“<kbd>[√](image/8.png)</kbd>”进行编译，如果编译无误，将单片机连接电脑，点击右上角“<kbd>[→](image/9.png)</kbd>”即可进行烧录。

### firmware烧录
1. 打开项目文件“tools”找到ESP32烧录工具，打开。

2. 选择正确的烧录芯片以及烧录方式点击“OK”，如图所示根据步骤1->2->3->4->5即可烧录程序，如果烧录不成功，请按住“BOOT-0”键再下载烧录。

3. 烧录文件在项目文件根目录“[firmware](./firmware/)”文件下，里面有对firmware文件版本的说明，选择合适的版本下载即可。

<p align="center" width="100%">
    <img src="image/10.png" alt="example">
    <img src="image/11.png" alt="example">
</p>


## 引脚总览

| RS485引脚  | ESP32引脚|
| :------------------: | :------------------:|
| TX         | IO22       |
| RX         | IO21       |
| CALLBACK         | IO17       |
| EN         | IO9       |

| WS2812引脚  | ESP32引脚|
| :------------------: | :------------------:|
| DATA         | IO4       |

| ME2107引脚  | ESP32引脚|
| :------------------: | :------------------:|
| EN         | IO16       |

| SD引脚  | ESP32引脚|
| :------------------: | :------------------:|
| MISO         | IO2       |
| MOSI         | IO15       |
| SCLK         | IO14       |
| CS         | IO13       |


## 相关测试

## 常见问题

* Q. 看了以上教程我还是不会搭建编程环境怎么办？
* A. 如果看了以上教程还不懂如何搭建环境的可以参考[LilyGo-Document](https://github.com/Xinyuan-LilyGO/LilyGo-Document)文档说明来搭建。

<br />

* Q. 为什么打开Arduino IDE时他会提醒我是否要升级库文件？我应该升级还是不升级？
* A. 选择不升级库文件，不同版本的库文件可能不会相互兼容所以不建议升级库文件。

<br />

* Q. 为什么我的板子一直烧录失败呢？
* A. 请按住“BOOT-0”按键重新下载程序。

## 项目
* [T-CAN485](./project/T-CAN485.pdf)

## 资料
* [MAX13487EESA+](./information/MAX13487EESA+.pdf)
* [SN65HVD231](./information/SN65HVD231.pdf)

## 依赖库
* [FastLED-3.7.0](https://github.com/FastLED/FastLED)