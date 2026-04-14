/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <signal.h>
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
#include "driver/gpio.h"
#include "esp_mac.h"
#include "mpu6050.h"

static const char *TAG = "HID_DEV_DEMO";
static const char *TAGSEND = "WIIMOTE_OUTPUT";
static const char *TAGW = "WII_OUTPUT";


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

typedef enum {
    // Output Reports (O_) - Wii to Wii Remote
    ACK_SUCCESS                         = 0x00,
    ACK_ERROR                           = 0x03,
    ACK_UNKNOWN1                        = 0x04,
    ACK_UNKNOWN2                        = 0x05,
	ACK_INACTIVE_EXTENSION              = 0x07, //for when writing to an unconnected extension like deactive motion plus
    ACK_UNKNOWN3                        = 0x08,
} ack_error_code_t;

typedef enum {
    // Output Reports (O_) - Wii to Wii Remote
    READ_SUCCESS                        = 0x00,
    READ_WRITE_ONLY                     = 0x07,
    READ_NONEXISTENT                    = 0x08,
} read_error_code_t;

// Decrypted last 2 bytes (lowest 16 bits) for each device
const uint16_t EXT_NONE                        = 0x0000;  // None
const uint16_t EXT_NUNCHUK                     = 0x0000;  // Nunchuk
const uint16_t EXT_CLASSIC_CONTROLLER          = 0x0101;  // Classic Controller 
const uint16_t EXT_WII_MOTION_PLUS_INACTIVE    = 0x0005;  // Inactive Wii Motion Plus (Built-in)
const uint16_t EXT_WII_MOTION_PLUS_ACTIVE      = 0x0405;  // Activated Wii Motion Plus
const uint16_t EXT_WII_MOTION_PLUS_NUNCHUK_PASSTHROUGH = 0x0505;  // Activated Wii Motion Plus in Nunchuck passthrough mode
const uint16_t EXT_WII_MOTION_PLUS_CLASSIC_PASSTHROUGH = 0x0705;  // Activated Wii Motion Plus in Classic Controller passthrough mode

const uint16_t EXTENSION_A4_TAG = 0x20A4; //in reverse because memcpy
const uint16_t EXTENSION_A6_TAG = 0x20A6; //in reverse because memcpy

// Button GPIO definitions
//GPIO 34-39 CANNOT SOFTWARE PULLUP
#define BUTTON_PIN_A        GPIO_NUM_32 //
#define BUTTON_PIN_B        GPIO_NUM_27 //
#define BUTTON_PIN_ONE      GPIO_NUM_18 //
#define BUTTON_PIN_TWO      GPIO_NUM_19 //
#define BUTTON_PIN_PLUS     GPIO_NUM_4 //
#define BUTTON_PIN_MINUS    GPIO_NUM_23 //
#define BUTTON_PIN_HOME     GPIO_NUM_34 //
#define BUTTON_PIN_UP       GPIO_NUM_26 //
#define BUTTON_PIN_DOWN     GPIO_NUM_33 //
#define BUTTON_PIN_LEFT     GPIO_NUM_35 //  
#define BUTTON_PIN_RIGHT    GPIO_NUM_25 //

//IMU (I2C)
#define I2C_MASTER_SCL_IO    22 // SCL pin
#define I2C_MASTER_SDA_IO    21 // SDA pin
#define I2C_MASTER_FREQ_HZ   400000
#define I2C_MASTER_NUM       I2C_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0
mpu6050_handle_t mpu6050_handle;

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
    .device_name        = "Nintendo RVL-CNT-01-TR",
    .manufacturer_name  = "Nintendo",
    .serial_number      = "1234567890",
    .report_maps        = bt_report_maps,
    .report_maps_len    = 1
};

// Operational Variables
bool continuousReporting = false;
uint8_t reportingMode = 0x30;
bool rumbling = false;
bool speaker_enable = false;
uint8_t status_byte = 0;

uint16_t which_extension = EXT_NONE;

//IMU
//In the image in README, the raw accel shows the relevant value when it is facing up. For example in the image in README, the face buttons are facing upwards, and we get a +Z value on the accelerometer.
//standard accelerometer values are ~100 when at normal earth gravity values (aka not moving)
mpu6050_raw_accel_value_t raw_accel;
int16_t accel_offset_4g[3] = {-340, 88, 764}; //add these to values, before multing by scale
float accel_scale_4g = 100 / (8192.0); //multiply values by this to get +-100 at +- 1G to match wiimote range
const int16_t accel_zero_value = 0x0200;

mpu6050_raw_gyro_value_t raw_gyro;

