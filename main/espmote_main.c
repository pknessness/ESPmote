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

#include "esp_rom_crc.h"
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
#include "portmacro.h"
#if CONFIG_BT_SDP_COMMON_ENABLED
#include "esp_sdp_api.h"
#endif /* CONFIG_BT_SDP_COMMON_ENABLED */

#include "esp_hidd.h"
#include "esp_hid_gap.h"

//Added by pknessness
#include "esp_random.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "driver/uart.h"

#include "lsm6ds3.h"
#include "pixart_ir.h"

#include "esp_adc/adc_continuous.h"

static const char *TAG = "HID_DEVICE";
static const char *TAGSEND = "WIIMOTE_OUTPUT";
static const char *TAGW = "WII_OUTPUT";

#define ADC_READ_LEN                    256

#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_0
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)

typedef struct //came with bt example
{
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;

// From https://wiibrew.org/wiki/Wiimote
// HID input and output report IDs
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

//From https://wiibrew.org/wiki/Wiimote#0x22:_Acknowledge_output_report,_return_function_result
//Error codes for acknowledge output report
typedef enum { 
    // Output Reports (O_) - Wii to Wii Remote
    ACK_SUCCESS                         = 0x00,
    ACK_ERROR                           = 0x03,
    ACK_UNKNOWN1                        = 0x04,
    ACK_UNKNOWN2                        = 0x05,
	ACK_INACTIVE_EXTENSION              = 0x07, //for when writing to an unconnected extension like deactive motion plus
    ACK_UNKNOWN3                        = 0x08,
} ack_error_code_t;

//From https://wiibrew.org/wiki/Wiimote#0x21:_Read_Memory_Data
//Error codes for read memory data response
typedef enum {
    // Output Reports (O_) - Wii to Wii Remote
    READ_SUCCESS                        = 0x00,
    READ_WRITE_ONLY                     = 0x07,
    READ_NONEXISTENT                    = 0x08,
} read_error_code_t;

//From https://wiibrew.org/wiki/Wiimote#Data_Formats
//IR camera data formats
typedef enum {
    // Output Reports (O_) - Wii to Wii Remote
    IR_BASIC                        = 0x01,
    IR_EXTENDED                     = 0x03,
    IR_FULL                   		= 0x05,
} ir_modes_t;

// EXTENSION IDS
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

// LED GPIO
#define LED1 GPIO_NUM_12
#define LED2 GPIO_NUM_13
#define LED3 GPIO_NUM_14
#define LED4 GPIO_NUM_15

// Button Matrix GPIO
#define BUTTON_I1 GPIO_NUM_19
#define BUTTON_I2 GPIO_NUM_18
#define BUTTON_I3 GPIO_NUM_27

#define BUTTON_O1 GPIO_NUM_36
#define BUTTON_O2 GPIO_NUM_39
#define BUTTON_O3 GPIO_NUM_34
#define BUTTON_O4 GPIO_NUM_35

#define BUTTON_A GPIO_NUM_33

#define BUTTON_MATRIX_TIME_ON 20

#define SPEAKER_GPIO GPIO_NUM_25
#define RUMBLE_GPIO GPIO_NUM_23
#define IR_CLK_GPIO GPIO_NUM_26
#define IR_ENABLE_GPIO GPIO_NUM_17
#define EXT_SENSE_GPIO GPIO_NUM_4

TaskHandle_t adc_task_hdl;

//My own IDS for buttons, relevant for button_array and button_array_adc
typedef enum {
    // Output Reports (O_) - Wii to Wii Remote
	BTN_A,
	BTN_B,
	BTN_ONE,
	BTN_TWO,
	BTN_PLUS,
	BTN_MINUS,
	BTN_HOME,
	BTN_UP,
	BTN_DOWN,
	BTN_LEFT,
	BTN_RIGHT,
	BTN_SYNC,
	BTN_POWER
} button_ids;

//boolean value of each button, indexed by button_ids enum
bool button_array[13] = {0};
//adc value of each button, indexed by button_ids enum
int32_t button_array_adc[13] = {0};

//ADC thresholds for buttons to be considered active
int32_t button_thresholds[13] = {100,100,100,100,100,100,100,100,100,100,100,100,100};

//Array of which channels the ADC is looking at during continuous mode
static adc_channel_t button_adc_channels[5] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_5};
//O1,2,3,4 = ADC1_ 0, 3, 6, 7
//A button = ADC1_5
//TODO: ADD BATTERY READ WHICH IS IO32

//ADC channel most recent read
//Some slots are empty but this is easier to index
int32_t adc1_channels[10] = {0};

//which input (1, 2, or 3) is the button matrix currently holding high.
int8_t button_matrix_input = 0;

//IMU (I2C) config bits
#define I2C_MASTER_SCL_IO    22 // SCL pin
#define I2C_MASTER_SDA_IO    21 // SDA pin
#define I2C_MASTER_FREQ_HZ   400000
#define I2C_MASTER_NUM       I2C_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0
lsm6ds3_handle_t imu_handle;
pixart_ir_handle_t ir_handle;


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

// Wiimote Operational Variables
bool continuousReporting = false;

uint8_t reportingMode = 0x30;

bool rumbling = false;
bool speaker_enable = false;

//bit   mask    meaning
//0 	0x01 	Battery is nearly empty
//1 	0x02 	An Extension Controller is connected
//2 	0x04 	Speaker enabled
//3 	0x08 	IR camera enabled
//4 	0x10 	LED 1
//5 	0x20 	LED 2
//6 	0x40 	LED 3
//7 	0x80 	LED 4 
uint8_t status_byte = 0; 

