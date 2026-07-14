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

#include "lsm6ds3_reg.h"

#define LSM6DS3_FUNC_CFG_ACCESS 0x01
#define LSM6DS3_SENSOR_SYNC_TIME_FRAME 0x04
#define LSM6DS3_SENSOR_SYNC_RES_RATIO 0x05
#define LSM6DS3_FIFO_CTRL1 0x06
#define LSM6DS3_FIFO_CTRL2 0x07
#define LSM6DS3_FIFO_CTRL3 0x08
#define LSM6DS3_FIFO_CTRL4 0x09
#define LSM6DS3_FIFO_CTRL5 0x0A
#define LSM6DS3_ORIENT_CFG_G 0x0B
#define LSM6DS3_INT1_CTRL 0x0D
#define LSM6DS3_INT2_CTRL 0x0E
#define LSM6DS3_WHO_AM_I 0x0F
#define LSM6DS3_CTRL1_XL 0x10
#define LSM6DS3_CTRL2_G 0x11
#define LSM6DS3_CTRL3_C 0x12
#define LSM6DS3_CTRL4_C 0x13
#define LSM6DS3_CTRL5_C 0x14
#define LSM6DS3_CTRL6_C 0x15
#define LSM6DS3_CTRL7_G 0x16
#define LSM6DS3_CTRL8_XL 0x17
#define LSM6DS3_CTRL9_XL 0x18
#define LSM6DS3_CTRL10_C 0x19
#define LSM6DS3_MASTER_CONFIG 0x1A
#define LSM6DS3_WAKE_UP_SRC 0x1B
#define LSM6DS3_TAP_SRC 0x1C
#define LSM6DS3_D6D_SRC 0x1D
#define LSM6DS3_STATUS_REG 0x1E
#define LSM6DS3_OUT_TEMP_L 0x20
#define LSM6DS3_OUT_TEMP_H 0x21
#define LSM6DS3_OUTX_L_G 0x22
#define LSM6DS3_OUTX_H_G 0x23
#define LSM6DS3_OUTY_L_G 0x24
#define LSM6DS3_OUTY_H_G 0x25
#define LSM6DS3_OUTZ_L_G 0x26
#define LSM6DS3_OUTZ_H_G 0x27
#define LSM6DS3_OUTX_L_XL 0x28
#define LSM6DS3_OUTX_H_XL 0x29
#define LSM6DS3_OUTY_L_XL 0x2A
#define LSM6DS3_OUTY_H_XL 0x2B
#define LSM6DS3_OUTZ_L_XL 0x2C
#define LSM6DS3_OUTZ_H_XL 0x2D
#define LSM6DS3_X_OFS_USR 0x73
#define LSM6DS3_Y_OFS_USR 0x74
#define LSM6DS3_Z_OFS_USR 0x75

static int32_t lsm6ds3_read_reg(lsm6ds3_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
    return ctx->read_reg(ctx->handle, reg, data, len);
}

static int32_t lsm6ds3_write_reg(lsm6ds3_ctx_t *ctx, uint8_t reg, const uint8_t *data, uint16_t len)
{
    return ctx->write_reg(ctx->handle, reg, data, len);
}

int32_t lsm6ds3_device_id_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return lsm6ds3_read_reg(ctx, LSM6DS3_WHO_AM_I, val, 1);
}

int32_t lsm6ds3_reset_set(lsm6ds3_ctx_t *ctx, lsm6ds3_property_t val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, &reg, 1)) {
        return -1;
    }
    reg = (reg & ~0x01) | (uint8_t)val;
    return lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, &reg, 1);
}

int32_t lsm6ds3_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, &reg, 1)) {
        return -1;
    }
    *val = reg & 0x01;
    return 0;
}

int32_t lsm6ds3_block_data_update_set(lsm6ds3_ctx_t *ctx, lsm6ds3_property_t val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, &reg, 1)) {
        return -1;
    }
    reg = (reg & ~0x40) | ((uint8_t)val << 6);
    return lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, &reg, 1);
}

int32_t lsm6ds3_xl_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, &reg, 1)) {
        return -1;
    }
    reg = (reg & ~0xF0) | ((uint8_t)val << 4);
    return lsm6ds3_write_reg(ctx, LSM6DS3_CTRL1_XL, &reg, 1);
}

int32_t lsm6ds3_xl_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t *val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, &reg, 1)) {
        return -1;
    }
    *val = (lsm6ds3_odr_xl_t)((reg & 0xF0) >> 4);
    return 0;
}

int32_t lsm6ds3_xl_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_xl_t val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, &reg, 1)) {
        return -1;
    }
    reg = (reg & ~0x0C) | ((uint8_t)val << 2);
    return lsm6ds3_write_reg(ctx, LSM6DS3_CTRL1_XL, &reg, 1);
}

