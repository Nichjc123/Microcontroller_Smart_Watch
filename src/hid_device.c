/**
 * @file hid_device.h
 * @author Nicholas Cantone
 * @date February 2024
 * @brief BT HID related functinos and definitions
 *
 */

/************************************************
 *  INCLUDES
 ***********************************************/

/* ESP related*/
#include "esp_log.h"
#include "esp_hidd_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_gap_bt_api.h"
#include <string.h>
#include <inttypes.h>

/* GPIO related. */
#include "driver/gpio.h"

/* RTOS related. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/* Inter-compoent. */
#include "display_main.h"

/************************************************
 *  DEFINITIONS
 ***********************************************/

#define REPORT_PROTOCOL_MOUSE_REPORT_SIZE      (4)
#define REPORT_BUFFER_SIZE                     REPORT_PROTOCOL_MOUSE_REPORT_SIZE

/* Commands for media controls */
#define CTRL_NEXT                              0x01
#define CTRL_PREV                              0x02
#define CTRL_STOP                              0x04
#define CTRL_PLAYPAUSE                         0x08
#define CTRL_MUTE                              0x10
#define CTRL_VOLUP                             0x20
#define CTRL_VOLDOWN                           0x40

/* Push button pins. */
#define PB_1_PIN                               4
#define PB_2_PIN                               5

/************************************************
 *  GLOBALS
 ***********************************************/
static HID_config_t HID_config = {0};

// HID report descriptor for a generic mouse. The contents of the report are:
// 3 buttons, moving information for X and Y cursors, information for a wheel.
uint8_t hid_media_descriptor[] = {
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
	0x09, 0x01,                    // USAGE (Consumer Control)
	0xa1, 0x01,                    // COLLECTION (Application)
									// -------------------- common global items
	0x21, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,                    //   REPORT_SIZE (1)    - each field occupies 1 bit
									// -------------------- misc bits
	0x95, 0x05,                    //   REPORT_COUNT (5)
	0x09, 0xb5,                    //   USAGE (Scan Next Track)
	0x09, 0xb6,                    //   USAGE (Scan Previous Track)
	0x09, 0xb7,                    //   USAGE (Stop)
	0x09, 0xcd,                    //   USAGE (Play/Pause)
	0x09, 0xe2,                    //   USAGE (Mute)
	0x81, 0x06,                    //   INPUT (Data,Var,Rel)  - relative inputs
									// -------------------- volume up/down bits
	0x95, 0x02,                    //   REPORT_COUNT (2)
	0x09, 0xe9,                    //   USAGE (Volume Up)
	0x09, 0xea,                    //   USAGE (Volume Down)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)  - absolute inputs
									// -------------------- padding bit
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
	0xc0                           // END_COLLECTION
};

/* Global HID configuration */
const int hid_media_descriptor_len = sizeof(hid_media_descriptor);

/************************************************
 *  FUNCTIONS
 ***********************************************/

// send the buttons, change in x, and change in y
void send_media_report(uint8_t* data)
{
    uint8_t report_id;
    uint16_t report_size;
    xSemaphoreTake(HID_config.config_mutex, portMAX_DELAY);
    if (HID_config.protocol_mode == ESP_HIDD_REPORT_MODE) {
		report_id = 0;
		report_size = 1;
    } else {
		ESP_LOGE("send_mouse_rep", "ERROR invalid protocol mode");
    }
	vTaskDelay(50 / portTICK_PERIOD_MS);

	uint8_t clear = 0x00;

	//send control command
	ESP_LOGI("send_mouse_rep", "SENDING CONTROL SIGNAL");
    esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, report_id, report_size, data);

	//Clearing (raise key equivalent)
    vTaskDelay(50 / portTICK_PERIOD_MS);
	esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, report_id, report_size, &clear);
	
    xSemaphoreGive(HID_config.config_mutex);
}

