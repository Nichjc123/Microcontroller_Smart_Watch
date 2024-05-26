/**
 * @file hid_device
 * @author Nicholas Cantone
 * @date April 2024
 * @brief 
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Includes
 ***********************************************/

//BLE & HID related
#include "esp_hidd_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_err.h"
#include "esp_gap_bt_api.h"

//ESP includes
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_log.h"

/************************************************
 *  TYPE DEFINITIONS
 ***********************************************/

/* Configuration struct containing all global data needed for the HID protocol. */
typedef struct {
    esp_hidd_app_param_t app_param;
    esp_hidd_qos_param_t both_qos;
    uint8_t protocol_mode;
    SemaphoreHandle_t mouse_mutex;
    uint8_t buffer[REPORT_BUFFER_SIZE];
} HID_config_t;

/************************************************
 *  Functions
 ***********************************************/

/** @brief Initialize display device
 *
 *  Initialize GPIO and send all necessary commands
 *  for display startup
 *
 *  @return Void.
 */
void GPIO_init(void);

/** @brief Initialize Bluetooth for display device
 *
 *  Initialize BT stack, global configurations and settings
 *
 *  @return Void.
 */
void HIDDevice_BT_init(void);

/** @brief Bluetooth GAP (generic access profile) event handler.
 *
 *  Handles authentification, GAP pin input, and SSP related events
 *
 *  @param event GAP event
 *  @param param optional parameter passed with event
 *  @return Void.
 */
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/** @brief Bluetooth HID device event handler.
 *
 *  Handles initialization, app registration, device connect/disconnection, get/send HID report
 *
 *  @param event HID event
 *  @param param optional parameter passed with event
 *  @return void
 */
void esp_bt_hidd_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

/** @brief Send HID report
 *
 *  @param data HID report data
 *  @return Void.
 */
void send_media_report(uint8_t *data);

/** @brief Cleanup and app shut down.
 *
 *  @return Void.
 */
void bt_app_shut_down(void);

#ifdef __cplusplus
}
#endif