// Register chunks
uint8_t speaker_settings[10]; //A20000 - A20009
uint8_t extension_controller_settings_data[256]; //A40000 - A400FF
uint8_t wii_motion_plus_settings_data[256]; //A60000 - A600FF
uint8_t IR_camera_settings[34]; //B00000 - B00033

// array size is 256
static const uint8_t register_a60000_sample_1[]  = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x78, 0xd9, 0x78, 0x38, 0x77, 0x9d, 0x2f, 0x0c, 0xcf, 0xf0, 0x31, 0xad, 0xc8, 0x0b, 0x5e, 0x39, 
  0x6f, 0x81, 0x7b, 0x89, 0x78, 0x51, 0x33, 0x60, 0xc9, 0xf5, 0x37, 0xc1, 0x2d, 0xe9, 0x15, 0x8d, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xb9, 0x3f, 0x25, 0x93, 0x9d, 0x17, 0xbb, 0x9c, 0x05, 0x9d, 0xc3, 0x38, 0x18, 0x3c, 0xba, 0x33, 
  0xba, 0x18, 0xd1, 0x7a, 0xbc, 0x03, 0xd3, 0x55, 0x32, 0xec, 0x81, 0x38, 0x7d, 0xa6, 0x77, 0xa8, 
  0x4c, 0xe6, 0xc7, 0x11, 0x7c, 0x50, 0x78, 0x80, 0x77, 0x35, 0x08, 0x81, 0xf6, 0x14, 0x4e, 0x67, 
  0xd4, 0xb5, 0xcb, 0xde, 0x6a, 0x54, 0x5f, 0x66, 0x3c, 0xc4, 0x25, 0xfd, 0x33, 0xda, 0x1d, 0x75, 
  0x58, 0x98, 0x15, 0x6d, 0x5e, 0x63, 0x51, 0xee, 0x8f, 0xdd, 0x3a, 0xb2, 0x94, 0xfe, 0x5b, 0x58, 
  0xbf, 0x17, 0x91, 0x78, 0x7f, 0x84, 0xb4, 0x9b, 0xb0, 0xf9, 0x75, 0xc2, 0x2e, 0x7f, 0x1f, 0xed, 
  0xe5, 0x6b, 0x02, 0xf4, 0xf2, 0x7d, 0x74, 0x17, 0x3d, 0x23, 0x35, 0x5c, 0xe0, 0x72, 0x22, 0x6e, 
  0x3b, 0xa7, 0x7b, 0x65, 0x6c, 0x3c, 0x72, 0x7e, 0x5b, 0xae, 0xe7, 0x09, 0x09, 0xf0, 0x01, 0x00, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x55, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0xff, 0xff, 0x00, 0x00, 0xa6, 0x20, 0x00, 0x05
};

void init_register_chunks(){
//	uint8_t wii_motion_plus_identifier[6] = {0x01,0x00,0xa6,0x20,0x00,0x05};
//	memcpy(wii_motion_plus_settings_data + 0xFA ,wii_motion_plus_identifier, 6);
	memcpy(wii_motion_plus_settings_data, register_a60000_sample_1, 256);
}

void init_GPIO(){
	// Set button GPIO directions to input
	gpio_set_direction(BUTTON_PIN_A, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_B, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_ONE, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_TWO, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_PLUS, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_MINUS, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_HOME, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_UP, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_DOWN, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_LEFT, GPIO_MODE_INPUT);
	gpio_set_direction(BUTTON_PIN_RIGHT, GPIO_MODE_INPUT);
	
	// Set button GPIO pull-up enable
	gpio_pullup_en(BUTTON_PIN_A);
	gpio_pullup_en(BUTTON_PIN_B);
	gpio_pullup_en(BUTTON_PIN_ONE);
	gpio_pullup_en(BUTTON_PIN_TWO);
	gpio_pullup_en(BUTTON_PIN_PLUS);
	gpio_pullup_en(BUTTON_PIN_MINUS);
	gpio_pullup_en(BUTTON_PIN_HOME);
	gpio_pullup_en(BUTTON_PIN_UP);
	gpio_pullup_en(BUTTON_PIN_DOWN);
	gpio_pullup_en(BUTTON_PIN_LEFT);
	gpio_pullup_en(BUTTON_PIN_RIGHT);
}

//first byte
static const uint8_t BUTTONS_SHIFT_DPAD_LEFT = 0;
static const uint8_t BUTTONS_SHIFT_DPAD_RIGHT = 1;
static const uint8_t BUTTONS_SHIFT_DPAD_DOWN = 2;
static const uint8_t BUTTONS_SHIFT_DPAD_UP = 3;
static const uint8_t BUTTONS_SHIFT_PLUS = 4;

