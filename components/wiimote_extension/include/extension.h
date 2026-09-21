/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/

Huge help from Kako's blog at http://www.kako.com/neta/2008-009/2008-009.html
*/

#pragma once
#include <esp_err.h>
#include <driver/i2c_master.h>

#define EXTENSION_ID  0x52

typedef struct {
	i2c_master_dev_handle_t i2c_handle; /*!< I2C device handle for PIXART_IR */
    uint8_t address;
} extension_handle_t;

esp_err_t extension_reg_write(extension_handle_t *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
esp_err_t extension_reg_read(extension_handle_t *handle, uint8_t reg, uint8_t *data, uint16_t len);

esp_err_t extension_init(i2c_master_bus_handle_t bus_handle, extension_handle_t *handle);
esp_err_t extension_deinit(extension_handle_t *handle);

esp_err_t extension_get_data(extension_handle_t *handle);
esp_err_t extension_get_raw_data(extension_handle_t *handle, uint8_t *data);
esp_err_t extension_set_sensitivity(extension_handle_t *handle);