/* GAP callback handler */
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    const char *TAG = "esp_bt_gap_cb";
    switch (event) {
    /* Authentification complete */
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    /* GAP pin request event (default of 1234 entered). */
    case ESP_BT_GAP_PIN_REQ_EVT: {
        ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }
    /* SSP related events. */
#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif
    default:
        ESP_LOGI(TAG, "event: %d", event);
        break;
    }
    return;
}

void bt_app_shut_down(void)
{
    if (HID_config.config_mutex) {
        vSemaphoreDelete(HID_config.config_mutex);
        HID_config.config_mutex = NULL;
    }
    return;
}

/* Bluetooth HID device callback handler. */
void esp_bt_hidd_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    static const char *TAG = "esp_bt_hidd_cb";
    switch (event) {
    case ESP_HIDD_INIT_EVT:
        if (param->init.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "setting hid parameters");
            /* Register HID device after initialization of stack. */
            esp_bt_hid_device_register_app(&HID_config.app_param, &HID_config.both_qos, &HID_config.both_qos);
        } else {
            ESP_LOGE(TAG, "init hidd failed!");
        }
        break;
    case ESP_HIDD_DEINIT_EVT:
        break;
    case ESP_HIDD_REGISTER_APP_EVT:
        if (param->register_app.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "setting hid parameters success!");
            ESP_LOGI(TAG, "setting to connectable, discoverable");
            /* Allow device to connect and discover. */
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            if (param->register_app.in_use) {
                ESP_LOGI(TAG, "start virtual cable plug!");
                esp_bt_hid_device_connect(param->register_app.bd_addr);
            }
        } else {
            ESP_LOGE(TAG, "setting hid parameters failed!");
        }
        break;
    case ESP_HIDD_UNREGISTER_APP_EVT:
        if (param->unregister_app.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "unregister app success!");
        } else {
            ESP_LOGE(TAG, "unregister app failed!");
        }
        break;
    case ESP_HIDD_OPEN_EVT:
        /* Connect to a new device. */
        if (param->open.status == ESP_HIDD_SUCCESS) {
            if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTING) {
                ESP_LOGI(TAG, "connecting...");
            } else if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED) {
                /* LOG BT address of connected device. */
                ESP_LOGI(TAG, "connected to %02x:%02x:%02x:%02x:%02x:%02x", param->open.bd_addr[0],
                         param->open.bd_addr[1], param->open.bd_addr[2], param->open.bd_addr[3], param->open.bd_addr[4],
                         param->open.bd_addr[5]);
                HID_config.config_mutex = xSemaphoreCreateMutex();
                memset(HID_config.buffer, 0, REPORT_BUFFER_SIZE);
                ESP_LOGI(TAG, "making self non-discoverable and non-connectable.");
                esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "open failed!");
        }
        break;
    case ESP_HIDD_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_CLOSE_EVT");
        if (param->close.status == ESP_HIDD_SUCCESS) {
            if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTING) {
                ESP_LOGI(TAG, "disconnecting...");
            } else if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "disconnected!");
                bt_app_shut_down();
                ESP_LOGI(TAG, "making self discoverable and connectable again.");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "close failed!");
        }
        break;
    case ESP_HIDD_SEND_REPORT_EVT:
        if (param->send_report.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "ESP_HIDD_SEND_REPORT_EVT id:0x%02x, type:%d", param->send_report.report_id,
                     param->send_report.report_type);
        } else {
            ESP_LOGE(TAG, "ESP_HIDD_SEND_REPORT_EVT id:0x%02x, type:%d, status:%d, reason:%d",
                     param->send_report.report_id, param->send_report.report_type, param->send_report.status,
                     param->send_report.reason);
        }
        break;
    case ESP_HIDD_REPORT_ERR_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_REPORT_ERR_EVT");
        break;
    case ESP_HIDD_SET_REPORT_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_SET_REPORT_EVT");
        break;
    case ESP_HIDD_SET_PROTOCOL_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_SET_PROTOCOL_EVT");
        if (param->set_protocol.protocol_mode == ESP_HIDD_BOOT_MODE) {
            ESP_LOGI(TAG, "  - boot protocol");
        } else if (param->set_protocol.protocol_mode == ESP_HIDD_REPORT_MODE) {
            ESP_LOGI(TAG, "  - report protocol");
        }
        xSemaphoreTake(HID_config.config_mutex, portMAX_DELAY);
        HID_config.protocol_mode = param->set_protocol.protocol_mode;
        xSemaphoreGive(HID_config.config_mutex);
        break;
    case ESP_HIDD_INTR_DATA_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_INTR_DATA_EVT");
        break;
    case ESP_HIDD_VC_UNPLUG_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_VC_UNPLUG_EVT");
        if (param->vc_unplug.status == ESP_HIDD_SUCCESS) {
            if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "disconnected!");
                bt_app_shut_down();
                ESP_LOGI(TAG, "making self discoverable and connectable again.");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "close failed!");
        }
        break;
    default:
        break;
    }
}
static QueueHandle_t gpio_evt_queue = NULL;