//second byte
static const uint8_t BUTTONS_SHIFT_TWO = 0;
static const uint8_t BUTTONS_SHIFT_ONE = 1;
static const uint8_t BUTTONS_SHIFT_B = 2;
static const uint8_t BUTTONS_SHIFT_A = 3;
static const uint8_t BUTTONS_SHIFT_MINUS = 4;

static const uint8_t BUTTONS_SHIFT_HOME = 7;

// send the buttons, change in x, and change in y

void load_buttons_buffer(uint8_t* destination)
{
	static uint8_t buttons_buffer[2];
	memset(buttons_buffer, 0, 2);
	
	buttons_buffer[0] |= (!gpio_get_level(BUTTON_PIN_UP) << BUTTONS_SHIFT_DPAD_UP);
	buttons_buffer[0] |= (!gpio_get_level(BUTTON_PIN_DOWN) << BUTTONS_SHIFT_DPAD_DOWN);
	buttons_buffer[0] |= (!gpio_get_level(BUTTON_PIN_LEFT) << BUTTONS_SHIFT_DPAD_LEFT);
	buttons_buffer[0] |= (!gpio_get_level(BUTTON_PIN_RIGHT) << BUTTONS_SHIFT_DPAD_RIGHT);
	buttons_buffer[0] |= (!gpio_get_level(BUTTON_PIN_PLUS) << BUTTONS_SHIFT_PLUS);

	
	buttons_buffer[1] |= (!gpio_get_level(BUTTON_PIN_A) << BUTTONS_SHIFT_A);
	buttons_buffer[1] |= (!gpio_get_level(BUTTON_PIN_B) << BUTTONS_SHIFT_B);
	buttons_buffer[1] |= (!gpio_get_level(BUTTON_PIN_ONE) << BUTTONS_SHIFT_ONE);
	buttons_buffer[1] |= (!gpio_get_level(BUTTON_PIN_TWO) << BUTTONS_SHIFT_TWO);
	buttons_buffer[1] |= (!gpio_get_level(BUTTON_PIN_MINUS) << BUTTONS_SHIFT_MINUS);
	buttons_buffer[1] |= (!gpio_get_level(BUTTON_PIN_HOME) << BUTTONS_SHIFT_HOME);
	
//	ESP_LOGI(TAG, "[%c%c%c%c%c%c%c%c%c%c%c]",
//	         (gpio_get_level(BUTTON_PIN_A) ? ' ' : 'A'),
//	         (gpio_get_level(BUTTON_PIN_B) ? ' ' : 'B'),
//	         (gpio_get_level(BUTTON_PIN_ONE) ? ' ' : '1'),
//	         (gpio_get_level(BUTTON_PIN_TWO) ? ' ' : '2'),
//	         (gpio_get_level(BUTTON_PIN_PLUS) ? ' ' : '+'),
//	         (gpio_get_level(BUTTON_PIN_MINUS) ? '-' : ' '),
//	         (gpio_get_level(BUTTON_PIN_HOME) ? ' ' : 'H'),
//	         (gpio_get_level(BUTTON_PIN_UP) ? ' ' : '^'),
//	         (gpio_get_level(BUTTON_PIN_DOWN) ? ' ' : 'v'),
//	         (gpio_get_level(BUTTON_PIN_LEFT) ? '<' : ' '),
//	         (gpio_get_level(BUTTON_PIN_RIGHT) ? ' ' : '>'));

	if(destination != nullptr){
		memcpy( destination, buttons_buffer, 2);
	}else{
		ESP_LOGE("LOAD_BUTTONS_BUFFER", "NO DESTINATION");
	}
}

int16_t accelerometer_raw_to_10bit(int16_t raw_accel, int16_t offset){
	int16_t aligned = raw_accel + offset;
	float scaled = aligned * accel_scale_4g;
	int16_t scaled_int = (int16_t)(scaled);
	int16_t plus_zero = scaled_int + accel_zero_value;
//	ESP_LOGI("MPU MATH", "(%d+%d)[%d] %f[%d] 0x%04x + 0x200 = 0x%04x", 
//		raw_accel, 
//		offset, 
//		aligned, 
//		scaled, 
//		scaled_int, 
//		scaled_int,
//		plus_zero);
	return plus_zero;
}

