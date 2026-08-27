/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/
*/

#include "pixart_ir.h"
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <esp_check.h>

//esp_err_t pixart_ir_init(i2c_master_bus_handle_t bus_handle, lsm6ds3_handle_t *handle)
//{
//    if (handle == NULL) {
//        return ESP_ERR_INVALID_ARG;
//    }
//	
//	i2c_device_config_t dev_cfg = {
//	    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//	    .device_address = LSM6DS3_ID,
//	    .scl_speed_hz = 400000,
//	};
//    
//    memset(handle, 0, sizeof(lsm6ds3_handle_t));
//    handle->calibration.accel_calibrated = false;
//    handle->calibration.gyro_calibrated = false;
//    
////	handle->i2c_handle = config->bus.i2c.bus_handle;
////    handle->address = config->bus.i2c.address;
//	i2c_master_bus_add_device(bus_handle, &dev_cfg, &handle->i2c_handle);
//    handle->ctx.write_reg = platform_write_i2c;
//    handle->ctx.read_reg = platform_read_i2c;
//    
//    handle->ctx.mdelay = platform_delay;
//    handle->ctx.handle = handle;
//    
//    uint8_t whoamI;
//    if (lsm6ds3_device_id_get(&handle->ctx, &whoamI) != 0) {
//        ESP_LOGE(TAG, "Failed to read device ID");
//        return ESP_ERR_NOT_FOUND;
//    }
//    
////    if (whoamI != LSM6DS3_ID) {
////        ESP_LOGE(TAG, "Invalid device ID: 0x%02X (expected 0x%02X)", whoamI, LSM6DS3_ID);
////        return ESP_ERR_NOT_FOUND;
////    }
//	
//	//Changing this to 6A, because for some reason even though I can't get the device id unless i set it to 0x6A, internally it shows 0x6B
//	if (whoamI != 0x6B) {
//	    ESP_LOGE(TAG, "Invalid device ID: 0x%02X (expected 0x%02X)", whoamI, 0x6B);
//	    return ESP_ERR_NOT_FOUND;
//	}
//    
//    ESP_LOGI(TAG, "LSM6DS3 initialized successfully (ID: 0x%02X)", whoamI);
//    
//    return ESP_OK;
//}