uint16_t active_extension = EXT_NONE;
uint16_t plugged_in_extension = EXT_NONE;

//IR CAMERA
uint8_t ir_raw_buffer[16];

//IMU
//In the image in README, the raw accel shows the relevant value when it is facing up. For example in the image in README, the face buttons are facing upwards, and we get a +Z value on the accelerometer.
//Standard accelerometer values are ~100 when at normal earth gravity values (aka not moving)
//When sending over bt, we add these values to 0x200 (512) to get a 10 bit positive number that is 512 +- G 
float accel_mg[3];

float accel_offset_mg[3] = {0, 0, 0}; //add these to values, before multing by scale
const int16_t CALIBRATION_ACCEL_1G_OFFSET = 100;
float accel_scale_mg = CALIBRATION_ACCEL_1G_OFFSET / (1000.0); //multiply values by this to get +-100 at +- 1G to match wiimote range
const int16_t CALIBRATION_ACCEL_ZERO = 0x0200;

float gyro_mdps[3];
float gyro_dps[3];
const uint16_t CALIBRATION_GYRO_ZERO = 0x8000; //CALIBRATION VALUES ARE BAKED INTO THE REGISTER THING, TODO IS TO NOT BAKE THEM IN? 
const uint16_t CALIBRATION_GYRO_SCALE_OFFSET = 0x4400;
const uint16_t CALIBRATION_GYRO_FAST_SCALE_DEGREES = 1200;
const uint16_t CALIBRATION_GYRO_SLOW_SCALE_DEGREES = 270;
const uint16_t VALUE_ZERO = 0x2000;
const uint16_t VALUE_SCALE_OFFSET = 0x1100;

// Register chunks
uint8_t speaker_settings[10]; //A20000 - A20009
uint8_t extension_controller_settings_data[256]; //A40000 - A400FF
//uint8_t wii_motion_plus_settings_data[256]; //A60000 - A600FF
uint8_t IR_camera_settings[52]; //B00000 - B00033