//send the start of the buffer because accelerometer data also places bits into the buttons section
void read_from_accelerometer(int16_t* processed_10bit_accel_x, int16_t* processed_10bit_accel_y, int16_t* processed_10bit_accel_z){
    esp_err_t ret = mpu6050_get_raw_accel(mpu6050_handle, &raw_accel);
	if (ret != ESP_OK) {
	    ESP_LOGE("MPU6050", "Read failed");
	    return;
	}
	*processed_10bit_accel_x = accelerometer_raw_to_10bit(raw_accel.raw_accel_x,accel_offset_4g[0]);
	*processed_10bit_accel_y = accelerometer_raw_to_10bit(raw_accel.raw_accel_y,accel_offset_4g[1]);
	*processed_10bit_accel_z = accelerometer_raw_to_10bit(raw_accel.raw_accel_z,accel_offset_4g[2]);

//	ESP_LOGI("MPU MATH", "(%d+%d)[%d] %f[%d] 0x%04x", 
//		raw_accel.raw_accel_x, 
//		accel_offset_4g[0], 
//		(raw_accel.raw_accel_x + accel_offset_4g[0]), 
//		((raw_accel.raw_accel_x + accel_offset_4g[0]) * accel_scale_4g), 
//		(int16_t)((raw_accel.raw_accel_x + accel_offset_4g[0]) * accel_scale_4g), 
//		(int16_t)((raw_accel.raw_accel_z + accel_offset_4g[0]) * accel_scale_4g));

}

//send the start of the buffer because accelerometer data also places bits into the buttons section
void load_accelerometer_buffer(uint8_t* destination, uint16_t processed_10bit_accel_x, uint16_t processed_10bit_accel_y, uint16_t processed_10bit_accel_z){
	
	uint8_t accel_x_byte = ((processed_10bit_accel_x & 0x03FC) >> 2);
	uint8_t accel_x_lower_two_bits = (processed_10bit_accel_x & 0x03) << 5; //shift from bits 01 to bits 56 to align with where it goes in the button matrix
	
	uint8_t accel_y_byte = ((processed_10bit_accel_y & 0x03FC) >> 2);
	uint8_t accel_y_lower_two_bits = (processed_10bit_accel_y & 0x02) << 4; //shift from bit 1 to bits 5 to align with where it goes in the button matrix
	
	uint8_t accel_z_byte = ((processed_10bit_accel_z & 0x03FC) >> 2);
	uint8_t accel_z_lower_two_bits = (processed_10bit_accel_z & 0x02) << 5; //shift from bit 1 to bits 6 to align with where it goes in the button matrix
	
//	ESP_LOGI("MPU6050", "X: %d [0x%04x] Y: %d [0x%04x] Z: %d [0x%04x]", 
//		raw_accel.raw_accel_x, processed_10bit_accel_x, raw_accel.raw_accel_y, processed_10bit_accel_y, raw_accel.raw_accel_z, processed_10bit_accel_z);
	if(destination != nullptr){
		memcpy(destination + 2, &accel_x_byte, 1);
		memcpy(destination + 3, &accel_y_byte, 1);
		memcpy(destination + 4, &accel_z_byte, 1);
		destination[0] |= accel_x_lower_two_bits;
		destination[1] |= (accel_y_lower_two_bits | accel_z_lower_two_bits);
	}else{
		ESP_LOGE("LOAD_ACCELEROMETER_BUFFER", "NO DESTINATION");
	}
}

