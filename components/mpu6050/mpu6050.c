/*
 * MPU6050 ESP-IDF Driver by TNY Robotics
 *
 * SPDX-FileCopyrightText: 2025 TNY Robotics
 * SPDX-License-Identifier: MIT
 * 
 * 
 * Copyright (C) 2025 TNY Robotics
 * 
 * This file is part of the MPU6050 ESP-IDF Driver.
 * 
 * License: MIT
 * Repository: https://github.com/tny-robotics/mpu6050-esp-idf
 * 
 * Author: TNY Robotics
 * Date: 04/10/2025
 * Version: 1.0
 */

#include "mpu6050.h"
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <esp_check.h>

/* MPU6050 register */
#define MPU6050_GYRO_CONFIG         0x1Bu
#define MPU6050_ACCEL_CONFIG        0x1Cu
#define MPU6050_INTR_PIN_CFG         0x37u
#define MPU6050_INTR_ENABLE          0x38u
#define MPU6050_INTR_STATUS          0x3Au
#define MPU6050_ACCEL_XOUT_H        0x3Bu
#define MPU6050_GYRO_XOUT_H         0x43u
#define MPU6050_TEMP_XOUT_H         0x41u
#define MPU6050_PWR_MGMT_1          0x6Bu
#define MPU6050_WHO_AM_I            0x75u

static esp_err_t mpu6050_write(mpu6050_handle_t mpu, const uint8_t reg_start_addr, const uint8_t *const data_buf, const uint8_t data_len)
{
    uint8_t tmp[data_len+1];
    tmp[0] = reg_start_addr;
    for (uint8_t i = 0; i < data_len; i++)
        tmp[i+1] = data_buf[i];
    return i2c_master_transmit(mpu.dev_handle, tmp, data_len+1, -1);
}

static esp_err_t mpu6050_read(mpu6050_handle_t mpu, const uint8_t reg_start_addr, uint8_t *const data_buf, const uint8_t data_len)
{
    return i2c_master_transmit_receive(mpu.dev_handle, &reg_start_addr, 1, data_buf, data_len, -1);
}

static float mpu6050_accel_sensitivity(uint8_t accel_fs)
{
    switch (accel_fs)
    {
        case ACCEL_FS_2G:  return 16384;
        case ACCEL_FS_4G:  return 8192;
        case ACCEL_FS_8G:  return 4096;
        case ACCEL_FS_16G: return 2048;
        default: break;
    }
    return 0.0f;
}

static esp_err_t mpu6050_get_accel_sensitivity(mpu6050_handle_t sensor, float *const accel_sensitivity)
{
    esp_err_t ret;
    uint8_t accel_fs;
    ret = mpu6050_read(sensor, MPU6050_ACCEL_CONFIG, &accel_fs, 1);
    accel_fs = (accel_fs >> 3) & 0x03;

    *accel_sensitivity = mpu6050_accel_sensitivity(accel_fs);
    return ret;
}

static float mpu6050_gyro_sensitivity(uint8_t gyro_fs)
{
    switch (gyro_fs)
    {
        case GYRO_FS_250DPS:  return 131;
        case GYRO_FS_500DPS:  return 65.5;
        case GYRO_FS_1000DPS: return 32.8;
        case GYRO_FS_2000DPS: return 16.4;
        default: break;
    }
    return 0.0f;
}

static esp_err_t mpu6050_get_gyro_sensitivity(mpu6050_handle_t sensor, float *const gyro_sensitivity)
{
    esp_err_t ret;
    uint8_t gyro_fs;
    ret = mpu6050_read(sensor, MPU6050_GYRO_CONFIG, &gyro_fs, 1);
    gyro_fs = (gyro_fs >> 3) & 0x03;

    *gyro_sensitivity = mpu6050_gyro_sensitivity(gyro_fs);
    return ret;
}

esp_err_t mpu6050_create(i2c_master_bus_handle_t i2c_bus_handle, mpu6050_info_t info, mpu6050_handle_t* out_mpu_handle)
{
    if (out_mpu_handle == NULL)
        return ESP_ERR_NO_MEM;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = info.address,
        .scl_speed_hz = info.clock_speed,
    };

    return i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &out_mpu_handle->dev_handle);
}

esp_err_t mpu6050_config(mpu6050_handle_t mpu_handle, mpu6050_config_t config)
{
    uint8_t config_regs[2] = {config.gyro_fs << 3,  config.accel_fs << 3};
    return mpu6050_write(mpu_handle, MPU6050_GYRO_CONFIG, config_regs, sizeof(config_regs));
}

