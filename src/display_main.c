/**
 * @file display_main.c
 * @author Nicholas Cantone
 * @date February 2024
 * @brief  This file contains the functions responsible for displaying data on the LCD
 *
 */


/************************************************
 *   INCLUDES
 ***********************************************/

#include "esp_lcd_panel_io.h"
#include "driver/spi_master.h"
#include "esp_sntp.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "display_main.h"

/************************************************
 *  DEFINITIONS	
 ***********************************************/
#define HIGH              1
#define LOW               0

// ST7735S driver commands (ref: pdf datasheet v1.4)
#define CMD_SWRESET 0x01 // Software Reset 
#define CMD_SLPIN   0x10 // Sleep In 
#define CMD_SLPOUT  0x11 // Sleep Out
#define CMD_INVOFF  0x20 // Display Inversion Off
#define CMD_INVON   0x21 // Display Inversion On
#define CMD_GAMSET  0x26 // Gamma Set
#define CMD_DISPOFF 0x28 // Display Off
#define CMD_DISPON  0x29 // Display On
#define CMD_CASET   0x2A // Column Address Set 
#define CMD_RASET   0x2B // Row Address Set 
#define CMD_RAMWR   0x2C // Memory Write 
#define CMD_TEOFF   0x34 // Tearing Effect Line OFF 
#define CMD_TEON    0x35 // Tearing Effect Line ON 
#define CMD_MADCTL  0x36 // Memory Data Access Control 
#define CMD_IDMOFF  0x38 // Idle Mode Off 
#define CMD_IDMON   0x39 // Idle Mode On 
#define CMD_COLMOD  0x3A // Interface Pixel Format

/************************************************
 *  FUNCTIONS
 ***********************************************/

//Abstraction for sending commands
void LCD_sendCommand(spi_device_handle_t spi, const uint8_t cmd)
{
	//Set data /command line low
	gpio_set_level(PIN_DATA_NCOMMAND, LOW);

	//Send SPI transfer
	esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8;                   //Command is 8 bits
    t.tx_buffer = &cmd;             //The data is the cmd itself

	ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

//Abstraction for sending data
void LCD_sendData(spi_device_handle_t spi, const uint8_t *data, const int len)
{
	//Set data /command line high
	gpio_set_level(PIN_DATA_NCOMMAND, HIGH);

	//Send Spi transfer
	esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) {
        return;    //empty message
    }
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;             //length in bits, len in bytes
    t.tx_buffer = data;             

    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);
}

//TODO: figure out buffering and how transactions will work in terms of size
void LCD_init(spi_device_handle_t spi)
{
	//Initialize remaining pins
	gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = ((1ULL << PIN_DATA_NCOMMAND) | (1ULL << PIN_RESET) | (1ULL << PIN_CHIP_SEL));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);

	//Reset display
    gpio_set_level(PIN_RESET, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RESET, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);

	//Turn off sleep mode
	LCD_sendCommand(spi, CMD_SLPOUT);

	//Required delay after exiting sleep
	vTaskDelay(120 / portTICK_PERIOD_MS);
	
	/*  Send initialization commands */

	//Memory Access Control
	LCD_sendCommand(spi, CMD_MADCTL);
	//set memory write/read direction
	uint8_t mem_acc_ctl_flags = 0x00;
	LCD_sendData(spi, &mem_acc_ctl_flags, 1);

	//Interface pixel format
	LCD_sendCommand(spi, CMD_COLMOD);
	uint8_t pixel_format = PIXEL_FORMAT;
	LCD_sendData(spi, &pixel_format, 1);

	//Predefined Gamma
	LCD_sendCommand(spi, CMD_GAMSET);
	uint8_t gamma_curve = GAMMA_CURVE;
	LCD_sendData(spi, &gamma_curve, 1);

	//Display inversion off
	LCD_sendCommand(spi, CMD_INVOFF);

	//Turn on Display
	LCD_sendCommand(spi, CMD_DISPON);
}

void LCD_sendFrame(spi_device_handle_t spi, uint8_t* buffer, const int frame_size, const int chunk_number)
{
	const int chunk_size = frame_size / chunk_number; //for chunk sends (should be divisibe by size)

	/* Activate mem write. */
	LCD_sendCommand(spi, CMD_RAMWR);

	//send chunks
	for (int i = 0; i < chunk_number; i ++)
	{
		LCD_sendData(spi, buffer + (i * chunk_size), chunk_size);
	}
}

void LCD_setDrawingWindow(spi_device_handle_t spi, const uint8_t x, const uint8_t y, const uint8_t w, const uint8_t h)
{
	//set collumn address
	LCD_sendCommand(spi, CMD_CASET);
	uint8_t window_position_buffer[4];
	window_position_buffer[0] = (x >> 8)        & 0x00FF; //collumn start MSB
	window_position_buffer[1] = (x)             & 0x00FF; //collumn start LSB
	window_position_buffer[2] = ((x + w) >> 8)  & 0x00FF; //collumn end MSB
	window_position_buffer[3] = (x + w)         & 0x00FF; //collumn end LSB 
	LCD_sendData(spi, &window_position_buffer, 4);

	//setting row address
	LCD_sendCommand(spi, CMD_RASET);
	window_position_buffer[0] = (y >> 8)        & 0x00FF; //collumn start MSB
	window_position_buffer[1] = (y)             & 0x00FF; //collumn start LSB
	window_position_buffer[2] = ((y + h) >> 8)  & 0x00FF; //collumn end MSB
	window_position_buffer[3] = (y + h)         & 0x00FF; //collumn end LSB 
	LCD_sendData(spi, &window_position_buffer, 4);
}