void load_wii_motion_plus_buffer(uint8_t* destination){
	//fast mode reaches a peak of 2000 degrees per second? and slow mode is potentially 440?
	esp_err_t ret = mpu6050_get_raw_gyro(mpu6050_handle, &raw_gyro);
	if (ret != ESP_OK) {
	    ESP_LOGE("MPU6050", "Read failed");
	    return;
	}
	
	ESP_LOGI("MPU6050 RAW", "X: %d [0x%04x] Y: %d [0x%04x] Z: %d [0x%04x]", 
		raw_gyro.raw_gyro_x, raw_gyro.raw_gyro_x, raw_gyro.raw_gyro_y, raw_gyro.raw_gyro_y, raw_gyro.raw_gyro_z, raw_gyro.raw_gyro_z);
	
	float gyro_yaw_dps = raw_gyro.raw_gyro_x / 16.4;
	float gyro_roll_dps = raw_gyro.raw_gyro_y / 16.4;
	float gyro_pitch_dps = raw_gyro.raw_gyro_z / 16.4;
	
	int16_t gyro_yaw_14b   = raw_gyro.raw_gyro_x >> 2;
	int16_t gyro_roll_14b  = raw_gyro.raw_gyro_y >> 2;
	int16_t gyro_pitch_14b = raw_gyro.raw_gyro_z >> 2;
	
	bool slow_yaw = false, slow_roll = false, slow_pitch = false;
	
	//TODO: OPTIMIZE THIS SO IT DOESNT REQUIRE ME TO CALC THE DPS FIRST AND JUST BAKES THE SENSITIVITY INTO THE IF STATEMENT
	if(gyro_yaw_dps < 440){
	    gyro_yaw_14b = (int16_t)((gyro_yaw_14b * 440.0 / 2000) + 8191);
	    slow_yaw = true;
	}

	if(gyro_roll_dps < 440){
	    gyro_roll_14b = (int16_t)((gyro_roll_14b * 440.0 / 2000) + 8191);
	    slow_roll = true;
	}

	if(gyro_pitch_dps < 440){
	    gyro_pitch_14b = (int16_t)((gyro_pitch_14b * 440.0 / 2000) + 8191);
	    slow_pitch = true;
	}
	
	ESP_LOGI("MPU6050 PRC", "Y: %d [0x%04x] R: %d [0x%04x] P: %d [0x%04x]", 
		gyro_yaw_14b, gyro_yaw_14b, gyro_roll_14b, gyro_roll_14b, gyro_pitch_14b, gyro_pitch_14b);
	
	uint8_t yaw7_0 = gyro_yaw_14b & 0xFF;
	uint8_t yaw13_8 = (gyro_yaw_14b & 0x3F00) >> 6;
	yaw13_8 |= ((slow_yaw << 1) | slow_pitch); //THIS ZERO IS FOR EXTENSION CONNECTED, TODO: IMPLEMENT

	uint8_t roll7_0 = gyro_roll_14b & 0xFF;
	uint8_t roll13_8 = (gyro_roll_14b & 0x3F00) >> 6;
	roll13_8 |= (slow_roll << 1 | 0); //THIS ZERO IS FOR EXTENSION CONNECTED, TODO: IMPLEMENT
	
	uint8_t pitch7_0 = gyro_pitch_14b & 0xFF;
	uint8_t pitch13_8 = (gyro_pitch_14b & 0x3F00) >> 6;
	roll13_8 |= 0x02;

	
	if(destination != nullptr){
		memcpy(destination, &yaw7_0, 1);
		memcpy(destination+1, &roll7_0, 1);
		memcpy(destination+2, &pitch7_0, 1);
		memcpy(destination+3, &yaw13_8, 1);
		memcpy(destination+4, &roll13_8, 1);
		memcpy(destination+5, &pitch13_8, 1);
	}else{
		ESP_LOGE("LOAD_WII_MOTION_PLUS_BUFFER", "NO DESTINATION");
	}
}

uint8_t input_report[21] = {0};

//20 BB BB LF 00 00 VV
void mote_input_data_status()
{
//	memcpy(input_report,buttons,2);
	load_buttons_buffer(input_report);
	input_report[2] = status_byte;
	input_report[3] = 0;
	input_report[4] = 0;
	input_report[5] = 0xEF; //TODO: Battery value, replace with actual battery value
	esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x20, input_report, 6);
}

//21 BB BB SE AA AA DD DD DD DD DD DD DD DD DD DD DD DD DD DD DD DD
void mote_input_data_read(uint8_t size, uint8_t error, uint16_t address_low_16, uint8_t* buffer)
{
	load_buttons_buffer(input_report);
	input_report[2] = (((size-1) & 0xF) << 4) | (error & 0xF);
	printf("%d %02x %02x\n", size - 1, (size-1) & 0xF, (((size-1) & 0xF) << 4));
	//E (low nybble of SE) is the error flag. Known error values are 0 for no error, 7 when attempting to read from a write-only register or an expansion that is not connected, and 8 when attempting to read from nonexistant memory addresses. 
	input_report[3] = (address_low_16 & 0xFF00) >> 8;
	input_report[4] = address_low_16 & 0x00FF;
//	memcpy(input_report + 3, &address_low_16, 2);
	memset(input_report + 5, 0, 16);
	memcpy(input_report + 5, buffer + address_low_16, size);
	esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x21, input_report, 21);
	
	ESP_LOGI( TAGSEND, "Responding to read");
	ESP_LOG_BUFFER_HEX(TAGSEND, buffer + address_low_16, size);
}

//22 BB BB RR EE
void mote_input_data_acknowledge(uint8_t report_number, uint8_t error)
{
	load_buttons_buffer(input_report);
	input_report[2] = report_number;
	input_report[3] = error;
	esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x22, input_report, 4);
}

