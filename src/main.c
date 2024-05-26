/**
 * @file main.c
 * @author Nicholas Cantone
 * @date February 2024
 * @brief 
 *
 * TODO: Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 * @see http://www.stack.nl/~dimitri/doxygen/docblocks.html
 * @see http://www.stack.nl/~dimitri/doxygen/commands.html
 */


/************************************************
 *  INCLUDE
 ***********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

//RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//ESP includes
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_log.h"

//Display related
#include "display_main.h"
#include "display_templates.h"

//BT related
#include "hid_device.h"

/************************************************
 *  GLOBALS
 ***********************************************/

static spi_device_handle_t spi;

/************************************************
 *  FUNCTIONS
 ***********************************************/

void LCD_drawMediaIcon(uint8_t icon_index)
{
    LCD_setDrawingWindow(
        spi,
        ICON_DISPLAY_X_OFFSET, //x pos
        ICON_DISPLAY_Y_OFFSET, //y pos
        ICON_WIDTH - 1,        //width
        ICON_HEIGHT - 1        //height
    );
    LCD_sendFrame(
        spi,
        &media_icons[icon_index],
        ICON_SIZE,
        ICON_CHUNKS
    );
};

void vTaskUpdateDisplayTime( void * pvParameters )
{
	//TODO: atm static initialized in future can do with pushbuttons
	static int hour = 5;
	static int mins = 40;

	//have a 1 min delay then imcrement mins,
	for ( ;; )
	{
		//populate array of values
		int time[5] = {hour / 10, hour % 10, 0, mins / 10, mins % 10}; // [H,H, (blank), M,M]

		for (int i = 0; i < 5; i++)
		{
			LCD_setDrawingWindow(
				spi,
				TIME_DISPLAY_X_OFFSET + ((i < 3) ? (i * NUM_WIDTH) : ((i-1) * NUM_WIDTH) + SC_WIDTH), //x pos
				TIME_DISPLAY_Y_OFFSET,                                                                //y pos
				(i == 2) ? SC_WIDTH - 1 : NUM_WIDTH - 1,                                              //width
				NUM_HEIGHT - 1                                                                        //height
			);
			LCD_sendFrame(
				spi,
				(i == 2) ? &semi_colon : &display_numbers[time[i]],
				(i == 2) ? SC_SIZE : NUM_SIZE,
				NUM_CHUNKS
			);
		}
		vTaskDelay(60000 / portTICK_PERIOD_MS);
		if (mins == 59) 
		{
			mins = 0;
			if (hour == 23)
			{
				hour = 0;
			}
			else 
			{
				hour++;
			}
		}
		else 
		{
			mins++;
		}
	}
};

void app_main(void)
{
	/*********************************
		Display Related Initialization
	**********************************/
	esp_err_t ret;

	spi_bus_config_t bus_config = {
		.mosi_io_num = PIN_SDA,
		.miso_io_num = -1,
		.sclk_io_num = PIN_SCK,
		.max_transfer_sz = MAX_TRANSFER_SIZE + 8
	};
	//Init bus
	ret = spi_bus_initialize(HOST_DEVICE, &bus_config, SPI_DMA_CH_AUTO);
	ESP_ERROR_CHECK(ret);

	spi_device_interface_config_t dev_config = {
		.mode = 0,   //SPI mode 0
		.clock_speed_hz = 10000000,  //10MHz clock (max 15)
		.spics_io_num = PIN_CHIP_SEL, //CS pin
		.queue_size = 7, //TODO: stable queue size?
	};

	//Attach device to bus
	ret = spi_bus_add_device(HOST_DEVICE, &dev_config, &spi);
    ESP_ERROR_CHECK(ret);

	LCD_init(spi);
	
	uint8_t* frame_buffer = malloc(FRAME_SIZE);
	//populate buffer
	for (int i = 0; i < FRAME_SIZE; i++)
	{
		//frame_buffer[i] = 128 + 127 * sin((3.14159 * (i) /  64.0) + 1);
		frame_buffer[i] = 125;
	}
	//make white
	LCD_setDrawingWindow(spi,X_OFFSET,Y_OFFSET,WIDTH - 1, HEIGHT - 1);
	LCD_sendFrame(spi, frame_buffer, FRAME_SIZE, 128);
	free(frame_buffer);

	/******************************
		BL Initialization
	*******************************/

	HIDDevice_BT_init();

	/****************
		Task Creation
	*****************/

	BaseType_t xRet;
	// may not need handle tbd...
	TaskHandle_t xHandle = NULL;

	xRet = xTaskCreate(
		vTaskUpdateDisplayTime,
		"UPDATE_DISPLAY_TIME",
		configMINIMAL_STACK_SIZE,
		NULL,
		1,
		NULL
	);

}