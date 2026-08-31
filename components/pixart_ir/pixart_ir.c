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

esp_err_t pixart_reg_write(pixart_ir_handle_t *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
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
    
    return i2c_master_transmit(handle->i2c_handle, write_buf, len + 1, -1);
}

esp_err_t pixart_reg_read(pixart_ir_handle_t *handle, uint8_t reg, uint8_t *data, uint16_t len){
	
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
    
    ESP_LOGI(TAG, "PIXART_IR initialized successfully");
	
	//TODO: MAKE THIS CLEANER CODE
	//these are writes in format of register, data 
//	uint8_t init_packet_1[2] = {0x30, 0x01};
//	uint8_t init_packet_2[2] = {0x30, 0x08};
//	uint8_t init_packet_3[2] = {0x06, 0x90};
//	uint8_t init_packet_4[2] = {0x08, 0xC0};
//	uint8_t init_packet_5[2] = {0x1A, 0x40};
//	uint8_t init_packet_6[2] = {0x33, 0x33};
//
//	i2c_master_transmit(handle->i2c_handle, init_packet_1, 2, -1);
//	i2c_master_transmit(handle->i2c_handle, init_packet_2, 2, -1);
//	i2c_master_transmit(handle->i2c_handle, init_packet_3, 2, -1);
//	i2c_master_transmit(handle->i2c_handle, init_packet_4, 2, -1);
//	i2c_master_transmit(handle->i2c_handle, init_packet_5, 2, -1);
//	i2c_master_transmit(handle->i2c_handle, init_packet_6, 2, -1);
	
    return ESP_OK;
}

esp_err_t pixart_ir_get_data(pixart_ir_handle_t *handle, ir_points_data *points_data){
	uint8_t data[16] = {0};
//	reg_read_i2c(handle, 0x36, data, 16);
	uint8_t send = 0x36;
	i2c_master_transmit(handle->i2c_handle, &send, 1, -1);	
	i2c_master_receive(handle->i2c_handle, data, 16, -1);
	
	points_data->point1.x = data[1] | ((data[3] & 0x30) << 4);
	points_data->point1.y = data[2] | ((data[3] & 0xC0) << 2);
	points_data->point1.size = data[3] & 0xF;
	
	points_data->point1.x = data[4] | ((data[6] & 0x30) << 4);
	points_data->point1.y = data[5] | ((data[6] & 0xC0) << 2);
	points_data->point1.size = data[6] & 0xF;
	
	points_data->point1.x = data[7] | ((data[9] & 0x30) << 4);
	points_data->point1.y = data[8] | ((data[9] & 0xC0) << 2);
	points_data->point1.size = data[9] & 0xF;
	
	points_data->point1.x = data[10] | ((data[12] & 0x30) << 4);
	points_data->point1.y = data[11] | ((data[12] & 0xC0) << 2);
	points_data->point1.size = data[12] & 0xF;
	
	//ESP_LOGI(TAG, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
	return ESP_OK;
}

esp_err_t pixart_ir_get_raw_data(pixart_ir_handle_t *handle, uint8_t *data){
//	uint8_t raw_data[16] = {0};
//	reg_read_i2c(handle, 0x36, data, 16);
	uint8_t send = 0x36;
	i2c_master_transmit(handle->i2c_handle, &send, 1, -1);	
	i2c_master_receive(handle->i2c_handle, data, 16, -1);
	
	ESP_LOGI(TAG, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
	return ESP_OK;
}

esp_err_t pixart_ir_set_sensitivity(pixart_ir_handle_t *handle){
	return ESP_OK;
}

