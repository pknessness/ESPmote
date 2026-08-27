/*
 * MIT License
 *
 * Copyright (c) 2025 Adam G. Sweeney <agsweeney@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LSM6DS3_H
#define LSM6DS3_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"
//#include "driver/spi_master.h"
#include "driver/i2c_types.h"
#include "lsm6ds3_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float accel_offset_mg[3];
    float gyro_offset_mdps[3];
    bool accel_calibrated;
    bool gyro_calibrated;
} lsm6ds3_calibration_t;

typedef struct {
	i2c_master_dev_handle_t i2c_handle; /*!< I2C device handle for LSM6DS3 */
    uint8_t address;
    lsm6ds3_ctx_t ctx;
    lsm6ds3_calibration_t calibration;
} lsm6ds3_handle_t;

esp_err_t lsm6ds3_init(i2c_master_bus_handle_t bus_handle, lsm6ds3_handle_t *handle);
esp_err_t lsm6ds3_deinit(lsm6ds3_handle_t *handle);
esp_err_t lsm6ds3_reset(lsm6ds3_handle_t *handle);
esp_err_t lsm6ds3_get_device_id(lsm6ds3_handle_t *handle, uint8_t *device_id);
esp_err_t lsm6ds3_set_accel_odr(lsm6ds3_handle_t *handle, lsm6ds3_odr_xl_t odr);
esp_err_t lsm6ds3_set_accel_full_scale(lsm6ds3_handle_t *handle, lsm6ds3_fs_xl_t fs);
esp_err_t lsm6ds3_set_gyro_odr(lsm6ds3_handle_t *handle, lsm6ds3_odr_g_t odr);
esp_err_t lsm6ds3_set_gyro_full_scale(lsm6ds3_handle_t *handle, lsm6ds3_fs_g_t fs);
esp_err_t lsm6ds3_read_accel(lsm6ds3_handle_t *handle, float accel_mg[3]);
esp_err_t lsm6ds3_read_gyro(lsm6ds3_handle_t *handle, float gyro_mdps[3]);
esp_err_t lsm6ds3_enable_block_data_update(lsm6ds3_handle_t *handle, bool enable);
esp_err_t lsm6ds3_calibrate_accel(lsm6ds3_handle_t *handle, uint32_t samples, uint32_t sample_delay_ms);
esp_err_t lsm6ds3_calibrate_gyro(lsm6ds3_handle_t *handle, uint32_t samples, uint32_t sample_delay_ms);
esp_err_t lsm6ds3_set_accel_offset(lsm6ds3_handle_t *handle, const float offset_mg[3]);
esp_err_t lsm6ds3_set_accel_offset_axis(lsm6ds3_handle_t *handle, uint8_t axis, float offset_mg);
esp_err_t lsm6ds3_set_gyro_offset(lsm6ds3_handle_t *handle, const float offset_mdps[3]);
esp_err_t lsm6ds3_set_gyro_offset_axis(lsm6ds3_handle_t *handle, uint8_t axis, float offset_mdps);
esp_err_t lsm6ds3_get_accel_offset(lsm6ds3_handle_t *handle, float offset_mg[3]);
esp_err_t lsm6ds3_get_gyro_offset(lsm6ds3_handle_t *handle, float offset_mdps[3]);
esp_err_t lsm6ds3_clear_accel_calibration(lsm6ds3_handle_t *handle);
esp_err_t lsm6ds3_clear_gyro_calibration(lsm6ds3_handle_t *handle);
esp_err_t lsm6ds3_set_accel_hw_offset(lsm6ds3_handle_t *handle, int8_t offset_x, int8_t offset_y, int8_t offset_z);
esp_err_t lsm6ds3_set_accel_hw_offset_axis(lsm6ds3_handle_t *handle, uint8_t axis, int8_t offset);
esp_err_t lsm6ds3_set_gyro_hw_offset(lsm6ds3_handle_t *handle, int8_t offset_x, int8_t offset_y, int8_t offset_z);
esp_err_t lsm6ds3_save_calibration_to_nvs(lsm6ds3_handle_t *handle, const char *namespace);
esp_err_t lsm6ds3_load_calibration_from_nvs(lsm6ds3_handle_t *handle, const char *namespace);

#ifdef __cplusplus
}
#endif

#endif