int32_t lsm6ds3_xl_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_xl_t *val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, &reg, 1)) {
        return -1;
    }
    *val = (lsm6ds3_fs_xl_t)((reg & 0x0C) >> 2);
    return 0;
}

int32_t lsm6ds3_gy_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, &reg, 1)) {
        return -1;
    }
    reg = (reg & ~0xF0) | ((uint8_t)val << 4);
    return lsm6ds3_write_reg(ctx, LSM6DS3_CTRL2_G, &reg, 1);
}

int32_t lsm6ds3_gy_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t *val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, &reg, 1)) {
        return -1;
    }
    *val = (lsm6ds3_odr_g_t)((reg & 0xF0) >> 4);
    return 0;
}

int32_t lsm6ds3_gy_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, &reg, 1)) {
        return -1;
    }
    reg = (reg & ~0x07) | (uint8_t)val;
    return lsm6ds3_write_reg(ctx, LSM6DS3_CTRL2_G, &reg, 1);
}

int32_t lsm6ds3_gy_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t *val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, &reg, 1)) {
        return -1;
    }
    *val = (lsm6ds3_fs_g_t)(reg & 0x07);
    return 0;
}

int32_t lsm6ds3_status_reg_get(lsm6ds3_ctx_t *ctx, lsm6ds3_status_reg_t *val)
{
    uint8_t reg;
    if (lsm6ds3_read_reg(ctx, LSM6DS3_STATUS_REG, &reg, 1)) {
        return -1;
    }
    val->xlda = (reg >> 0) & 0x01;
    val->gda = (reg >> 1) & 0x01;
    val->tda = (reg >> 2) & 0x01;
    val->xlda_gy = (reg >> 4) & 0x01;
    val->gda_gy = (reg >> 5) & 0x01;
    val->den_lh = (reg >> 7) & 0x01;
    return 0;
}

int32_t lsm6ds3_acceleration_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
    return lsm6ds3_read_reg(ctx, LSM6DS3_OUTX_L_XL, buff, 6);
}

int32_t lsm6ds3_angular_rate_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
    return lsm6ds3_read_reg(ctx, LSM6DS3_OUTX_L_G, buff, 6);
}

float lsm6ds3_from_fs2g_to_mg(int16_t lsb)
{
    return ((float)lsb * 0.061f);
}

float lsm6ds3_from_fs4g_to_mg(int16_t lsb)
{
    return ((float)lsb * 0.122f);
}

float lsm6ds3_from_fs8g_to_mg(int16_t lsb)
{
    return ((float)lsb * 0.244f);
}

float lsm6ds3_from_fs16g_to_mg(int16_t lsb)
{
    return ((float)lsb * 0.488f);
}

float lsm6ds3_from_fs125dps_to_mdps(int16_t lsb)
{
    return ((float)lsb * 4.375f);
}

float lsm6ds3_from_fs250dps_to_mdps(int16_t lsb)
{
    return ((float)lsb * 8.75f);
}

float lsm6ds3_from_fs500dps_to_mdps(int16_t lsb)
{
    return ((float)lsb * 17.50f);
}

float lsm6ds3_from_fs1000dps_to_mdps(int16_t lsb)
{
    return ((float)lsb * 35.0f);
}

float lsm6ds3_from_fs2000dps_to_mdps(int16_t lsb)
{
    return ((float)lsb * 70.0f);
}

int32_t lsm6ds3_xl_usr_offset_x_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
    return lsm6ds3_write_reg(ctx, LSM6DS3_X_OFS_USR, &val, 1);
}

int32_t lsm6ds3_xl_usr_offset_x_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return lsm6ds3_read_reg(ctx, LSM6DS3_X_OFS_USR, val, 1);
}

int32_t lsm6ds3_xl_usr_offset_y_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
    return lsm6ds3_write_reg(ctx, LSM6DS3_Y_OFS_USR, &val, 1);
}

int32_t lsm6ds3_xl_usr_offset_y_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return lsm6ds3_read_reg(ctx, LSM6DS3_Y_OFS_USR, val, 1);
}

int32_t lsm6ds3_xl_usr_offset_z_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
    return lsm6ds3_write_reg(ctx, LSM6DS3_Z_OFS_USR, &val, 1);
}

int32_t lsm6ds3_xl_usr_offset_z_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return lsm6ds3_read_reg(ctx, LSM6DS3_Z_OFS_USR, val, 1);
}

int32_t lsm6ds3_gy_usr_offset_x_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
    return -1;
}

int32_t lsm6ds3_gy_usr_offset_x_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return -1;
}

int32_t lsm6ds3_gy_usr_offset_y_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
    return -1;
}

int32_t lsm6ds3_gy_usr_offset_y_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return -1;
}

int32_t lsm6ds3_gy_usr_offset_z_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
    return -1;
}

int32_t lsm6ds3_gy_usr_offset_z_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
    return -1;
}

