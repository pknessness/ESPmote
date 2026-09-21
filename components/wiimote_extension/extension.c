/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/

Huge help from Kako's blog at http://www.kako.com/neta/2008-009/2008-009.html
*/

#include "extension.h"
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <esp_check.h>

static const char *TAG = "EXTENSION";

#define EXTENSION_MAX_REG_TRANSFER_LEN 16  // Maximum register transfer length (register + data) (This might be 8, 16 is random num)

esp_err_t extension_reg_write(extension_handle_t *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
	
    if (bufp == NULL && len > 0) {
        ESP_LOGE(TAG, "I2C write buffer pointer is NULL");
        return -1;
    }
    
    if (len > EXTENSION_MAX_REG_TRANSFER_LEN - 1) {
        ESP_LOGE(TAG, "I2C write length %u exceeds maximum %d", len, EXTENSION_MAX_REG_TRANSFER_LEN - 1);
        return -1;
    }
    
    uint8_t write_buf[EXTENSION_MAX_REG_TRANSFER_LEN];
    write_buf[0] = reg;
    if (len > 0) {
        memcpy(&write_buf[1], bufp, len);
    }
    
    return i2c_master_transmit(handle->i2c_handle, write_buf, len + 1, -1);
}

esp_err_t extension_reg_read(extension_handle_t *handle, uint8_t reg, uint8_t *data, uint16_t len){
	
	if(len <= 0){
		ESP_LOGE(TAG, "I2C invalid read length");
	    return -1;
	}
	
	if (data == NULL && len > 0) {
	    ESP_LOGE(TAG, "I2C write buffer pointer is NULL");
	    return -1;
	}
	
	uint8_t register_to_read = reg;
	esp_err_t transmit_err = i2c_master_transmit(handle->i2c_handle, &register_to_read, 1, -1);	
	
	if (transmit_err) {
	    ESP_LOGE(TAG, "Invalid Write");
	    return transmit_err;
	}
	
	esp_err_t recv_err = i2c_master_receive(handle->i2c_handle, data, len, -1);
	
	if (recv_err) {
	    ESP_LOGE(TAG, "Invalid Read");
	    return recv_err;
	}
	return ESP_OK;
		
//	esp_err_t ret = i2c_master_transmit_receive(handle->i2c_handle, &reg, 1, data, len, -1);
//	if (ret) {
//	    ESP_LOGE(TAG, "Invalid ReadWrite");
//	    return ret;
//	}
//	return ESP_OK;
}

esp_err_t extension_init(i2c_master_bus_handle_t bus_handle, extension_handle_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
	
	i2c_device_config_t dev_cfg = {
	    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
	    .device_address = EXTENSION_ID,
	    .scl_speed_hz = 400000,
	};
    
    memset(handle, 0, sizeof(extension_handle_t));
    
	esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &handle->i2c_handle);
    
    ESP_LOGI(TAG, "EXTENSION initialized successfully");
	
    return ret;
}

esp_err_t extension_deinit(extension_handle_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
	i2c_master_bus_rm_device(handle->i2c_handle);
    
    ESP_LOGI(TAG, "EXTENSION deinitialized successfully");
	
    return ESP_OK;
}

// For some reason, when reading from a nunchuck, you need to read and then write, possibly for the next read. 
// Does each read take that long that you need to write for the next read immediately after the read?
esp_err_t extension_get_data(extension_handle_t *handle){
	uint8_t data[16] = {0};
	
	uint8_t send = 0x00;
	i2c_master_receive(handle->i2c_handle, data, 16, -1);
	i2c_master_transmit(handle->i2c_handle, &send, 1, -1);	
	
	//ESP_LOGI(TAG, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
	
	return ESP_OK;
}

esp_err_t extension_get_raw_data(extension_handle_t *handle, uint8_t *data){

	uint8_t send = 0x00;
	i2c_master_receive(handle->i2c_handle, data, 16, -1);
	i2c_master_transmit(handle->i2c_handle, &send, 1, -1);	

	//ESP_LOGI(TAG, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

	return ESP_OK;
}

esp_err_t extension_set_sensitivity(extension_handle_t *handle){
	return ESP_OK;
}

