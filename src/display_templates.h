/**
 * @file display_templates.h
 * @author Nicholas Cantone
 * @date February 2024
 * @brief Display Library header for ST7735s LCD and ESP32
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Includes
 ***********************************************/

#include <inttypes.h>

/************************************************
 *  Symbolic Constants
 ***********************************************/

//maybe update width and heigh to be -1 if we never use true size
#define NUM_WIDTH      25
#define NUM_HEIGHT     42
#define NUM_SIZE       2100
#define NUM_CHUNKS     10

#define SC_WIDTH       9
#define SC_HEIGHT      42
#define SC_SIZE        756
#define SC_CHUNKS      4

#define ICON_WIDTH     30
#define ICON_HEIGHT    30
#define ICON_SIZE      1800
#define ICON_CHUNKS    8

/************************************************
 *  Globals
 ***********************************************/

extern uint8_t display_numbers[10][NUM_SIZE];
extern uint8_t media_icons[7][ICON_SIZE];

extern uint8_t semi_colon[SC_SIZE];

#ifdef __cplusplus
}
#endif