# ESP32 Microcontroller Smartwatch

This project is a custom smartwatch using the ESP32 microcontroller. The watch features a 1.44” IPS TFT display, and media control via Bluetooth wifi integration and weather notificaitons are currently under development. The entire project was developed from scratch, without relying on external libraries (except for the ones provided by ESP).


## The display 

The display code in this project is a custom library specifically designed to interface with the ST7735s LCD screen using SPI communication. It handles the initialization of the LCD, including setting up the appropriate pins and configuring the SPI parameters. The library provides functions to send commands and data to the LCD, allowing for the display of the current time and various symbols. It is optimized for efficient communication, ensuring smooth and responsive updates to the screen. By writing this display code from scratch, the project achieves a high degree of control and customization over the visual output, tailored specifically for the ESP32 microcontroller.


## Bluetooth HID Device

The Bluetooth HID media controller code in this project is a custom implementation that enables the ESP32 to function as a Bluetooth Human Interface Device (HID). This custom HID profile allows the smartwatch to communicate with paired devices and control media playback. The code handles the initialization of the Bluetooth stack, setting up the HID service, and managing the necessary HID reports. Functions are provided to send commands for play/pause, next track, previous track, and volume adjustments. By creating the Bluetooth HID profile from scratch, the project ensures precise and reliable media control functionality, seamlessly integrating with various Bluetooth-enabled devices for a smooth user experience.

## WIFI integration

WIP

## Custom PCB Design

WIP

## Images (Updated May '24)

![Watch Image 1, MAY 2024](./images/may24-im1.HEIC "Watch #1 May '24")

![Watch Image 2, MAY 2024](./images/may24-im2.HEIC "Watch #2 May '24")