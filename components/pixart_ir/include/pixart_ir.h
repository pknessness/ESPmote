/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/
*/

#pragma once
#include <esp_err.h>
#include <driver/i2c_master.h>

#define PIXART_ADDR  0x58
#define PIXART_X_MAX 1023
#define PIXART_X_MIN 0

#define PIXART_Y_MAX 754
#define PIXART_Y_MIN 0

typedef struct
{
	uint16_t x;
	uint16_t y;
	uint8_t size;
} ir_data_t;

typedef struct {
	i2c_master_dev_handle_t i2c_handle; /*!< I2C device handle for PIXART_IR */
    uint8_t address;
} pixart_ir_handle_t;

esp_err_t pixart_ir_init(i2c_master_bus_handle_t bus_handle, pixart_ir_handle_t *handle);



