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

#ifndef LSM6DS3_FUSION_H
#define LSM6DS3_FUSION_H

#include "lsm6ds3.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float roll;
    float pitch;
    float yaw;
} lsm6ds3_euler_angles_t;

typedef struct {
    float roll_zero;
    float pitch_zero;
    float yaw_zero;
    bool roll_zero_set;
    bool pitch_zero_set;
    bool yaw_zero_set;
} lsm6ds3_angle_zero_t;

typedef struct {
    float q0;
    float q1;
    float q2;
    float q3;
} lsm6ds3_quaternion_t;

typedef struct {
    float beta;
    float sample_rate_hz;
    lsm6ds3_quaternion_t quaternion;
    float accel[3];
    float gyro[3];
    bool initialized;
} lsm6ds3_madgwick_filter_t;

typedef struct {
    float alpha;
    float sample_rate_hz;
    float roll;
    float pitch;
    bool initialized;
} lsm6ds3_complementary_filter_t;

esp_err_t lsm6ds3_madgwick_init(lsm6ds3_madgwick_filter_t *filter, float beta, float sample_rate_hz);
esp_err_t lsm6ds3_madgwick_update(lsm6ds3_madgwick_filter_t *filter, const float accel[3], const float gyro[3], float dt);
esp_err_t lsm6ds3_madgwick_get_euler(lsm6ds3_madgwick_filter_t *filter, lsm6ds3_euler_angles_t *angles);

esp_err_t lsm6ds3_complementary_init(lsm6ds3_complementary_filter_t *filter, float alpha, float sample_rate_hz);
esp_err_t lsm6ds3_complementary_update(lsm6ds3_complementary_filter_t *filter, const float accel[3], const float gyro[3], float dt);
esp_err_t lsm6ds3_complementary_get_angles(lsm6ds3_complementary_filter_t *filter, float *roll, float *pitch);
esp_err_t lsm6ds3_set_angle_zero(lsm6ds3_angle_zero_t *zero_ref, float roll, float pitch, float yaw);
esp_err_t lsm6ds3_set_roll_zero(lsm6ds3_angle_zero_t *zero_ref, float roll);
esp_err_t lsm6ds3_set_pitch_zero(lsm6ds3_angle_zero_t *zero_ref, float pitch);
esp_err_t lsm6ds3_set_yaw_zero(lsm6ds3_angle_zero_t *zero_ref, float yaw);
esp_err_t lsm6ds3_apply_angle_zero(const lsm6ds3_angle_zero_t *zero_ref, float *roll, float *pitch, float *yaw);
esp_err_t lsm6ds3_clear_angle_zero(lsm6ds3_angle_zero_t *zero_ref);
esp_err_t lsm6ds3_save_angle_zero_to_nvs(const lsm6ds3_angle_zero_t *zero_ref, const char *namespace);
esp_err_t lsm6ds3_load_angle_zero_from_nvs(lsm6ds3_angle_zero_t *zero_ref, const char *namespace);
float lsm6ds3_calculate_angle_from_vertical(float roll_deg, float pitch_deg);

#ifdef __cplusplus
}
#endif

#endif

