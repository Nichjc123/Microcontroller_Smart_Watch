/**
 * @file display_main.h
 * @author Nicholas Cantone
 * @date February 2024
 * @brief Display Library header for ST7735s LCD and ESP32
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/spi_master.h"

/************************************************
 *  DEFINITIONS
 ***********************************************/

#define X_OFFSET    2 //slight offsets in actual position 
#define Y_OFFSET    1

#define TIME_DISPLAY_X_OFFSET   11
#define TIME_DISPLAY_Y_OFFSET   44

#define ICON_DISPLAY_X_OFFSET   49
#define ICON_DISPLAY_Y_OFFSET   92

// no MISO pin lcd does not send data
#define PIN_DATA_NCOMMAND 12 //A0 D/C (high data low command)
#define PIN_CHIP_SEL      13 //CS (active low)
#define PIN_SDA           14 //SDA or MOSI
#define PIN_SCK           27 //SCK
#define PIN_RESET         0  //RST (active low)

#define WIDTH             128
#define HEIGHT            128

#define HOST_DEVICE       SPI2_HOST

#define PIXEL_FORMAT      0x55
#define PIXEL_SIZE        2
#define GAMMA_CURVE       0x01
#define MAX_TRANSFER_SIZE 3072  //frame_size / 16
#define FRAME_SIZE        32768 //height * width * pixel_size

/************************************************
 *  FUNCTIONS
 ***********************************************/

/** @brief Initialize display device
 *
 *  Initialize GPIO and send all necessary commands
 *  for display startup
 *
 *  @param spi The device handle used for sending commands.
 *  @return Void.
 */
void LCD_init(spi_device_handle_t spi);

/** @brief Send A Frame of data to display
 *
 *  Send the contents of the buffer to the LCD via SPI
 *
 *  @param spi The device handle used for sending commands.
 *  @param buffer Source of display data
 *  @param frame_size Size of frame to be sent (used when sending portions of the display)
 *  @param chunk_number Number of chunks (data will be seperated into chunks in order to increase speed)
 *  @return Void.
 */
void LCD_sendFrame(spi_device_handle_t spi, uint8_t* buffer, const int frame_size, const int chunk_number);

/** @brief Send Data to Display
 *
 *  Send the contents of data to the LCD via SPI
 *
 *  @param spi The device handle used for sending commands.
 *  @param data Source of display data
 *  @param len Length of data buffer
 *  @return Void.
 */
void LCD_sendData(spi_device_handle_t spi, const uint8_t *data, const int len);

/** @brief Set the drawing window
 *
 *  Specify the region at which we will be drawing on the display (this must be done before send frame)
 *
 *  @param spi The device handle used for sending commands.
 *  @param x x coordinate of drawing window (top left)
 *  @param y y coordinate of drawing window (top left)
 *  @param w width of drawing window
 *  @param h height of drawing window
 *  @return Void.
 */
void LCD_setDrawingWindow(spi_device_handle_t spi, const uint8_t x, const uint8_t y, const uint8_t w, const uint8_t h);

/** @brief Send Command to Display
 *
 *  Send a command to the LCD display, used for initialization and sending general configuration instructions.
 *
 *  @param spi The device handle used for sending commands.
 *  @param cmd single byte command to be sent to the display 
 *  @return Void.
 */
void LCD_sendCommand(spi_device_handle_t spi, const uint8_t cmd);

/** @brief Draw a media icon
 *
 *  Wrapper function that draws a media control icon to the display
 *
 *  @param icon_index Index of the desired icon to be drawn 
 *  @return Void.
 */
void LCD_drawMediaIcon(uint8_t icon_index);

#ifdef __cplusplus
}
#endif