void mote_input_data_core()
{
    static uint8_t old_buttons[2] = {0};
	static int16_t old_accel[3] = {0};
	uint8_t buttons[2] = {0};
	load_buttons_buffer(buttons);
	
	int16_t accel_10b_x, accel_10b_y, accel_10b_z;
	read_from_accelerometer(&accel_10b_x, &accel_10b_y, &accel_10b_z);
	
	bool send_packet = 
		continuousReporting || 
		(reportingMode != 0x3d && (old_buttons[0] != buttons[0] || old_buttons[1] != buttons[1])) || 
		((reportingMode == 0x31 || reportingMode == 0x33 || reportingMode == 0x35 || reportingMode == 0x37 || reportingMode == 0x3e) && (old_accel[0] != accel_10b_x || old_accel[1] != accel_10b_y || old_accel[1] != accel_10b_z));
	if(send_packet){
		switch(reportingMode){
			case 0x30:
				//memcpy(input_report,buttons,2);
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x30, buttons, 2);
				ESP_LOG_BUFFER_HEX("SEND 0x30", buttons, 2);
			    break;
			case 0x31:
				memcpy(input_report,buttons,2);
				load_accelerometer_buffer(input_report, accel_10b_x, accel_10b_y, accel_10b_z);
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x31, input_report, 5);
				ESP_LOG_BUFFER_HEX("SEND 0x31", input_report, 5);
			    break;
			case 0x32:
				memcpy(input_report,buttons,2);
				if(which_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report+2);
					memset(input_report+8,0,2);
				}else{
					memset(input_report+2,0xFF,8); //replace with 8 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x32, input_report, 10);
				ESP_LOG_BUFFER_HEX("SEND 0x32", input_report, 10);
				
			    break;
			case 0x33:
				memcpy(input_report,buttons,2);
				load_accelerometer_buffer(input_report, accel_10b_x, accel_10b_y, accel_10b_z);
				memset(input_report+5,0xFF,12); //replace with 12 IR bytes
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x33, input_report, 17);
				ESP_LOG_BUFFER_HEX("SEND 0x33", input_report, 17);
			    break;
			case 0x34:
				memcpy(input_report,buttons,2);
				if(which_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report+2);
					memset(input_report+8,0,13);
				}else{
					memset(input_report+2,0xFF,19); //replace with 19 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x34, input_report, 21);
				ESP_LOG_BUFFER_HEX("SEND 0x34", input_report, 21);
			    break;
			case 0x35:
				memcpy(input_report,buttons,2);
				load_accelerometer_buffer(input_report, accel_10b_x, accel_10b_y, accel_10b_z);
				if(which_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report+5);
					memset(input_report+11,0,10);
				}else{
					memset(input_report+5,0xFF,16); //replace with 16 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x35, input_report, 21);
				ESP_LOG_BUFFER_HEX("SEND 0x35", input_report, 21);
			    break;
			case 0x36:
				memcpy(input_report,buttons,2);
				memset(input_report+2,0xFF,10); //replace with 10 IR bytes
				if(which_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report+12);
					memset(input_report+18,0,3);
				}else{
					memset(input_report+12,0xFF,9); //replace with 9 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x36, input_report, 21);
				ESP_LOG_BUFFER_HEX("SEND 0x36", input_report, 21);
			    break;
			case 0x37:
				memcpy(input_report,buttons,2);
				load_accelerometer_buffer(input_report, accel_10b_x, accel_10b_y, accel_10b_z);
				memset(input_report+5,0xFF,10); //replace with 10 IR bytes
				if(which_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report+15);
				}else{
					memset(input_report+15,0xFF,6); //replace with 6 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x37, input_report, 21);
				ESP_LOG_BUFFER_HEX("SEND 0x37", input_report, 21);
			    break;
			case 0x3d:
				if(which_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report);
					memset(input_report+6,0,15); 
				}else{
					memset(input_report,0xFF,21); //replace with 21 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x3d, input_report, 21);
				ESP_LOG_BUFFER_HEX("SEND 0x3d", input_report, 21);
			    break;
			case 0x3e: //same as 3f, forced to 3e during output report handling
				//TODO: SET UP LATER ITS A WHOLE THING
				//esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x3e, buttons, 21);
				ESP_LOG_BUFFER_HEX("0x3e NOT SUPPORTED", input_report, 21);
			    break;
			default:
			    break;
		}
		
	}
	memcpy(old_buttons, buttons, 2);
	old_accel[0] = accel_10b_x;
	old_accel[1] = accel_10b_y;
	old_accel[1] = accel_10b_z;
}

