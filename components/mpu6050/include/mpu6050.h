/**
 * @file mpu6050.h
 * @brief MPU6050 ESP-IDF Driver Header File
 * 
 * This file contains the definitions, structures, and function prototypes for the MPU6050 driver.
 */

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

#pragma once
#include <esp_err.h>
#include <driver/i2c_master.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPU6050_DEFAULT_ADDR 0x68
#define MPU6050_DEFAULT_CLOCK 100000
#define MPU6050_DEFAULT_INFO() {\
    .address = MPU6050_DEFAULT_ADDR,\
    .clock_speed = MPU6050_DEFAULT_CLOCK\
}
#define MPU6050_DEFAULT_CONFIG() {\
    .accel_fs = ACCEL_FS_4G,\
    .gyro_fs = GYRO_FS_500DPS\
}

/**
 * @brief MPU6050 accelerometer full-scale range options.
 */
typedef enum {
    ACCEL_FS_2G  = 0,     /*!< Accelerometer full scale range is +/- 2g */
    ACCEL_FS_4G  = 1,     /*!< Accelerometer full scale range is +/- 4g */
    ACCEL_FS_8G  = 2,     /*!< Accelerometer full scale range is +/- 8g */
    ACCEL_FS_16G = 3,     /*!< Accelerometer full scale range is +/- 16g */
} mpu6050_accel_fs_t;

/**
 * @brief MPU6050 gyroscope full-scale range options.
 */
typedef enum {
    GYRO_FS_250DPS  = 0,     /*!< Gyroscope full scale range is +/- 250 degrees per second */
    GYRO_FS_500DPS  = 1,     /*!< Gyroscope full scale range is +/- 500 degrees per second */
    GYRO_FS_1000DPS = 2,     /*!< Gyroscope full scale range is +/- 1000 degrees per second */
    GYRO_FS_2000DPS = 3,     /*!< Gyroscope full scale range is +/- 2000 degrees per second */
} mpu6050_gyro_fs_t;

/**
 * @brief MPU6050 device information structure.
 */
typedef struct {
    uint16_t address;         /*!< I2C address of the MPU6050 device */
    uint32_t clock_speed;     /*!< I2C clock speed in Hz */
} mpu6050_info_t;

/**
 * @brief MPU6050 configuration structure.
 */
typedef struct {
    mpu6050_accel_fs_t accel_fs; /*!< Accelerometer full-scale range */
    mpu6050_gyro_fs_t gyro_fs; /*!< Gyroscope full-scale range */
} mpu6050_config_t;

/**
 * @brief MPU6050 raw accelerometer values.
 */
typedef struct {
    int16_t raw_accel_x; /*!< Raw accelerometer X-axis value */
    int16_t raw_accel_y; /*!< Raw accelerometer Y-axis value */
    int16_t raw_accel_z; /*!< Raw accelerometer Z-axis value */
} mpu6050_raw_accel_value_t;

/**
 * @brief MPU6050 raw gyroscope values.
 */
typedef struct {
    int16_t raw_gyro_x; /*!< Raw gyroscope X-axis value */
    int16_t raw_gyro_y; /*!< Raw gyroscope Y-axis value */
    int16_t raw_gyro_z; /*!< Raw gyroscope Z-axis value */
} mpu6050_raw_gyro_value_t;

/**
 * @brief MPU6050 accelerometer values in g.
 */
typedef struct {
    float accel_x; /*!< Accelerometer X-axis value in g */
    float accel_y; /*!< Accelerometer Y-axis value in g */
    float accel_z; /*!< Accelerometer Z-axis value in g */
} mpu6050_accel_value_t;

/**
 * @brief MPU6050 gyroscope values in degrees per second.
 */
typedef struct {
    float gyro_x; /*!< Gyroscope X-axis value in degrees per second */
    float gyro_y; /*!< Gyroscope Y-axis value in degrees per second */
    float gyro_z; /*!< Gyroscope Z-axis value in degrees per second */
} mpu6050_gyro_value_t;

/**
 * @brief MPU6050 temperature value in degrees Celsius.
 */
typedef struct {
    float temp; /*!< Temperature value in degrees Celsius */
} mpu6050_temp_value_t;

/**
 * @brief MPU6050 device handle structure.
 */
typedef struct {
    i2c_master_dev_handle_t dev_handle; /*!< I2C device handle for MPU6050 */
} mpu6050_handle_t;

/**
 * @brief Create an MPU6050 device instance.
 * 
 * @param i2c_bus_handle Handle to the I2C bus.
 * @param info MPU6050 device information.
 * @param out_mpu_handle Pointer to the created MPU6050 handle.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_create(i2c_master_bus_handle_t i2c_bus_handle, mpu6050_info_t info, mpu6050_handle_t* out_mpu_handle);

/**
 * @brief Configure the MPU6050 device.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @param config Configuration parameters for the MPU6050.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_config(mpu6050_handle_t mpu_handle, mpu6050_config_t config);

/**
 * @brief Reset the MPU6050 device.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_reset(mpu6050_handle_t mpu_handle);

/**
 * @brief Wake up the MPU6050 device from sleep mode.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_wake_up(mpu6050_handle_t mpu_handle);

/**
 * @brief Put the MPU6050 device into sleep mode.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_sleep(mpu6050_handle_t mpu_handle);

/**
 * @brief Get raw accelerometer values from the MPU6050.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @param raw_accel_value Pointer to store raw accelerometer values.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_get_raw_accel(mpu6050_handle_t mpu_handle, mpu6050_raw_accel_value_t *const raw_accel_value);

/**
 * @brief Get raw gyroscope values from the MPU6050.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @param raw_gyro_value Pointer to store raw gyroscope values.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_get_raw_gyro(mpu6050_handle_t mpu_handle, mpu6050_raw_gyro_value_t *const raw_gyro_value);

/**
 * @brief Get accelerometer values in g from the MPU6050.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @param accel_value Pointer to store accelerometer values in g.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_get_accel(mpu6050_handle_t mpu_handle, mpu6050_accel_value_t *const accel_value);

/**
 * @brief Get gyroscope values in degrees per second from the MPU6050.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @param gyro_value Pointer to store gyroscope values in degrees per second.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_get_gyro(mpu6050_handle_t mpu_handle, mpu6050_gyro_value_t *const gyro_value);

/**
 * @brief Get the temperature value in degrees Celsius from the MPU6050.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @param out_temp Pointer to store the temperature value in degrees Celsius.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_get_temp(mpu6050_handle_t mpu_handle, mpu6050_temp_value_t* out_temp);

/**
 * @brief Delete the MPU6050 device instance.
 * 
 * @param mpu_handle MPU6050 device handle.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mpu6050_delete(mpu6050_handle_t mpu_handle);

#ifdef __cplusplus
}
#endif