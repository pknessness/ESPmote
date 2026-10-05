/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/

Huge help from Kako's blog at http://www.kako.com/neta/2008-009/2008-009.html
*/

#pragma once

#include <esp_err.h>
#include <driver/i2c_master.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define EXTENSION_ID  0x52

typedef struct {
	i2c_master_dev_handle_t i2c_handle; /*!< I2C device handle for PIXART_IR */
    uint8_t address;
} extension_handle_t;

typedef struct {
	uint8_t stick_x;
	uint8_t stick_y;
	uint16_t accel_x; //10 bit
	uint16_t accel_y; //10 bit
	uint16_t accel_z; //10 bit
	bool button_C;
	bool button_Z;
} nunchuk_data_t;

esp_err_t extension_init_mutex();

esp_err_t extension_reg_write(extension_handle_t *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
esp_err_t extension_reg_read(extension_handle_t *handle, uint8_t reg, uint8_t *data, uint16_t len);

esp_err_t extension_init(i2c_master_bus_handle_t bus_handle, extension_handle_t *handle);
esp_err_t extension_deinit(extension_handle_t *handle);

esp_err_t extension_get_data(extension_handle_t *handle);
esp_err_t extension_get_raw_data(extension_handle_t *handle, uint8_t *data, uint8_t len);
esp_err_t extension_get_raw_data_sync(extension_handle_t *handle, uint8_t *data, uint8_t len);

nunchuk_data_t extension_decompose_nunchuck(uint8_t *data);