void mote_hid_main_task(void *pvParameters)
{
    static const char* help_string = "########################################################################\n"\
    "ESPmote:\n"\
    "q -- send pressed buttons\n"\
    "h -- show the help\n"\
    "########################################################################\n";
    printf("%s\n", help_string);
    char c = 0;
    while (1) {
		c = fgetc(stdin);
		
		mote_input_data_core();
		
		switch (c) {
		case 'q':
		 	
		    break;
		case 'h':
		    printf("%s\n", help_string);
		    break;
		default:
		    break;
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
		
		uint32_t offset;
		uint16_t offset_16;
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
					status_byte &= 0x0F;
					status_byte |= (param->output.data[0] & 0xF0);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}
				
		        break;
		        
		    case O_DATA_REPORTING_MODE:
		        // 2 bytes - TT MM (TT bit2 = continuous, MM = mode 0x30-0x3f)
				continuousReporting = (param->output.data[0] & 0x04);
				reportingMode = param->output.data[1];
				if(reportingMode == 0x3f){
					reportingMode = 0x3e; //for simplicity, lock both 3f and 3e behinnd 3e as they are the same.
				}
				ESP_LOGI( TAGW, "Data Reporting: TT[%2x] MM[%x]", continuousReporting, reportingMode);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}
				
		        break;
		        
		    case O_IR_CAMERA_ENABLE: //Based on my knowledge this enables the IR Pixel Clock. 
				// 1 byte - bit 2 = ON/OFF
				//TODO: I am currently treating this as the main IR Camera Toggle (for status byte reasons), but this is not to be the case in final
						        
				if(param->output.data[0] & 0x04){
					status_byte |= 0x04;
				}
				
				ESP_LOGI( TAGW, "Written %2x to IR Camera 1", param->output.data[0]);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}
				
		        break;
		        
		    case O_SPEAKER_ENABLE:
		        // 1 byte - bit 2 = ON/OFF
				if(param->output.data[0] & 0x04){
					status_byte |= 0x04;
				}
				ESP_LOGI( TAGW, "Written %2x to Speaker Enable", param->output.data[0]);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}

		        break;
		        
		    case O_STATUS_INFO_REQUEST:
		        // 1 byte - request status report
				ESP_LOGI( TAGW, "Requesting %02x to Status Info", param->output.data[0]);
				
				mote_input_data_status();
				
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}

		        break;
		        
		    case O_WRITE_MEMORY_REGISTERS: //TODO: make sure doesnt buffer overflow
		        // 21 bytes - write to memory/registers
				offset = (param->output.data[1] << 16) | (param->output.data[2] << 8) | (param->output.data[3]);
				offset_16 = (param->output.data[2] << 8) | (param->output.data[3]);
				size = param->output.data[4];
				
				ack_error_code_t return_ack = ACK_SUCCESS;
				
				if((param->output.data[0] & 0x04)){
					if(param->output.data[1] == 0xA2){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to speaker settings at %6x [%04x]", size, offset, offset_16);
						memcpy(speaker_settings + offset_16, param->output.data + 5, size);
					}else if(param->output.data[1] == 0xA4){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to extension controller settings and data at %6x [%04x]", size, offset, offset_16);
						if(which_extension != EXT_NONE){
							memcpy(extension_controller_settings_data + offset_16, param->output.data + 5, size);
						}else{
							return_ack = ACK_INACTIVE_EXTENSION;
						}
					}else if(param->output.data[1] == 0xA6){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to wii motion plus settings and data at %6x [%04x]", size, offset, offset_16);
						memcpy(wii_motion_plus_settings_data + offset_16, param->output.data + 5, size);
						if(offset_16 == 0x00FE && size == 1 && param->output.data[5] == 0x04){ //changing active extension to wii motion plus
							which_extension = EXT_WII_MOTION_PLUS_ACTIVE;
							status_byte |= 0x02;
//							memset(extension_controller_settings_data + 0x00FA, 0, 2);
//							memcpy(extension_controller_settings_data + 0x00FC, &EXTENSION_A4_TAG, 2);
//							memcpy(extension_controller_settings_data + 0x00FE, &EXT_WII_MOTION_PLUS_ACTIVE, 2);
							memcpy(extension_controller_settings_data + 0x00FA, wii_motion_plus_settings_data + 0x00FA, 6);
							memcpy(extension_controller_settings_data + 0x00FC, &EXTENSION_A4_TAG, 2);
						}
					}else if(param->output.data[1] == 0xB0){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to IR camera settings at %6x [%04x]", size, offset, offset_16);
						memcpy(IR_camera_settings + offset_16, param->output.data + 5, size);
					}else {
						ESP_LOGI( TAGW, "Attempting to write %d bytes to control registers at %6x [%04x]", size, offset, offset_16);
					}
				}else{
					ESP_LOGI( TAGW, "Attempting to write %d bytes to EEPROM Memory at %6x", size, offset);
				}
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data + 5, size); 
					
				//apparently all writes request an ack automatically
				mote_input_data_acknowledge(param->feature.report_id, return_ack);

				break;
		        
		    case O_READ_MEMORY_REGISTERS: //TODO: SET UP READS FOR GREATER THAN 16 BYTES TOTAL 
				//TODO: MAKE SURE NO MEM OVERFLOW
		        // 6 bytes - read from memory/registers
				offset = (param->output.data[1] << 16) | (param->output.data[2] << 8) | (param->output.data[3]);
				offset_16 = (param->output.data[2] << 8) | (param->output.data[3]);
