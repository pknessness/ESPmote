/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/

Huge help from Kako's blog at http://www.kako.com/neta/2008-009/2008-009.html
*/

#include "pixart_ir.h"
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <esp_check.h>

static const char *TAG = "PIXART";

#define PIXART_IR_MAX_REG_TRANSFER_LEN 16  // Maximum register transfer length (register + data) (This might be 8, 16 is random num)

static esp_err_t reg_write_i2c(pixart_ir_handle_t *device, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    
    if (bufp == NULL && len > 0) {
        ESP_LOGE(TAG, "I2C write buffer pointer is NULL");
        return -1;
    }
    
    if (len > PIXART_IR_MAX_REG_TRANSFER_LEN - 1) {
        ESP_LOGE(TAG, "I2C write length %u exceeds maximum %d", len, PIXART_IR_MAX_REG_TRANSFER_LEN - 1);
        return -1;
    }
    
    uint8_t write_buf[PIXART_IR_MAX_REG_TRANSFER_LEN];
    write_buf[0] = reg;
    if (len > 0) {
        memcpy(&write_buf[1], bufp, len);
    }
    
    return i2c_master_transmit(device->i2c_handle, write_buf, len + 1, -1);
}

static esp_err_t reg_read_i2c(pixart_ir_handle_t *device, uint8_t reg, uint8_t *readbuf, uint16_t len)
{
    return i2c_master_transmit_receive(device->i2c_handle, &reg, 1, readbuf, len, -1);
}

esp_err_t pixart_ir_init(i2c_master_bus_handle_t bus_handle, pixart_ir_handle_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
	
	i2c_device_config_t dev_cfg = {
	    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
	    .device_address = PIXART_ID,
	    .scl_speed_hz = 400000,
	};
    
    memset(handle, 0, sizeof(pixart_ir_handle_t));
    
	i2c_master_bus_add_device(bus_handle, &dev_cfg, &handle->i2c_handle);
    
    uint8_t whoamI = 0;
//    if (pixart_ir_get_id(handle, &whoamI) != 0) {
//        ESP_LOGE(TAG, "Failed to read device ID");
//        return ESP_ERR_NOT_FOUND;
//    }
    
//	if (whoamI != 0x6B) {
//	    ESP_LOGE(TAG, "PIXART_IR invalid device ID: 0x%02X (expected 0x%02X)", whoamI, 0x6B);
//	    return ESP_ERR_NOT_FOUND;
//	}
    
    ESP_LOGI(TAG, "PIXART_IR initialized successfully (ID: 0x%02X)", whoamI);
	
	//TODO: MAKE THIS CLEANER CODE
	//these are writes in format of register, data 
	uint8_t init_packet_1[2] = {0x30, 0x01};
	uint8_t init_packet_2[2] = {0x30, 0x08};
	uint8_t init_packet_3[2] = {0x06, 0x90};
	uint8_t init_packet_4[2] = {0x08, 0xC0};
	uint8_t init_packet_5[2] = {0x1A, 0x40};
	uint8_t init_packet_6[2] = {0x33, 0x33};

	i2c_master_transmit(handle->i2c_handle, init_packet_1, 2, -1);
	ESP_LOGI(TAG, "Init1", whoamI);
	i2c_master_transmit(handle->i2c_handle, init_packet_2, 2, -1);
	ESP_LOGI(TAG, "Init2", whoamI);
	i2c_master_transmit(handle->i2c_handle, init_packet_3, 2, -1);
	ESP_LOGI(TAG, "Init3", whoamI);
	i2c_master_transmit(handle->i2c_handle, init_packet_4, 2, -1);
	ESP_LOGI(TAG, "Init4", whoamI);
	i2c_master_transmit(handle->i2c_handle, init_packet_5, 2, -1);
	ESP_LOGI(TAG, "Init5", whoamI);
	i2c_master_transmit(handle->i2c_handle, init_packet_6, 2, -1);
	ESP_LOGI(TAG, "Init6", whoamI);
	
    return ESP_OK;
}

esp_err_t pixart_ir_get_data(pixart_ir_handle_t *handle){
	uint8_t data[16] = {0};
	reg_read_i2c(handle, 0x36, data, 16);
	
	ESP_LOGI(TAG, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
	return ESP_OK;
}

esp_err_t pixart_ir_set_sensitivity(pixart_ir_handle_t *handle){
	return ESP_OK;
}

esp_err_t pixart_ir_get_id(pixart_ir_handle_t *handle, uint8_t* whoami){
	return reg_read_i2c(handle, PIXART_IR_WHO_AM_I, whoami, 1);
}