//Fake EEPROM for calibration stuff
uint8_t eeprom_start[48] = {
	0xA1, 0xAA, 0x8B, 0x99, 0xAE, 0x9E, 0x78, 0x30, 0xA7, 0x74, 0xD3,
	0xA1, 0xAA, 0x8B, 0x99, 0xAE, 0x9E, 0x78, 0x30, 0xA7, 0x74, 0xD3,
	0x80, 0x80, 0x80, 0x00, 0x99, 0x99, 0x99, 0x00, 0x40, 0xE0,
	0x80, 0x80, 0x80, 0x00, 0x99, 0x99, 0x99, 0x00, 0x40, 0xE0, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//A60000 - A600FF
uint8_t wii_motion_plus_settings_data[256]  = { //TODO: HAVE CALIBRATION VALUES BETTER MATCH REAL MOTE STUFF
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  
  0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0xc8, 0x01, 0x03, 0xbf, 
  0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x2d, 0x6e, 0x13, 0xf7, 

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
  0x55, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0xff, 0xff, 0x01, 0x00, 0xa6, 0x20, 0x00, 0x05
};

//real mote calibration values
//0x78, 0xd9, 0x78, 0x38, 0x77, 0x9d, 0x2f, 0x0c, 0xcf, 0xf0, 0x31, 0xad, 0xc8, 0x0b, 0x5e, 0x39
//0x6f, 0x81, 0x7b, 0x89, 0x78, 0x51, 0x33, 0x60, 0xc9, 0xf5, 0x37, 0xc1, 0x2d, 0xe9, 0x15, 0x8d
//crc: 0x5e39158d

//taken from dolphin MotionPlus.h
//a60000 register map
// 0x00 21 bytes of controller data
// 0x15 11 bytes of 0xFF (unknown)
// 0x20 32 bytes of calibration
// 0x40 16 bytes of passthrough extension calibration
// 0x50 64 bytes of challenge data
// 0x90 96 bytes of unknown
// 0xF0 1 byte of init trigger
// 0xF1 1 byte of challenge type
// 0xF2 1 byte of calibration trigger
// 0xF3 3 bytes of unknown
// 0xF6 1 byte of passthrough_extension_id_4
// 0xF7 1 byte of challenge_state
// DOLPHIN: Games read this value to know when the data at 0x50 is ready.
// DOLPHIN: Value is 0x02 upon activation. (via a write to 0xfe)
// DOLPHIN: Real M+ changes this value to 0x4, 0x8, 0xc, and finally 0xe.
// DOLPHIN: Games then trigger a 2nd stage via a write to 0xf1.
// DOLPHIN: Real M+ changes this value to 0x14, 0x18, and finally 0x1a.
// DOLPHIN: Note: We don't progress like this. We jump to the final value as soon as possible.
// 0xF8 1 byte of passthrough_extension_id_0
// 0xF9 1 byte of passthrough_extension_id_5
// 0xFA 6 bytes of identifier (for motion plus)

void init_register_chunks(){ //TODO: THIS IS FOR INITIALIZING THE A60000 REGISTERS, CURRENTLY HARDCODED
//	uint8_t wii_motion_plus_identifier[6] = {0x01,0x00,0xa6,0x20,0x00,0x05};
//	memcpy(wii_motion_plus_settings_data + 0xFA ,wii_motion_plus_identifier, 6);

//	memcpy(wii_motion_plus_settings_data, register_a60000_sample_1, 256);
	
//	uint32_t crc_result = esp_rom_crc32_le(0, wii_motion_plus_settings_data + 0x20, 14);
//	crc_result = esp_rom_crc32_le(crc_result, wii_motion_plus_settings_data + 0x30, 14);
//	
//	ESP_LOG_BUFFER_HEX("CRC THINGU", wii_motion_plus_settings_data + 0x20, 0x20);
//
//	uint16_t crc_msb = (crc_result >> 16) & 0xFFFF;
//	uint16_t crc_lsb = (crc_result & 0xFFFF);
//	
//	printf("0x %04x %04x", crc_msb, crc_lsb);
	
//	memcpy(&wii_motion_plus_settings_data + 46, &crc_msb, 2);
}

void init_GPIO(){
	
	//RESET I2C, seems to be important for some reason, i guess the i2c init code doesn't do this?
	gpio_reset_pin(I2C_MASTER_SCL_IO);
	gpio_reset_pin(I2C_MASTER_SDA_IO);
	
	//these are flipped because input to the keyboard array is output on ESP32 GPIO and vice versa
	gpio_reset_pin(BUTTON_I1);
	gpio_reset_pin(BUTTON_I2);
	gpio_reset_pin(BUTTON_I3);
	gpio_set_direction(BUTTON_I1, GPIO_MODE_OUTPUT);
	gpio_set_direction(BUTTON_I2, GPIO_MODE_OUTPUT);
	gpio_set_direction(BUTTON_I3, GPIO_MODE_OUTPUT);
	
	//No GPIO for O1,O2,O3,O4 as they are handled by the ADC, not GPIO.

	//Other inits
	gpio_reset_pin(SPEAKER_GPIO);
	gpio_reset_pin(RUMBLE_GPIO);
	gpio_reset_pin(IR_CLK_GPIO);
	gpio_reset_pin(IR_ENABLE_GPIO);
	gpio_reset_pin(EXT_SENSE_GPIO);
	gpio_set_direction(SPEAKER_GPIO, GPIO_MODE_OUTPUT); //might not be necessary as this is handled by DAC
	gpio_set_direction(RUMBLE_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(IR_CLK_GPIO, GPIO_MODE_OUTPUT); //TODO: replace with relevant output report
	gpio_set_direction(IR_ENABLE_GPIO, GPIO_MODE_OUTPUT); //TODO: replace with relevant output report
	gpio_set_direction(EXT_SENSE_GPIO, GPIO_MODE_INPUT);

	//LED SETUP
	gpio_reset_pin(LED1);
	gpio_reset_pin(LED2);
	gpio_reset_pin(LED3);
	gpio_reset_pin(LED4);
	gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED3, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED4, GPIO_MODE_OUTPUT);
}

//set all four LEDs to the binary representation of a number.
void setLEDBinary(uint8_t bin){
	gpio_set_level(LED4, bin & 0x01);
	gpio_set_level(LED3, bin & 0x02);
	gpio_set_level(LED2, bin & 0x04);
	gpio_set_level(LED1, bin & 0x08);
}

//bit shifts for buttons buffer
//first byte
const uint8_t BUTTONS_SHIFT_DPAD_LEFT = 0;
const uint8_t BUTTONS_SHIFT_DPAD_RIGHT = 1;
const uint8_t BUTTONS_SHIFT_DPAD_DOWN = 2;
const uint8_t BUTTONS_SHIFT_DPAD_UP = 3;
static const uint8_t BUTTONS_SHIFT_PLUS = 4;

//second byte
const uint8_t BUTTONS_SHIFT_TWO = 0;
const uint8_t BUTTONS_SHIFT_ONE = 1;
const uint8_t BUTTONS_SHIFT_B = 2;
const uint8_t BUTTONS_SHIFT_A = 3;
const uint8_t BUTTONS_SHIFT_MINUS = 4;
const uint8_t BUTTONS_SHIFT_HOME = 7;

//Cycle through button matrix inputs
void assign_buttons_adc(){
	gpio_set_level(BUTTON_I1, 1);
	button_matrix_input = 1;
	vTaskDelay(BUTTON_MATRIX_TIME_ON / portTICK_PERIOD_MS);
	gpio_set_level(BUTTON_I1, 0);
	
	gpio_set_level(BUTTON_I2, 1);
	button_matrix_input = 2;
	vTaskDelay(BUTTON_MATRIX_TIME_ON / portTICK_PERIOD_MS);
	gpio_set_level(BUTTON_I2, 0);
	
	gpio_set_level(BUTTON_I3, 1);
	button_matrix_input = 3;
	vTaskDelay(BUTTON_MATRIX_TIME_ON / portTICK_PERIOD_MS);
	button_matrix_input = 0;
	gpio_set_level(BUTTON_I3, 0);

	button_array[BTN_A] = adc1_channels[5] > button_thresholds[BTN_A];
	button_array_adc[BTN_A] =  adc1_channels[5];
}


void load_buttons_buffer(uint8_t* destination)
{
	static uint8_t buttons_buffer[2];
	memset(buttons_buffer, 0, 2);
	
	buttons_buffer[0] |= (button_array[BTN_UP] << BUTTONS_SHIFT_DPAD_UP);
	buttons_buffer[0] |= (button_array[BTN_DOWN] << BUTTONS_SHIFT_DPAD_DOWN);
	buttons_buffer[0] |= (button_array[BTN_LEFT] << BUTTONS_SHIFT_DPAD_LEFT);
	buttons_buffer[0] |= (button_array[BTN_RIGHT] << BUTTONS_SHIFT_DPAD_RIGHT);
	buttons_buffer[0] |= (button_array[BTN_PLUS] << BUTTONS_SHIFT_PLUS);

	
	buttons_buffer[1] |= (button_array[BTN_A] << BUTTONS_SHIFT_A);
	buttons_buffer[1] |= (button_array[BTN_B] << BUTTONS_SHIFT_B);
	buttons_buffer[1] |= (button_array[BTN_ONE] << BUTTONS_SHIFT_ONE);
	buttons_buffer[1] |= (button_array[BTN_TWO] << BUTTONS_SHIFT_TWO);
	buttons_buffer[1] |= (button_array[BTN_MINUS] << BUTTONS_SHIFT_MINUS);
	buttons_buffer[1] |= (button_array[BTN_HOME] << BUTTONS_SHIFT_HOME);
	
//	ESP_LOGI(TAG, "%c%c%c%c%c%c%c%c%c%c%c",
//	    button_array[BTN_A] ? 'A' : ' ',
//	    button_array[BTN_B] ? 'B' : ' ',
//	    button_array[BTN_ONE] ? '1' : ' ',
//	    button_array[BTN_TWO] ? '2' : ' ',
//	    button_array[BTN_PLUS] ? '+' : ' ',
//	    button_array[BTN_MINUS] ? '-' : ' ',
//	    button_array[BTN_HOME] ? 'H' : ' ',
//	    button_array[BTN_UP] ? '^' : ' ',
//	    button_array[BTN_DOWN] ? 'v' : ' ',
//	    button_array[BTN_LEFT] ? '<' : ' ',
//	    button_array[BTN_RIGHT] ? '>' : ' '
//	);  

	if(destination != nullptr){
		memcpy( destination, buttons_buffer, 2);
	}else{
		ESP_LOGE("LOAD_BUTTONS_BUFFER", "NO DESTINATION");
	}
}

//TODO: gain a better understanding of what this does and how this works, src: https://randomnerdtutorials.com/esp-idf-esp32-gpio-analog-adc/
static TaskHandle_t s_task_handle;
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    // Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = ADC_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
        .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = ADC_ATTEN_DB_12;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = ADC_BITWIDTH_12;

//        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
//        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
//        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

void continuous_adc(void *pvParameters){ //TODO: MUTEX THIS SAFELY
	esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[ADC_READ_LEN] = {0};
    memset(result, 0xcc, ADC_READ_LEN);

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(button_adc_channels, sizeof(button_adc_channels) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

	ESP_LOGI("ADC", "ADC CONTINUOUS BEGIN");
	
    while (1) {

         // This is to show you the way to use the ADC continuous mode driver event callback.
         // This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         // However in this example, the data processing (print) is slow, so you barely block here.
         // Without using this event callback (to notify this task), you can still just call
         // adc_continuous_read() here in a loop, with/without a certain block timeout.
         // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

        while (1) {
            ret = adc_continuous_read(handle, result, ADC_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                //ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
                    uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
                    uint32_t data = EXAMPLE_ADC_GET_DATA(p);
		            // Check the channel number validation, the data is invalid if the channel num exceed the maximum channel 
		            if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
		                //ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, unit, chan_num, data);
		                //ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %lu", unit, chan_num, data);
						adc1_channels[chan_num] = data;
		            } else {
		                ESP_LOGW(TAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", unit, chan_num, data);
		            }
		        }
				
				// ESP_LOGI(TAG, "O: [%d] [%d] [%d] [%d] {%d} %d", adc1_channels[0], adc1_channels[3], adc1_channels[6], adc1_channels[7], adc1_channels[5], keyboard_matrix_input);
		        //  Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
		        //  To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
		        //  usually you don't need this delay (as this task will block for a while).
		        if(button_matrix_input == 1){
					button_array[BTN_UP] = adc1_channels[0] > button_thresholds[BTN_UP];
					button_array[BTN_DOWN] = adc1_channels[3] > button_thresholds[BTN_DOWN];
					button_array[BTN_LEFT] = adc1_channels[6] > button_thresholds[BTN_LEFT];
					button_array[BTN_RIGHT] = adc1_channels[7] > button_thresholds[BTN_RIGHT];
					
					button_array_adc[BTN_UP] = adc1_channels[0];
					button_array_adc[BTN_DOWN] = adc1_channels[3];
					button_array_adc[BTN_LEFT] = adc1_channels[6];
					button_array_adc[BTN_RIGHT] = adc1_channels[7];
				}else if (button_matrix_input == 2){
					button_array[BTN_SYNC] = adc1_channels[0] > button_thresholds[BTN_SYNC];
					button_array[BTN_HOME] = adc1_channels[3] > button_thresholds[BTN_HOME];
					button_array[BTN_ONE] = adc1_channels[6] > button_thresholds[BTN_ONE];
					button_array[BTN_TWO] = adc1_channels[7] > button_thresholds[BTN_TWO];
					
					button_array_adc[BTN_SYNC] = adc1_channels[0];
					button_array_adc[BTN_HOME] = adc1_channels[3];
					button_array_adc[BTN_ONE] = adc1_channels[6];
					button_array_adc[BTN_TWO] = adc1_channels[7];
				}else if (button_matrix_input == 3){
					button_array[BTN_PLUS] = adc1_channels[0] > button_thresholds[BTN_PLUS];
					button_array[BTN_MINUS] = adc1_channels[3] > button_thresholds[BTN_MINUS];
					button_array[BTN_B] = adc1_channels[6] > button_thresholds[BTN_B];
					button_array[BTN_POWER] = adc1_channels[7] > button_thresholds[BTN_POWER];
					
					button_array_adc[BTN_PLUS] = adc1_channels[0];
					button_array_adc[BTN_MINUS] = adc1_channels[3];
					button_array_adc[BTN_B] = adc1_channels[6];
					button_array_adc[BTN_POWER] = adc1_channels[7];
				}
				
		        vTaskDelay(1);
		    } else if (ret == ESP_ERR_TIMEOUT) {
		        // We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
		        break;
		    }
		}
	}
	
	ESP_ERROR_CHECK(adc_continuous_stop(handle));
	ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}

uint8_t get_IR_mode(){
//	uint8_t mode;
//	pixart_reg_read(&ir_handle, 0x33, &mode, 1);
//	return mode;
	return IR_camera_settings[0x33];
}

void read_IR(){
	pixart_ir_get_raw_data(&ir_handle, ir_raw_buffer);	
	//ESP_LOG_BUFFER_HEX("PIXART_IR read", data, length);
}

void load_IR_basic_buffer(uint8_t* destination){
	memcpy(destination, ir_raw_buffer, 3);
	memset(destination+2, (ir_raw_buffer[3] & 0xF0) | ((ir_raw_buffer[6] & 0xF0) >> 4), 1);
	memcpy(destination+3, ir_raw_buffer+4, 2);
	
	memcpy(destination+5, ir_raw_buffer+7, 2);
	memset(destination+7, (ir_raw_buffer[9] & 0xF0) | ((ir_raw_buffer[12] & 0xF0) >> 4), 1);
	memcpy(destination+8, ir_raw_buffer+10, 2);
}

void load_IR_extended_buffer(uint8_t* destination){
	memcpy(destination, ir_raw_buffer+1, 12);
}

void load_IR_full_buffer(uint8_t* destination){
	
}

//Converts the mg of acceleration we get from imu into the 10-bit format that the wiimote protocol calls for
int16_t accelerometer_mg_to_10bit(float accel_mg, float offset){ //TODO: speed this up, this was split into several lines for debugging reasons
	int16_t aligned = accel_mg + offset;
	float scaled = aligned * accel_scale_mg;
	int16_t scaled_int = (int16_t)(scaled); //TODO: ROUND INSTEAD OF TRUNCATING
	int16_t plus_zero = scaled_int + CALIBRATION_ACCEL_ZERO;

	return plus_zero;
}

void read_from_accelerometer(int16_t* processed_10bit_accel_x, int16_t* processed_10bit_accel_y, int16_t* processed_10bit_accel_z){
    esp_err_t ret = lsm6ds3_read_accel(&imu_handle, accel_mg);
	if (ret != ESP_OK) {
	    ESP_LOGE("LSM6DS3", "Read failed");
	    return;
	}
	//IMU is rotated from what is desired by wiimote protocol
	//x => [1]
	//y => [0]
	//z => [2]
	
	*processed_10bit_accel_x = accelerometer_mg_to_10bit(accel_mg[1],accel_offset_mg[1]);
	*processed_10bit_accel_y = accelerometer_mg_to_10bit(accel_mg[0],accel_offset_mg[0]);
	*processed_10bit_accel_z = accelerometer_mg_to_10bit(accel_mg[2],accel_offset_mg[2]);
}

//Note: destination is the start of the buffer because accelerometer data also places bits into the buttons section
void load_accelerometer_buffer(uint8_t* destination, uint16_t processed_10bit_accel_x, uint16_t processed_10bit_accel_y, uint16_t processed_10bit_accel_z){
	
	uint8_t accel_x_byte = ((processed_10bit_accel_x & 0x03FC) >> 2);
	uint8_t accel_x_lower_two_bits = (processed_10bit_accel_x & 0x03) << 5; //shift from bits 01 to bits 56 to align with where it goes in the button matrix
	
	uint8_t accel_y_byte = ((processed_10bit_accel_y & 0x03FC) >> 2);
	uint8_t accel_y_lower_two_bits = (processed_10bit_accel_y & 0x02) << 4; //shift from bit 1 to bits 5 to align with where it goes in the button matrix
	
	uint8_t accel_z_byte = ((processed_10bit_accel_z & 0x03FC) >> 2);
	uint8_t accel_z_lower_two_bits = (processed_10bit_accel_z & 0x02) << 5; //shift from bit 1 to bits 6 to align with where it goes in the button matrix
	
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

//TODO: LOOK AT THIS AND COMMENT PROPERLY (AND MAYBE REWRITE), WHY TF IS THERE 1200 AND NOT 2000?
void load_wii_motion_plus_buffer(uint8_t* destination){
	//fast mode reaches a peak of 2000 degrees per second? and slow mode is potentially 440?
	esp_err_t ret = lsm6ds3_read_gyro(&imu_handle, gyro_mdps);
	if (ret != ESP_OK) {
	    ESP_LOGE("LSM6DS3", "Read failed");
	    return;
	}

	gyro_dps[0] = gyro_mdps[0] / 1000.0f;
	gyro_dps[1] = gyro_mdps[1] / 1000.0f;
	gyro_dps[2] = gyro_mdps[2] / 1000.0f;

	//_base value should be 0 to 1 on the range from 0 to 440
	float gyro_yaw_base = gyro_dps[0] / 440;
	float gyro_roll_base = gyro_dps[1] / 440;
	float gyro_pitch_base = gyro_dps[2] / 440;
	
	bool slow_yaw = true, slow_roll = true, slow_pitch = true;
	
	if(gyro_yaw_base > 1.0 || gyro_yaw_base < -1.0){
		slow_yaw = false;
		gyro_yaw_base *= (440.0 / 1200);
	}
	if(gyro_roll_base > 1.0 || gyro_roll_base < -1.0){
		slow_roll = false;
		gyro_roll_base *= (440.0 / 1200);
	}
	if(gyro_pitch_base > 1.0 || gyro_pitch_base < -1.0){
		slow_pitch = false;
		gyro_pitch_base *= (440.0 / 1200);
	}
	
	int16_t gyro_yaw_14b = VALUE_ZERO + (gyro_yaw_base * VALUE_SCALE_OFFSET);
	int16_t gyro_roll_14b = VALUE_ZERO + (gyro_roll_base * VALUE_SCALE_OFFSET);
	int16_t gyro_pitch_14b = VALUE_ZERO + (gyro_pitch_base * VALUE_SCALE_OFFSET);
	
	uint8_t yaw7_0 = gyro_yaw_14b & 0xFF;
	uint8_t yaw13_8 = (gyro_yaw_14b & 0x3F00) >> 6;
	yaw13_8 |= ((slow_yaw << 1) | slow_pitch);

	uint8_t roll7_0 = gyro_roll_14b & 0xFF;
	uint8_t roll13_8 = (gyro_roll_14b & 0x3F00) >> 6;
	roll13_8 |= (slow_roll << 1 | 0); //THIS ZERO IS FOR EXTENSION CONNECTED, TODO: IMPLEMENT
	
	uint8_t pitch7_0 = gyro_pitch_14b & 0xFF;
	uint8_t pitch13_8 = (gyro_pitch_14b & 0x3F00) >> 6;
	pitch13_8 |= 0x02; //AAAAAAAAAAAAAAAAAAAAAAAAAAAA I SPENT TWO FUCKING DAYS ON roll13_8 vs pitch13_8

	
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
	//TODO: MAKE SURE DONT MEM OVERFLOW
	load_buttons_buffer(input_report);
	//	memcpy(input_report + 3, &address_low_16, 2);
	int index = 0;
	while(index < size){
		uint8_t chunk = size - index;
		uint16_t pointer = address_low_16 + index;
		if(chunk >= 16){
			input_report[2] = (0xF << 4) | (error & 0xF);
		}else{
			input_report[2] = (((chunk - 1) & 0xF) << 4) | (error & 0xF);
		}
		input_report[3] = (pointer & 0xFF00) >> 8;
		input_report[4] = pointer & 0x00FF;
	
		memset(input_report + 5, 0, 16);
		memcpy(input_report + 5, buffer + pointer, size);
	
		esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x21, input_report, 21);
		ESP_LOG_BUFFER_HEX("Responding to read", input_report + 5, size);
		
		index += 16;
	}

	
	//TODO: WORK ON READS OVER 16 bytes
}

//22 BB BB RR EE
void mote_input_data_acknowledge(uint8_t report_number, uint8_t error)
{
	load_buttons_buffer(input_report);
	input_report[2] = report_number;
	input_report[3] = error;
	esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x22, input_report, 4);
}

