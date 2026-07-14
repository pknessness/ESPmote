/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2018-2025 STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is derived from STMicroelectronics STMems_Standard_C_drivers:
 * https://github.com/STMicroelectronics/STMems_Standard_C_drivers
 */

#ifndef LSM6DS3_REG_H
#define LSM6DS3_REG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LSM6DS3_ID 0x69

typedef struct {
    int16_t i16bit[3];
    uint8_t u8bit[6];
} lsm6ds3_reg_t;

typedef struct {
    int32_t (*write_reg)(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
    int32_t (*read_reg)(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
    void (*mdelay)(void *handle, uint32_t ms);
    void *handle;
} lsm6ds3_ctx_t;

typedef enum {
    PROPERTY_DISABLE = 0,
    PROPERTY_ENABLE = 1
} lsm6ds3_property_t;

typedef enum {
    LSM6DS3_XL_ODR_OFF = 0,
    LSM6DS3_XL_ODR_12_5Hz = 1,
    LSM6DS3_XL_ODR_26Hz = 2,
    LSM6DS3_XL_ODR_52Hz = 3,
    LSM6DS3_XL_ODR_104Hz = 4,
    LSM6DS3_XL_ODR_208Hz = 5,
    LSM6DS3_XL_ODR_416Hz = 6,
    LSM6DS3_XL_ODR_833Hz = 7,
    LSM6DS3_XL_ODR_1_66kHz = 8,
    LSM6DS3_XL_ODR_3_33kHz = 9,
    LSM6DS3_XL_ODR_6_66kHz = 10
} lsm6ds3_odr_xl_t;

typedef enum {
    LSM6DS3_2g = 0,
    LSM6DS3_4g = 2,
    LSM6DS3_8g = 3,
    LSM6DS3_16g = 1
} lsm6ds3_fs_xl_t;

typedef enum {
    LSM6DS3_GY_ODR_OFF = 0,
    LSM6DS3_GY_ODR_12_5Hz = 1,
    LSM6DS3_GY_ODR_26Hz = 2,
    LSM6DS3_GY_ODR_52Hz = 3,
    LSM6DS3_GY_ODR_104Hz = 4,
    LSM6DS3_GY_ODR_208Hz = 5,
    LSM6DS3_GY_ODR_416Hz = 6,
    LSM6DS3_GY_ODR_833Hz = 7,
    LSM6DS3_GY_ODR_1_66kHz = 8
} lsm6ds3_odr_g_t;

typedef enum {
    LSM6DS3_125dps = 0,
    LSM6DS3_250dps = 1,
    LSM6DS3_500dps = 2,
    LSM6DS3_1000dps = 3,
    LSM6DS3_2000dps = 4
} lsm6ds3_fs_g_t;

typedef struct {
    uint8_t xlda : 1;
    uint8_t gda : 1;
    uint8_t tda : 1;
    uint8_t _not_used_1 : 1;
    uint8_t xlda_gy : 1;
    uint8_t gda_gy : 1;
    uint8_t _not_used_2 : 1;
    uint8_t den_lh : 1;
} lsm6ds3_status_reg_t;

int32_t lsm6ds3_device_id_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_reset_set(lsm6ds3_ctx_t *ctx, lsm6ds3_property_t val);
int32_t lsm6ds3_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_block_data_update_set(lsm6ds3_ctx_t *ctx, lsm6ds3_property_t val);
int32_t lsm6ds3_xl_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t val);
int32_t lsm6ds3_xl_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t *val);
int32_t lsm6ds3_xl_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_xl_t val);
int32_t lsm6ds3_xl_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_xl_t *val);
int32_t lsm6ds3_gy_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t val);
int32_t lsm6ds3_gy_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t *val);
int32_t lsm6ds3_gy_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t val);
int32_t lsm6ds3_gy_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t *val);
int32_t lsm6ds3_status_reg_get(lsm6ds3_ctx_t *ctx, lsm6ds3_status_reg_t *val);
int32_t lsm6ds3_acceleration_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);
int32_t lsm6ds3_angular_rate_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);
int32_t lsm6ds3_xl_usr_offset_x_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_xl_usr_offset_x_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_xl_usr_offset_y_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_xl_usr_offset_y_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_xl_usr_offset_z_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_xl_usr_offset_z_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_gy_usr_offset_x_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_usr_offset_x_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_gy_usr_offset_y_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_usr_offset_y_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
int32_t lsm6ds3_gy_usr_offset_z_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_usr_offset_z_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

float lsm6ds3_from_fs2g_to_mg(int16_t lsb);
float lsm6ds3_from_fs4g_to_mg(int16_t lsb);
float lsm6ds3_from_fs8g_to_mg(int16_t lsb);
float lsm6ds3_from_fs16g_to_mg(int16_t lsb);
float lsm6ds3_from_fs125dps_to_mdps(int16_t lsb);
float lsm6ds3_from_fs250dps_to_mdps(int16_t lsb);
float lsm6ds3_from_fs500dps_to_mdps(int16_t lsb);
float lsm6ds3_from_fs1000dps_to_mdps(int16_t lsb);
float lsm6ds3_from_fs2000dps_to_mdps(int16_t lsb);

#ifdef __cplusplus
}
#endif

#endif

