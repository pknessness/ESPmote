/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"


#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#if CONFIG_BT_SDP_COMMON_ENABLED
#include "esp_sdp_api.h"
#endif /* CONFIG_BT_SDP_COMMON_ENABLED */

#include "esp_hidd.h"
#include "esp_hid_gap.h"

//Added by pknessness
#include "esp_random.h"

static const char *TAG = "HID_DEV_DEMO";

static inline uint32_t swapEndian32(uint32_t val) {

    return ((0xFF000000 & val) >> 24) |

        ((0x00FF0000 & val) >> 8) |

        ((0x0000FF00 & val) << 8) |

        ((0x000000FF & val) << 24);

}

static inline uint16_t swapEndian16(uint16_t val) {

    return ((0xFF00 & val) >> 8) |

        ((0x00FF & val) << 8);

}

typedef struct
{
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;

typedef enum {
    // Output Reports (O_) - Wii to Wii Remote
    O_RUMBLE                         = 0x10,  // 1 byte
    O_PLAYER_LEDS                    = 0x11,  // 1 byte
    O_DATA_REPORTING_MODE            = 0x12,  // 2 bytes
    O_IR_CAMERA_ENABLE               = 0x13,  // 1 byte
    O_SPEAKER_ENABLE                 = 0x14,  // 1 byte
    O_STATUS_INFO_REQUEST            = 0x15,  // 1 byte
    O_WRITE_MEMORY_REGISTERS         = 0x16,  // 21 bytes
    O_READ_MEMORY_REGISTERS          = 0x17,  // 6 bytes
    O_SPEAKER_DATA                   = 0x18,  // 21 bytes
    O_SPEAKER_MUTE                   = 0x19,  // 1 byte
    O_IR_CAMERA_ENABLE_2             = 0x1a,  // 1 byte
    
    // Input Reports (I_) - Wii Remote to Wii
    I_STATUS_INFO                    = 0x20,  // BB BB LF 00 00 VV
    I_READ_MEMORY_REGISTERS_DATA     = 0x21,  // BB BB SE AA AA DD...
    I_ACK_OUTPUT_REPORT              = 0x22,  // BB BB RR EE
    
    // Data Reports (I_)
    I_CORE_BUTTONS                   = 0x30,  // BB BB
    I_CORE_BUTTONS_ACCEL             = 0x31,  // BB BB AA AA AA
    I_CORE_BUTTONS_8_EXT             = 0x32,  // BB BB + 8 EE
    I_CORE_BUTTONS_ACCEL_12_IR       = 0x33,  // BB BB AA AA AA + 12 II
    I_CORE_BUTTONS_19_EXT            = 0x34,  // BB BB + 19 EE
    I_CORE_BUTTONS_ACCEL_16_EXT      = 0x35,  // BB BB AA AA AA + 16 EE
    I_CORE_BUTTONS_10_IR_9_EXT       = 0x36,  // BB BB + 10 II + 9 EE
    I_CORE_BUTTONS_ACCEL_10_IR_6_EXT = 0x37,  // BB BB AA AA AA + 10 II + 6 EE
    I_EXT_21_BYTES                   = 0x3d,  // 21 EE (no core buttons)
    I_INTERLEAVED_36_IR              = 0x3e,  // Interleaved Core Buttons + Accelerometer + 36 IR bytes
    I_INTERLEAVED_36_IR_ALT          = 0x3f   // Interleaved Core Buttons + Accelerometer + 36 IR bytes (alternate)
} wii_report_id_t;

#if CONFIG_BT_HID_DEVICE_ENABLED
static local_param_t s_bt_hid_param = {0};
uint8_t WiiMoteHIDDescriptor[] = {
    /*
    |-----------------------------|
    |           Wiimote           |
    |-----------------------------|
    */  
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x10,        //   Report ID (16) //Rumble
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x11,        //   Report ID (17) //Player LEDs
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x12,        //   Report ID (18) //Data Reporting Mode
    0x95, 0x02,        //   Report Count (2)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x13,        //   Report ID (19) //IR Camera Enable
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x14,        //   Report ID (20) //Speaker Enable
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x15,        //   Report ID (21) //Status Information Request
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x16,        //   Report ID (22) //Write Memory and Registers
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x17,        //   Report ID (23) //Read Memory and Registers
    0x95, 0x06,        //   Report Count (6)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x18,        //   Report ID (24) //Speaker Data
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x19,        //   Report ID (25) //Speaker Mute
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x1A,        //   Report ID (26) IR Camera Enable 2
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x20,        //   Report ID (32) //Status Information
    0x95, 0x06,        //   Report Count (6)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x21,        //   Report ID (33) //Read Memory and Registers Data
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x22,        //   Report ID (34) //Acknowledge output report, return function result
    0x95, 0x04,        //   Report Count (4)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x30,        //   Report ID (48) //Data Report: Core Buttons
    0x95, 0x02,        //   Report Count (2)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x31,        //   Report ID (49) //Data Report: Core Buttons and Accelerometer
    0x95, 0x05,        //   Report Count (5)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x32,        //   Report ID (50) //Data Report: Core Buttons with 8 Extension Bytes
    0x95, 0x0A,        //   Report Count (10)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x33,        //   Report ID (51) //Data Report: Core Buttons with 12 IR Bytes
    0x95, 0x11,        //   Report Count (17)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x34,        //   Report ID (52) //Data Report: Core Buttons with 19 Extension Bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x35,        //   Report ID (53) //Data Report: Core Buttons and Accelerometer with 16 Extension Bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x36,        //   Report ID (54) //Data Report: Core Buttons with 10 IR bytes and 9 Extension Bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x37,        //   Report ID (55) //Data Report: Core Buttons and Accelerometer with 10 IR bytes and 6 Extension Bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x3D,        //   Report ID (61) //Data Report: 21 Extension Bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x3E,        //   Report ID (62) //Data Report: Interleaved Core Buttons and Accelerometer with 36 IR bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x3F,        //   Report ID (63) //Data Report: Interleaved Core Buttons and Accelerometer with 36 IR bytes
    0x95, 0x15,        //   Report Count (21)
    0x09, 0x01,        //   Usage (0x01)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};

static esp_hid_raw_report_map_t bt_report_maps[] = {
    {
        .data = WiiMoteHIDDescriptor,
        .len = sizeof(WiiMoteHIDDescriptor)
    },
};

static esp_hid_device_config_t bt_hid_config = {
    .vendor_id          = 0x057e,
    .product_id         = 0x0306,
    .version            = 0x0100,
    .device_name        = "Nintendo RVL-CNT-01",
    .manufacturer_name  = "Nintendo",
    .serial_number      = "1234567890",
    .report_maps        = bt_report_maps,
    .report_maps_len    = 1
};

// Operational Variables
bool continuousReporting = false;
uint8_t reportingMode = 0x30;
bool rumbling = false;

// send the buttons, change in x, and change in y
void send_mouse(uint8_t buttons, char dx, char dy, char wheel)
{
    static uint8_t buffer[4] = {0};
    buffer[0] = buttons;
    buffer[1] = dx;
    buffer[2] = dy;
    buffer[3] = wheel;
    esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0, buffer, 4);
}

void bt_hid_demo_task(void *pvParameters)
{
    static const char* help_string = "########################################################################\n"\
    "BT hid mouse demo usage:\n"\
    "You can input these value to simulate mouse: 'q', 'w', 'e', 'a', 's', 'd', 'h'\n"\
    "q -- click the left key\n"\
    "w -- move up\n"\
    "e -- click the right key\n"\
    "a -- move left\n"\
    "s -- move down\n"\
    "d -- move right\n"\
    "h -- show the help\n"\
    "########################################################################\n";
    printf("%s\n", help_string);
    char c;
    while (1) {
        c = fgetc(stdin);
        switch (c) {
        case 'q':
            send_mouse(1, 0, 0, 0);
            break;
        case 'w':
            send_mouse(0, 0, -10, 0);
            break;
        case 'e':
            send_mouse(2, 0, 0, 0);
            break;
        case 'a':
            send_mouse(0, -10, 0, 0);
            break;
        case 's':
            send_mouse(0, 0, 10, 0);
            break;
        case 'd':
            send_mouse(0, 10, 0, 0);
            break;
        case 'h':
            printf("%s\n", help_string);
            break;
        default:
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

////first byte
//static const uint8_t BUTTONS_SHIFT_DPAD_LEFT = 0;
//static const uint8_t BUTTONS_SHIFT_DPAD_RIGHT = 1;
//static const uint8_t BUTTONS_SHIFT_DPAD_DOWN = 2;
//static const uint8_t BUTTONS_SHIFT_DPAD_UP = 3;
//static const uint8_t BUTTONS_SHIFT_DPAD_PLUS = 4;
//
////second byte
//static const uint8_t BUTTONS_SHIFT_TWO = 0;
//static const uint8_t BUTTONS_SHIFT_ONE = 1;
//static const uint8_t BUTTONS_SHIFT_B = 2;
//static const uint8_t BUTTONS_SHIFT_A = 3;
//static const uint8_t BUTTONS_SHIFT_MINUS = 4;
//
//static const uint8_t BUTTONS_SHIFT_HOME = 7;

// send the buttons, change in x, and change in y
void mote_output_data_core()
{
    static uint8_t buffer[2] = {0};
	int rand = esp_random();
    buffer[0] = rand & 0xFF;
    buffer[1] = (rand >> 8) & 0xFF;
    esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x30, buffer, 2);
}

void mote_hid_main_task(void *pvParameters)
{
	static const char *TAGW = "WIIMOTE_SEND";

    static const char* help_string = "########################################################################\n"\
    "BT hid mouse demo usage:\n"\
    "You can input these value to simulate mouse: 'q', 'w', 'e', 'a', 's', 'd', 'h'\n"\
    "q -- click the left key\n"\
    "w -- move up\n"\
    "e -- click the right key\n"\
    "a -- move left\n"\
    "s -- move down\n"\
    "d -- move right\n"\
    "h -- show the help\n"\
    "########################################################################\n";
    printf("%s\n", help_string);
    char c = 0;
    while (1) {
		c = fgetc(stdin);
		if(c != 0){
			mote_output_data_core();
			ESP_LOGI()
		}
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void bt_hid_task_start_up(void)
{
//    xTaskCreate(bt_hid_demo_task, "bt_hid_demo_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, &s_bt_hid_param.task_hdl);
	xTaskCreate(mote_hid_main_task, "mote_hid_main_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, &s_bt_hid_param.task_hdl);
	return;
}

void bt_hid_task_shut_down(void)
{
    if (s_bt_hid_param.task_hdl) {
        vTaskDelete(s_bt_hid_param.task_hdl);
        s_bt_hid_param.task_hdl = NULL;
    }
}

static void bt_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;
    static const char *TAG = "HID_DEV_BT";
	static const char *TAGW = "WII_OUTPUT";

    switch (event) {
    case ESP_HIDD_START_EVENT: {
        if (param->start.status == ESP_OK) {
            ESP_LOGI(TAG, "START OK");
            ESP_LOGI(TAG, "Setting to connectable, discoverable");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        } else {
            ESP_LOGE(TAG, "START failed!");
        }
        break;
    }
    case ESP_HIDD_CONNECT_EVENT: {
        if (param->connect.status == ESP_OK) {
            ESP_LOGI(TAG, "CONNECT OK");
            ESP_LOGI(TAG, "Setting to non-connectable, non-discoverable");
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            bt_hid_task_start_up();
        } else {
            ESP_LOGE(TAG, "CONNECT failed!");
        }
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT: {
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_OUTPUT_EVENT: {
        //ESP_LOGI(TAGW, "DATA FROM WII[%u]: %8s ID: 0x%2x, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        //ESP_LOG_BUFFER_HEX(TAGW, param->output.data, param->output.length);
		
		//bit 0 of any output report is for rumble
		if(param->output.data[0] & 0x01 && rumbling == false){
			rumbling = true;
			ESP_LOGI(TAGW, "RUMBLING ON");
		}else if(!(param->output.data[0] & 0x01) && rumbling == true){
			rumbling = false;
			ESP_LOGI(TAGW, "RUMBLING OFF");
		}
		
		//bit 1 of any output report is requesting an acknowledgement input report
		if(param->output.data[0] & 0x02){
			ESP_LOGI(TAGW, "REQEUSTING RESPONSE");
		}
		uint32_t offset;
		uint16_t size;
		
		switch (param->feature.report_id) {
		    case O_RUMBLE:
		        // 1 byte - bit 0 controls rumble
				//ESP_LOGI(TAG, "RUMBLING");
		        //ignore this because actually bit 0 of any report is for rumble, this bit is just for only rumble
				break;
		        
		    case O_PLAYER_LEDS:
		        // 1 byte - bits 4-7 control LEDs 1-4
				ESP_LOGI(TAGW, "LEDS %c %c %c %c", 
					(param->output.data[0] & 0x10) ? '+' : '_', 
					(param->output.data[0] & 0x20) ? '+' : '_', 
					(param->output.data[0] & 0x40) ? '+' : '_', 
					(param->output.data[0] & 0x80) ? '+' : '_');
		        break;
		        
		    case O_DATA_REPORTING_MODE:
		        // 2 bytes - TT MM (TT bit2 = continuous, MM = mode 0x30-0x3f)
				continuousReporting = (param->output.data[0] & 0x04);
				reportingMode = param->output.data[1];
				ESP_LOGI( TAGW, "Data Reporting: TT[%2x] MM[%x]", continuousReporting, reportingMode);
		        break;
		        
		    case O_IR_CAMERA_ENABLE:
		        // 1 byte - bit 2 = ON/OFF
				ESP_LOGI( TAGW, "Written %2x to IR Camera 1", param->output.data[0]);
		        break;
		        
		    case O_SPEAKER_ENABLE:
		        // 1 byte - bit 2 = ON/OFF
				ESP_LOGI( TAGW, "Written %2x to Speaker Enable", param->output.data[0]);
		        break;
		        
		    case O_STATUS_INFO_REQUEST:
		        // 1 byte - request status report
				ESP_LOGI( TAGW, "Written %2x to Status Info", param->output.data[0]);

		        break;
		        
		    case O_WRITE_MEMORY_REGISTERS:
		        // 21 bytes - write to memory/registers
				//memcpy(&offset, param->output.data + 2, 3);
				offset = (param->output.data[1] << 16) | (param->output.data[2] << 8) | (param->output.data[3]);
//				memset(&size, 0, 2);
				//memcpy(&size, param->output.data + 5, 1);
				size = param->output.data[4];
				if((param->output.data[0] & 0x04)){
					ESP_LOGI( TAGW, "Attempting to write %d bytes to control registers at %6x", size, offset);
				}else{
					ESP_LOGI( TAGW, "Attempting to write %d bytes to EEPROM Memory at %6x", size, offset);
				}
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data + 5, size); //TODO: make sure doesnt buffer overflow
				break;
		        
		    case O_READ_MEMORY_REGISTERS:
		        // 6 bytes - read from memory/registers
				offset = (param->output.data[1] << 16) | (param->output.data[2] << 8) | (param->output.data[3]);
//				memset(&size, 0, 2);
				size = (param->output.data[4] << 8) | (param->output.data[5]);
				if((param->output.data[0] & 0x04)){
					ESP_LOGI( TAGW, "Attempting to read %d bytes from control registers at %6x", size, offset);
				}else{
					ESP_LOGI( TAGW, "Attempting to read %d bytes from EEPROM Memory at %6x", size, offset);
				}
		        break;
		        
		    case O_SPEAKER_DATA:
		        // 21 bytes - audio data for speaker
				ESP_LOGI( TAGW, "%d bytes of Speaker Data", param->output.data[0]);
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data + 1, param->output.data[0]); //TODO: make sure doesnt buffer overflow
		        break;
		        
		    case O_SPEAKER_MUTE:
		        // 1 byte - bit 2 = mute when set
				ESP_LOGI( TAGW, "Written %2x to Speaker Mute", param->output.data[0]);
		        break;
		        
		    case O_IR_CAMERA_ENABLE_2:
		        // 1 byte - bit 2 = ON/OFF (alternate)
				ESP_LOGI( TAGW, "Written %2x to IR Camera 2", param->output.data[0]);
		        break;
		        
		    default:
		        // Unknown output report ID
				ESP_LOGW(TAGW, "UNKNOWN REPORT ID[%u]: %8s ID: 0x%2x, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data, param->output.length);
		        break;
		}
        break;
    }
    case ESP_HIDD_FEATURE_EVENT: {
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT: {
        if (param->disconnect.status == ESP_OK) {
            ESP_LOGI(TAG, "DISCONNECT OK");
            bt_hid_task_shut_down();
            ESP_LOGI(TAG, "Setting to connectable, discoverable again");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        } else {
            ESP_LOGE(TAG, "DISCONNECT failed!");
        }
        break;
    }
    case ESP_HIDD_STOP_EVENT: {
        ESP_LOGI(TAG, "STOP");
        break;
    }
    default:
        break;
    }
    return;
}

#if CONFIG_BT_SDP_COMMON_ENABLED
static void esp_sdp_cb(esp_sdp_cb_event_t event, esp_sdp_cb_param_t *param)
{
    switch (event) {
    case ESP_SDP_INIT_EVT:
        ESP_LOGI(TAG, "ESP_SDP_INIT_EVT: status:%d", param->init.status);
        if (param->init.status == ESP_SDP_SUCCESS) {
            esp_bluetooth_sdp_dip_record_t dip_record = {
                .hdr =
                    {
                        .type = ESP_SDP_TYPE_DIP_SERVER,
                    },
                .vendor           = bt_hid_config.vendor_id,
                .vendor_id_source = ESP_SDP_VENDOR_ID_SRC_BT,
                .product          = bt_hid_config.product_id,
                .version          = bt_hid_config.version,
                .primary_record   = true,
            };
            esp_sdp_create_record((esp_bluetooth_sdp_record_t *)&dip_record);
        }
        break;
    case ESP_SDP_DEINIT_EVT:
        ESP_LOGI(TAG, "ESP_SDP_DEINIT_EVT: status:%d", param->deinit.status);
        break;
    case ESP_SDP_SEARCH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_SDP_SEARCH_COMP_EVT: status:%d", param->search.status);
        break;
    case ESP_SDP_CREATE_RECORD_COMP_EVT:
        ESP_LOGI(TAG, "ESP_SDP_CREATE_RECORD_COMP_EVT: status:%d, handle:0x%x", param->create_record.status,
                 param->create_record.record_handle);
        break;
    case ESP_SDP_REMOVE_RECORD_COMP_EVT:
        ESP_LOGI(TAG, "ESP_SDP_REMOVE_RECORD_COMP_EVT: status:%d", param->remove_record.status);
        break;
    default:
        break;
    }
}
#endif /* CONFIG_BT_SDP_COMMON_ENABLED */

#endif

void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_LOGI(TAG, "setting hid gap, mode:%d", HID_DEV_MODE);
    ret = esp_hid_gap_init(HID_DEV_MODE);
    ESP_ERROR_CHECK( ret );

#if CONFIG_BT_HID_DEVICE_ENABLED
    ESP_LOGI(TAG, "setting device name");
    esp_bt_gap_set_device_name(bt_hid_config.device_name);
    ESP_LOGI(TAG, "setting cod major, peripheral");
    esp_bt_cod_t cod = {0};
    cod.major = ESP_BT_COD_MAJOR_DEV_PERIPHERAL;
    cod.minor = ESP_BT_COD_MINOR_PERIPHERAL_JOYSTICK;
	cod.service = ESP_BT_COD_SRVC_LMTD_DISCOVER;
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "setting bt device");
    ESP_ERROR_CHECK(
        esp_hidd_dev_init(&bt_hid_config, ESP_HID_TRANSPORT_BT, bt_hidd_event_callback, &s_bt_hid_param.hid_dev));
#if CONFIG_BT_SDP_COMMON_ENABLED
    ESP_ERROR_CHECK(esp_sdp_register_callback(esp_sdp_cb));
    ESP_ERROR_CHECK(esp_sdp_init());
#endif /* CONFIG_BT_SDP_COMMON_ENABLED */
#endif /* CONFIG_BT_HID_DEVICE_ENABLED */
}