//central function for all hid input data (wiimote > wii)
void mote_input_data_core()
{
    static uint8_t old_buttons[2] = {0};
	static int16_t old_accel[3] = {0};
	uint8_t buttons[2] = {0};
	load_buttons_buffer(buttons);
	
	int16_t accel_10b_x, accel_10b_y, accel_10b_z;
	read_from_accelerometer(&accel_10b_x, &accel_10b_y, &accel_10b_z);
	
	read_IR();
	
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
				if(active_extension == EXT_WII_MOTION_PLUS_ACTIVE){
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
				if(get_IR_mode() == IR_EXTENDED){
					load_IR_extended_buffer(input_report+5);
				}else{
					memset(input_report+5,0xFF,12); //blank 12 IR bytes
					ESP_LOGW("SEND 0x37", "WRONG IR MODE: %d instead of 3", get_IR_mode());
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x33, input_report, 17);
				ESP_LOG_BUFFER_HEX("SEND 0x33", input_report, 17);
			    break;
			case 0x34:
				memcpy(input_report,buttons,2);
				if(active_extension == EXT_WII_MOTION_PLUS_ACTIVE){
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
				if(active_extension == EXT_WII_MOTION_PLUS_ACTIVE){
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
				if(get_IR_mode() == IR_BASIC){
					load_IR_basic_buffer(input_report+5);
				}else{
					memset(input_report+5,0xFF,10); //blank 10 IR bytes
					ESP_LOGW("SEND 0x37", "WRONG IR MODE: %d instead of 1", get_IR_mode());
				}
				if(active_extension == EXT_WII_MOTION_PLUS_ACTIVE){
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
				if(get_IR_mode() == IR_BASIC){
					load_IR_basic_buffer(input_report+5);
				}else{
					memset(input_report+5,0xFF,10); //blank 10 IR bytes
					ESP_LOGW("SEND 0x37", "WRONG IR MODE: %d instead of 1", get_IR_mode());
				}
				if(active_extension == EXT_WII_MOTION_PLUS_ACTIVE){
					load_wii_motion_plus_buffer(input_report+15);
				}else{
					memset(input_report+15,0xFF,6); //replace with 6 extension bytes
				}
				esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x37, input_report, 21);
				ESP_LOG_BUFFER_HEX("SEND 0x37", input_report, 21);
			    break;
			case 0x3d:
				if(active_extension == EXT_WII_MOTION_PLUS_ACTIVE){
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

//main loop for ESPmote, begun on successful bluetooth connection
void mote_hid_main_task(void *pvParameters)
{
    while (1) {
		assign_buttons_adc();
		mote_input_data_core();
		
//		ESP_LOGI(TAG, "%c%c%c%c%c%c%c%c%c%c%c%c%c",
//		    button_array[BTN_A] ? 'A' : ' ',
//		    button_array[BTN_B] ? 'B' : ' ',
//		    button_array[BTN_ONE] ? '1' : ' ',
//		    button_array[BTN_TWO] ? '2' : ' ',
//		    button_array[BTN_PLUS] ? '+' : ' ',
//		    button_array[BTN_MINUS] ? '-' : ' ',
//		    button_array[BTN_HOME] ? 'H' : ' ',
//		    button_array[BTN_UP] ? 'U' : ' ',
//		    button_array[BTN_DOWN] ? 'D' : ' ',
//		    button_array[BTN_LEFT] ? 'L' : ' ',
//		    button_array[BTN_RIGHT] ? 'R' : ' ',
//		    button_array[BTN_SYNC] ? 'S' : ' ',
//		    button_array[BTN_POWER] ? 'P' : ' '
//		);  
//		ESP_LOGI(TAG, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
//		    button_array_adc[BTN_A],
//		    button_array_adc[BTN_B],
//		    button_array_adc[BTN_ONE],
//		    button_array_adc[BTN_TWO],
//		    button_array_adc[BTN_PLUS],
//		    button_array_adc[BTN_MINUS],
//		    button_array_adc[BTN_HOME],
//		    button_array_adc[BTN_UP],
//		    button_array_adc[BTN_DOWN],
//		    button_array_adc[BTN_LEFT],
//		    button_array_adc[BTN_RIGHT],
//		    button_array_adc[BTN_SYNC],
//		    button_array_adc[BTN_POWER]
//		);  
		
//		gpio_set_level(LED1, !gpio_get_level(LED1));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void bt_hid_task_start_up(void)
{
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
		        //ignore this because actually bit 0 of any report is for rumble, this report is for *only* rumble
				break;
		        
		    case O_PLAYER_LEDS: //TODO: UPDATE TO REAL LEDS
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
					reportingMode = 0x3e; //for simplicity, lock both 3f and 3e behind 3e as they are the same.
				}
				ESP_LOGI( TAGW, "Data Reporting: TT[%2x] MM[%x]", continuousReporting, reportingMode);
					
				//bit 1 of any output report is requesting an acknowledgement input report
				if(param->output.data[0] & 0x02){
					mote_input_data_acknowledge(param->feature.report_id, ACK_SUCCESS);
				}
				
		        break;
		        
		    case O_IR_CAMERA_ENABLE: //Enables the 25 MHz (or is it 24) IR Clock
				// 1 byte - bit 2 = ON/OFF
				
				gpio_set_level(IR_CLK_GPIO, param->output.data[0] & 0x04);

				ESP_LOGI( TAGW, "Written %2x to IR Camera 1", param->output.data[0] & 0x04);
					
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
				
				bool extension_activated = false;
				
				if((param->output.data[0] & 0x04)){
					if(param->output.data[1] == 0xA2){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to speaker settings at %6x [%04x]", size, offset, offset_16);
						memcpy(speaker_settings + offset_16, param->output.data + 5, size);
					}else if(param->output.data[1] == 0xA4){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to extension controller settings and data at %6x [%04x]", size, offset, offset_16);
						if(active_extension != EXT_NONE){
							memcpy(extension_controller_settings_data + offset_16, param->output.data + 5, size);
						}else{
							return_ack = ACK_INACTIVE_EXTENSION;
						}
					}else if(param->output.data[1] == 0xA6){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to wii motion plus settings and data at %6x [%04x]", size, offset, offset_16);
						memcpy(wii_motion_plus_settings_data + offset_16, param->output.data + 5, size);
						if(offset_16 == 0x00FE && size == 1 && (param->output.data[5] == 0x04 || param->output.data[5] == 0x05 || param->output.data[5] == 0x07)){ //changing active extension to wii motion plus
							active_extension = EXT_WII_MOTION_PLUS_ACTIVE;
							status_byte |= 0x02;

							//copy wii motion plus extension data
							memcpy(extension_controller_settings_data + 0x00FA, wii_motion_plus_settings_data + 0x00FA, 6);
							memcpy(extension_controller_settings_data + 0x00FC, &EXTENSION_A4_TAG, 2);
							
							//copy wii motion plus calibration
							memcpy(extension_controller_settings_data + 0x20, wii_motion_plus_settings_data + 0x20, 0x20);
							
							extension_activated = true;
						}
					}else if(param->output.data[1] == 0xB0){
						ESP_LOGI( TAGW, "Attempting to write %d bytes to IR camera settings at 0x%06x [%04x]", size, offset, offset_16);
						memcpy(IR_camera_settings + offset_16, param->output.data + 5, size);
						pixart_reg_write(&ir_handle, offset_16, param->output.data + 5, size);
						
						ESP_LOG_BUFFER_HEX(TAGW, IR_camera_settings, 52);
					}else {
						ESP_LOGI( TAGW, "Attempting to write %d bytes to control registers at 0x%06x [%04x]", size, offset, offset_16);
					}
				}else{
					ESP_LOGI( TAGW, "Attempting to write %d bytes to EEPROM Memory at 0x%06x", size, offset);
					return_ack = ACK_ERROR;
				}
				ESP_LOG_BUFFER_HEX(TAGW, param->output.data + 5, size); 
					
				//apparently all writes request an ack automatically
				mote_input_data_acknowledge(param->feature.report_id, return_ack);

				if(extension_activated){
					mote_input_data_status();
				}
				
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
						ESP_LOGI( TAGW, "Attempting to read %d bytes from speaker settings at 0x%06x [%04x]", size, offset, offset_16);
						mote_input_data_read(size, 0, offset_16, speaker_settings);
					}else if(param->output.data[1] == 0xA4){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from extension controller settings and data at 0x%06x [%04x]", size, offset, offset_16);
						if(active_extension != EXT_NONE){
							mote_input_data_read(size, 0, offset_16, extension_controller_settings_data);
						}else{
							uint8_t zero_buffer[16] = {0};
							mote_input_data_read(16, READ_WRITE_ONLY, offset_16, zero_buffer - offset_16); //TODO: fix this, this is atrocious
						}
					}else if(param->output.data[1] == 0xA6){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from wii motion plus settings and data at 0x%06x [%04x]", size, offset, offset_16);
						mote_input_data_read(size, 0, offset_16, wii_motion_plus_settings_data);
					}else if(param->output.data[1] == 0xB0){
						ESP_LOGI( TAGW, "Attempting to read %d bytes from IR camera settings at 0x%06x [%04x]", size, offset, offset_16);
						//mote_input_data_read(size, 0, offset_16, IR_camera_settings);
						pixart_reg_read(&ir_handle, offset_16, input_report + 5, size);
						esp_hidd_dev_input_set(s_bt_hid_param.hid_dev, 0, 0x21, input_report, 21);
						ESP_LOG_BUFFER_HEX("Responding to read", input_report + 5, size);
					}else {
						ESP_LOGI( TAGW, "Attempting to read %d bytes from control registers at 0x%06x", size, offset);
					}
				}else{
					ESP_LOGI( TAGW, "Attempting to read %d bytes from EEPROM Memory at 0x%06x", size, offset);
					if(size + offset <= 48){
						mote_input_data_read(size, 0, offset_16, eeprom_start);
					}else{
						printf("READING OUT OF RANGE");
					}
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
		        
		    case O_IR_CAMERA_ENABLE_2: //Enables the IR Camera itself
		        // 1 byte - bit 2 = ON/OFF (alternate)
				
				gpio_set_level(IR_ENABLE_GPIO, param->output.data[0] & 0x04);
				
				//TODO: I am currently treating this as the main IR Camera Toggle (status byte), this MAY be the case in final	        
				if(param->output.data[0] & 0x04){
					status_byte |= 0x04;
				}
				
				ESP_LOGI( TAGW, "Written %2x to IR Camera 2", param->output.data[0] & 0x04);
					
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

	//i2c config
	i2c_master_bus_config_t bus_config = {
	    .i2c_port = I2C_MODE_MASTER,               // I2C port number
	    .sda_io_num = I2C_MASTER_SDA_IO,         // GPIO number for I2C sda signal
	    .scl_io_num = I2C_MASTER_SCL_IO,         // GPIO number for I2C scl signal
	    .clk_source = I2C_CLK_SRC_DEFAULT,  // I2C clock source, just use the default
	    .glitch_ignore_cnt = 7,             // glitch filter, again, just use the default
	    .flags = {
	        .enable_internal_pullup = false, // enable internal pullup resistors (oled screen does not have one)
	    },
	};
	
	i2c_master_bus_handle_t i2c_bus_handle = NULL;
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));
	
	//lsm6ds3 config
	ret = lsm6ds3_init(i2c_bus_handle, &imu_handle);
	if (ret != ESP_OK) {
	    ESP_LOGE("LSM6DS3", "Creation failed");
	    return;
	}else{
		ESP_LOGI("LSM6DS3", "Successful Creation");
	}
	
	lsm6ds3_set_accel_odr(&imu_handle, LSM6DS3_XL_ODR_104Hz);
	lsm6ds3_set_accel_full_scale(&imu_handle, LSM6DS3_4g);
	lsm6ds3_set_gyro_odr(&imu_handle, LSM6DS3_GY_ODR_104Hz);
	lsm6ds3_set_gyro_full_scale(&imu_handle, LSM6DS3_2000dps);
	
	//pixart_ir config
	ret = pixart_ir_init(i2c_bus_handle, &ir_handle);
	if (ret != ESP_OK) {
	    ESP_LOGE("PIXART_IR", "Creation failed");
	    return;
	}else{
		ESP_LOGI("PIXART_IR", "Successful Creation");
	}
	
	//nvs flash init (TODO: WHAT DOES THIS DO?)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

	//hid gap init
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
    ESP_ERROR_CHECK( esp_hidd_dev_init(&bt_hid_config, ESP_HID_TRANSPORT_BT, bt_hidd_event_callback, &s_bt_hid_param.hid_dev));
	
	setLEDBinary(8);
	
	xTaskCreate(continuous_adc, "adc_async_buttons_task", 2 * 1024, NULL, configMAX_PRIORITIES - 4, &adc_task_hdl);
		
#if CONFIG_BT_SDP_COMMON_ENABLED
    ESP_ERROR_CHECK(esp_sdp_register_callback(esp_sdp_cb));
    ESP_ERROR_CHECK(esp_sdp_init());
#endif /* CONFIG_BT_SDP_COMMON_ENABLED */

#endif /* CONFIG_BT_HID_DEVICE_ENABLED */

	int16_t x, y, z;
	read_from_accelerometer(&x, &y, &z);
	ESP_LOGI("IMU_DATA","%d %d %d", x, y, z);
	ir_points_data irpd = {0};
	pixart_ir_get_data(&ir_handle, &irpd);
	ESP_LOGI("IR DATA", "%d,%d[%d] %d,%d[%d] %d,%d[%d] %d,%d[%d]", irpd.point1.x, irpd.point1.y,irpd.point1.size, irpd.point2.x, irpd.point2.y,irpd.point2.size, irpd.point3.x, irpd.point3.y,irpd.point3.size, irpd.point4.x, irpd.point4.y,irpd.point4.size);
}