esp_err_t mpu6050_reset(mpu6050_handle_t mpu_handle)
{
    uint8_t reset = 0x00;
    return mpu6050_write(mpu_handle, MPU6050_PWR_MGMT_1, &reset, 1);
}

esp_err_t mpu6050_wake_up(mpu6050_handle_t mpu_handle)
{
    esp_err_t ret;
    uint8_t tmp;
    ret = mpu6050_read(mpu_handle, MPU6050_PWR_MGMT_1, &tmp, 1);
    if (ESP_OK != ret) {
        return ret;
    }

    tmp &= (~BIT6);
    ret = mpu6050_write(mpu_handle, MPU6050_PWR_MGMT_1, &tmp, 1);
    return ret;
}

esp_err_t mpu6050_sleep(mpu6050_handle_t mpu_handle)
{
    esp_err_t ret;
    uint8_t tmp;
    ret = mpu6050_read(mpu_handle, MPU6050_PWR_MGMT_1, &tmp, 1);
    if (ESP_OK != ret) {
        return ret;
    }
    tmp |= BIT6;
    ret = mpu6050_write(mpu_handle, MPU6050_PWR_MGMT_1, &tmp, 1);
    return ret;
}

esp_err_t mpu6050_get_raw_accel(mpu6050_handle_t mpu_handle, mpu6050_raw_accel_value_t *const raw_accel_value)
{
    uint8_t data_rd[6];
    esp_err_t ret = mpu6050_read(mpu_handle, MPU6050_ACCEL_XOUT_H, data_rd, sizeof(data_rd));

    raw_accel_value->raw_accel_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    raw_accel_value->raw_accel_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    raw_accel_value->raw_accel_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
    return ret;
}

esp_err_t mpu6050_get_raw_gyro(mpu6050_handle_t mpu_handle, mpu6050_raw_gyro_value_t *const raw_gyro_value)
{
    uint8_t data_rd[6];
    esp_err_t ret = mpu6050_read(mpu_handle, MPU6050_GYRO_XOUT_H, data_rd, sizeof(data_rd));

    raw_gyro_value->raw_gyro_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    raw_gyro_value->raw_gyro_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    raw_gyro_value->raw_gyro_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

    return ret;
}

esp_err_t mpu6050_get_accel(mpu6050_handle_t mpu_handle, mpu6050_accel_value_t *const accel_value)
{
    esp_err_t ret;
    float accel_sensitivity;
    mpu6050_raw_accel_value_t raw_acce;

    ret = mpu6050_get_accel_sensitivity(mpu_handle, &accel_sensitivity);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = mpu6050_get_raw_accel(mpu_handle, &raw_acce);
    if (ret != ESP_OK) {
        return ret;
    }

    accel_value->accel_x = raw_acce.raw_accel_x / accel_sensitivity;
    accel_value->accel_y = raw_acce.raw_accel_y / accel_sensitivity;
    accel_value->accel_z = raw_acce.raw_accel_z / accel_sensitivity;
    return ESP_OK;
}

esp_err_t mpu6050_get_gyro(mpu6050_handle_t mpu_handle, mpu6050_gyro_value_t *const gyro_value)
{
    esp_err_t ret;
    float gyro_sensitivity;
    mpu6050_raw_gyro_value_t raw_gyro;

    ret = mpu6050_get_gyro_sensitivity(mpu_handle, &gyro_sensitivity);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = mpu6050_get_raw_gyro(mpu_handle, &raw_gyro);
    if (ret != ESP_OK) {
        return ret;
    }

    gyro_value->gyro_x = raw_gyro.raw_gyro_x / gyro_sensitivity;
    gyro_value->gyro_y = raw_gyro.raw_gyro_y / gyro_sensitivity;
    gyro_value->gyro_z = raw_gyro.raw_gyro_z / gyro_sensitivity;
    return ESP_OK;
}

esp_err_t mpu6050_get_temp(mpu6050_handle_t mpu_handle, mpu6050_temp_value_t* out_temp)
{
    uint8_t data_rd[2];
    esp_err_t ret = mpu6050_read(mpu_handle, MPU6050_TEMP_XOUT_H, data_rd, sizeof(data_rd));
    out_temp->temp = (int16_t)((data_rd[0] << 8) | (data_rd[1])) / 340.00 + 36.53;
    return ret;
}

esp_err_t mpu6050_delete(mpu6050_handle_t mpu_handle)
{
    i2c_master_bus_rm_device(mpu_handle.dev_handle);
    return ESP_OK;
}