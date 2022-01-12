<h1 align = "center">🌟LilyGo T-CAN485🌟</h1>

# Introduce
![](img/T-CAN485-PINMAP-EN.jpg)
![](img/T-CAN485-DETAIL-EN.jpg)

## Onboard functions
- 1 x RS485 receiver.
- 1 x CAN receiver.
- 1 x TF card connector.
- 1 x WS2812 RGB LED light.
- 8 x Reserved programmable GPIO.

<h3 align = "left">Product 📷:</h3>

|  Product |  Product Link |
| :--------: | :---------: |
| T-CAN485 |  [AliExpress](https://pt.aliexpress.com/item/1005003624034092.html)   |


# Quick Start
## Arduino 
>- Click "File" in the upper left corner -> Preferences -> Additional Development >Board Manager URL -> Enter the URL in the input box
> `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pagespackage_esp32_index.json`
>-  Click OK and the software will be installed by itself. After installation, restart the Arduino IDE software.

## PlatfromIO
> - PlatformIO plug-in installation: Click on the extension on the left column -> search platformIO -> install the first plug-in
> - Click Platforms -> Embedded -> search Espressif 32 in the input box -> select the corresponding firmware installation


# Q&A
1. CAN bus protocol does not work properly.

- A:Depending on the ESP32 chip, the CAN controller register IER parameter needs to be changed. If it is a V3 version chip, you can use 0xEF, otherwise, use 0xFF.