/* ISR Handler. */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/* Push button ISR, updates current media control and sends commands. */
static void push_button_handler(void* arg)
{
    uint32_t io_num;
	static uint8_t icon_index = 0;
    /* Media control commands. */
    uint8_t media_control_data[7] = {CTRL_NEXT, CTRL_PREV, CTRL_STOP, CTRL_PLAYPAUSE, CTRL_MUTE, CTRL_VOLUP, CTRL_VOLDOWN};
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			ESP_LOGI("push_button_handler", "Push button triggered on GPIO num: %"PRIu32"\n", io_num);
			if (io_num == PB_1_PIN)
			{
				//Left PB - Cycle Commands

				//Increment icon index
				icon_index = (icon_index + 1) % 6;
                
				//Display icon
				LCD_drawMediaIcon(icon_index);
			}
			else if (io_num == PB_2_PIN)
			{
				//Right PB
				send_media_report(&media_control_data[icon_index]);
			}
        }
    }
}

void GPIO_init(void)
{
	gpio_config_t io_conf = {};

    //configure GPIO with the given settings
	//interrupt on falling edge
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//bit mask of the pin
    io_conf.pin_bit_mask = ((1ULL << PB_1_PIN) | (1ULL << PB_2_PIN));
    io_conf.pull_up_en = true;

	gpio_config(&io_conf);

	//create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(push_button_handler, "push_button_handler", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PB_1_PIN, gpio_isr_handler, (void*) PB_1_PIN);
	gpio_isr_handler_add(PB_2_PIN, gpio_isr_handler, (void*) PB_2_PIN);
}


void HIDDevice_BT_init(void)
{
	GPIO_init();
    const char *TAG = "bt_init";
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    /* Init and enable bt stack and bluedroid. */
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);

    esp_bluedroid_init();
    esp_bluedroid_enable();
    
    esp_bt_gap_register_callback(esp_bt_gap_cb);

    esp_bt_dev_set_device_name("HID Media Controller");

    ESP_LOGI(TAG, "setting cod major, peripheral");
    esp_bt_cod_t cod;
    cod.major = ESP_BT_COD_MAJOR_DEV_PERIPHERAL;
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR);

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // Initialize HID SDP information and L2CAP parameters.
    // to be used in the call of `esp_bt_hid_device_register_app` after profile initialization finishes
    HID_config.app_param.name = "Media Controller";
    HID_config.app_param.description = "HID Media controller for ESP32";
    HID_config.app_param.provider = "ESP32";
    HID_config.app_param.subclass = ESP_HID_CLASS_MIC;
    HID_config.app_param.desc_list = hid_media_descriptor;
    HID_config.app_param.desc_list_len = hid_media_descriptor_len;

    memset(&HID_config.both_qos, 0, sizeof(esp_hidd_qos_param_t)); // don't set the qos parameters

    // Report Protocol Mode is the default mode, according to Bluetooth HID specification
    HID_config.protocol_mode = ESP_HIDD_REPORT_MODE;

    esp_bt_hid_device_register_callback(esp_bt_hidd_cb);

    ESP_LOGI(TAG, "starting hid device");
    esp_bt_hid_device_init();

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(TAG, "exiting");
}