//				memset(&size, 0, 2);
				size = (param->output.data[4] << 8) | (param->output.data[5]);
				if((param->output.data[0] & 0x04)){
					if(param->output.data[1] == 0xA2){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from speaker settings at %6x [%04x]", size, offset, offset_16);
						mote_input_data_read(size, 0, offset_16, speaker_settings);
					}else if(param->output.data[1] == 0xA4){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from extension controller settings and data at %6x [%04x]", size, offset, offset_16);
						if(which_extension != EXT_NONE){
							mote_input_data_read(size, 0, offset_16, extension_controller_settings_data);
						}else{
							uint8_t zero_buffer[16] = {0};
							mote_input_data_read(16, READ_WRITE_ONLY, offset_16, zero_buffer - offset_16); //TODO: fix this, this is atrocious
						}
					}else if(param->output.data[1] == 0xA6){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from wii motion plus settings and data at %6x [%04x]", size, offset, offset_16);
						mote_input_data_read(size, 0, offset_16, wii_motion_plus_settings_data);
						//ESP_LOG_BUFFER_HEX("WII_MP DUMP", wii_motion_plus_settings_data, 256);
					}else if(param->output.data[1] == 0xB0){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from IR camera settings at %6x [%04x]", size, offset, offset_16);
						mote_input_data_read(size, 0, offset_16, IR_camera_settings);

					}else {
						ESP_LOGI( TAGW, "Attempting to read %d bytes from control registers at %6x", size, offset);
					}
				}else{
					ESP_LOGI( TAGW, "Attempting to read %d bytes from EEPROM Memory at %6x", size, offset);
				}
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}

		        break;
		        
		    case O_SPEAKER_DATA:
		        // 21 bytes - audio data for speaker
				ESP_LOGI( TAGW, "%d bytes of Speaker Data", param->output.data[0]);
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data + 1, param->output.data[0]); //TODO: make sure doesnt buffer overflow
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}

		        break;
		        
		    case O_SPEAKER_MUTE:
		        // 1 byte - bit 2 = mute when set
				ESP_LOGI( TAGW, "Written %2x to Speaker Mute", param->output.data[0]);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}

		        break;
		        
		    case O_IR_CAMERA_ENABLE_2: //Based on my knowledge this enables the IR logic
		        // 1 byte - bit 2 = ON/OFF (alternate)
				ESP_LOGI( TAGW, "Written %2x to IR Camera 2", param->output.data[0]);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}

		        break;
		        
		    default:
		        // Unknown output report ID
				ESP_LOGW(TAGW, "UNKNOWN REPORT ID[%u]: %8s ID: 0x%2x, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data, param->output.length);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_ERROR);
				}

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

	//Setting up Correct MAC Address TODO: REPLACE WITH A CONSISTENT RANDOM LOOKUP OF THE MAC TABLE
	uint8_t baseMac[6];
	esp_base_mac_addr_get(baseMac);
	baseMac[0] = 0xCC; //pinkmote's first 3
	baseMac[1] = 0x9E;
	baseMac[2] = 0x00;
	esp_base_mac_addr_set(baseMac);
	
	init_GPIO();
	init_register_chunks();
	
	i2c_master_bus_config_t bus_config = {
	    .i2c_port = I2C_MODE_MASTER,               // I2C port number
	    .sda_io_num = I2C_MASTER_SDA_IO,         // GPIO number for I2C sda signal
	    .scl_io_num = I2C_MASTER_SCL_IO,         // GPIO number for I2C scl signal
	    .clk_source = I2C_CLK_SRC_DEFAULT,  // I2C clock source, just use the default
	    .glitch_ignore_cnt = 7,             // glitch filter, again, just use the default
	    .flags = {
	        .enable_internal_pullup = true, // enable internal pullup resistors (oled screen does not have one)
	    },
	};
	
	i2c_master_bus_handle_t i2c_bus_handle = NULL;
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

	// Initialize MPU6050
	mpu6050_info_t info = {
		.address = 0x68,
		.clock_speed = I2C_MASTER_FREQ_HZ
	};
	ret = mpu6050_create(i2c_bus_handle, info, &mpu6050_handle);
	if (ret != ESP_OK) {
	    ESP_LOGE("MPU6050", "Creation failed");
	    return;
	}
	
	mpu6050_config_t config = {
		.accel_fs = ACCEL_FS_4G,
		.gyro_fs = GYRO_FS_2000DPS
	};
	ret = mpu6050_config(mpu6050_handle, config);
	if (ret != ESP_OK) {
	    ESP_LOGE("MPU6050", "Config failed");
	    return;
	}
	ret = mpu6050_wake_up(mpu6050_handle);
	if (ret != ESP_OK) {
	    ESP_LOGE("MPU6050", "Wakeup failed");
	    return;
	}
	
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
