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

#include "lsm6ds3_fusion.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define RAD_TO_DEG (180.0f / PI)
#define DEG_TO_RAD (PI / 180.0f)

static void madgwick_ahrs_update(lsm6ds3_madgwick_filter_t *filter, float gx, float gy, float gz, float ax, float ay, float az, float dt)
{
    float q0 = filter->quaternion.q0;
    float q1 = filter->quaternion.q1;
    float q2 = filter->quaternion.q2;
    float q3 = filter->quaternion.q3;
    
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;
    
    if (!filter->initialized) {
        float norm = sqrtf(ax * ax + ay * ay + az * az);
        if (norm > 0.0f) {
            ax /= norm;
            ay /= norm;
            az /= norm;
            
            float roll = atan2f(ay, az);
            float pitch = asinf(-ax);
            
            float cy = cosf(roll * 0.5f);
            float sy = sinf(roll * 0.5f);
            float cp = cosf(pitch * 0.5f);
            float sp = sinf(pitch * 0.5f);
            
            q0 = cy * cp;
            q1 = cy * sp;
            q2 = sy * cp;
            q3 = sy * sp;
            
            filter->quaternion.q0 = q0;
            filter->quaternion.q1 = q1;
            filter->quaternion.q2 = q2;
            filter->quaternion.q3 = q3;
            filter->initialized = true;
        }
        return;
    }
    
    gx *= DEG_TO_RAD;
    gy *= DEG_TO_RAD;
    gz *= DEG_TO_RAD;
    
    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);
    
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
        recipNorm = 1.0f / sqrtf(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;
        
        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        _4q0 = 4.0f * q0;
        _4q1 = 4.0f * q1;
        _4q2 = 4.0f * q2;
        _8q1 = 8.0f * q1;
        _8q2 = 8.0f * q2;
        q0q0 = q0 * q0;
        q1q1 = q1 * q1;
        q2q2 = q2 * q2;
        q3q3 = q3 * q3;
        
        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
        s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
        s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
        s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
        
        float s_norm = sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        if (s_norm > 0.0f) {
            recipNorm = 1.0f / s_norm;
            s0 *= recipNorm;
            s1 *= recipNorm;
            s2 *= recipNorm;
            s3 *= recipNorm;
        }
        
        qDot1 -= filter->beta * s0;
        qDot2 -= filter->beta * s1;
        qDot3 -= filter->beta * s2;
        qDot4 -= filter->beta * s3;
    }
    
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;
    
    float q_norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (q_norm > 0.0f) {
        recipNorm = 1.0f / q_norm;
        q0 *= recipNorm;
        q1 *= recipNorm;
        q2 *= recipNorm;
        q3 *= recipNorm;
    }
    
    filter->quaternion.q0 = q0;
    filter->quaternion.q1 = q1;
    filter->quaternion.q2 = q2;
    filter->quaternion.q3 = q3;
}

esp_err_t lsm6ds3_madgwick_init(lsm6ds3_madgwick_filter_t *filter, float beta, float sample_rate_hz)
{
    if (filter == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate beta parameter (typically 0.01 to 0.2)
    if (beta < 0.0f || beta > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate sample rate
    if (sample_rate_hz <= 0.0f || sample_rate_hz > 10000.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(filter, 0, sizeof(lsm6ds3_madgwick_filter_t));
    filter->beta = beta;
    filter->sample_rate_hz = sample_rate_hz;
    filter->quaternion.q0 = 1.0f;
    filter->initialized = false;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_madgwick_update(lsm6ds3_madgwick_filter_t *filter, const float accel[3], const float gyro[3], float dt)
{
    if (filter == NULL || accel == NULL || gyro == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate dt to prevent division by zero or invalid updates
    if (dt <= 0.0f || dt > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(filter->accel, accel, sizeof(float) * 3);
    memcpy(filter->gyro, gyro, sizeof(float) * 3);
    
    madgwick_ahrs_update(filter, gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2], dt);
    
    return ESP_OK;
}

esp_err_t lsm6ds3_madgwick_get_euler(lsm6ds3_madgwick_filter_t *filter, lsm6ds3_euler_angles_t *angles)
{
    if (filter == NULL || angles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    float q0 = filter->quaternion.q0;
    float q1 = filter->quaternion.q1;
    float q2 = filter->quaternion.q2;
    float q3 = filter->quaternion.q3;
    
    angles->roll = atan2f(q0 * q1 + q2 * q3, 0.5f - q1 * q1 - q2 * q2) * RAD_TO_DEG;
    
    // Clamp argument to [-1, 1] to prevent NaN from asinf()
    float pitch_arg = 2.0f * (q0 * q2 - q3 * q1);
    pitch_arg = fmaxf(-1.0f, fminf(1.0f, pitch_arg));
    angles->pitch = asinf(pitch_arg) * RAD_TO_DEG;
    
    angles->yaw = atan2f(q0 * q3 + q1 * q2, 0.5f - q2 * q2 - q3 * q3) * RAD_TO_DEG;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_complementary_init(lsm6ds3_complementary_filter_t *filter, float alpha, float sample_rate_hz)
{
    if (filter == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate alpha parameter (typically 0.9 to 0.99)
    if (alpha < 0.0f || alpha > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate sample rate
    if (sample_rate_hz <= 0.0f || sample_rate_hz > 10000.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(filter, 0, sizeof(lsm6ds3_complementary_filter_t));
    filter->alpha = alpha;
    filter->sample_rate_hz = sample_rate_hz;
    filter->initialized = false;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_complementary_update(lsm6ds3_complementary_filter_t *filter, const float accel[3], const float gyro[3], float dt)
{
    if (filter == NULL || accel == NULL || gyro == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate dt to prevent invalid updates
    if (dt <= 0.0f || dt > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    
    float accel_norm = sqrtf(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);
    
    if (accel_norm > 0.0f) {
        float accel_x = accel[0] / accel_norm;
        float accel_y = accel[1] / accel_norm;
        float accel_z = accel[2] / accel_norm;
        
        float accel_roll = atan2f(accel_y, accel_z) * RAD_TO_DEG;
        float accel_pitch = asinf(-accel_x) * RAD_TO_DEG;
        
        if (!filter->initialized) {
            filter->roll = accel_roll;
            filter->pitch = accel_pitch;
            filter->initialized = true;
        } else {
            filter->roll = filter->alpha * (filter->roll + gyro[0] * dt) + (1.0f - filter->alpha) * accel_roll;
            filter->pitch = filter->alpha * (filter->pitch + gyro[1] * dt) + (1.0f - filter->alpha) * accel_pitch;
        }
    } else {
        if (filter->initialized) {
            filter->roll += gyro[0] * dt;
            filter->pitch += gyro[1] * dt;
        }
    }
    
    return ESP_OK;
}

esp_err_t lsm6ds3_complementary_get_angles(lsm6ds3_complementary_filter_t *filter, float *roll, float *pitch)
{
    if (filter == NULL || roll == NULL || pitch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *roll = filter->roll;
    *pitch = filter->pitch;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_set_angle_zero(lsm6ds3_angle_zero_t *zero_ref, float roll, float pitch, float yaw)
{
    if (zero_ref == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    zero_ref->roll_zero = roll;
    zero_ref->pitch_zero = pitch;
    zero_ref->yaw_zero = yaw;
    zero_ref->roll_zero_set = true;
    zero_ref->pitch_zero_set = true;
    zero_ref->yaw_zero_set = true;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_set_roll_zero(lsm6ds3_angle_zero_t *zero_ref, float roll)
{
    if (zero_ref == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    zero_ref->roll_zero = roll;
    zero_ref->roll_zero_set = true;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_set_pitch_zero(lsm6ds3_angle_zero_t *zero_ref, float pitch)
{
    if (zero_ref == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    zero_ref->pitch_zero = pitch;
    zero_ref->pitch_zero_set = true;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_set_yaw_zero(lsm6ds3_angle_zero_t *zero_ref, float yaw)
{
    if (zero_ref == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    zero_ref->yaw_zero = yaw;
    zero_ref->yaw_zero_set = true;
    
    return ESP_OK;
}

esp_err_t lsm6ds3_apply_angle_zero(const lsm6ds3_angle_zero_t *zero_ref, float *roll, float *pitch, float *yaw)
{
    if (zero_ref == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (roll != NULL && zero_ref->roll_zero_set) {
        *roll -= zero_ref->roll_zero;
    }
    
    if (pitch != NULL && zero_ref->pitch_zero_set) {
        *pitch -= zero_ref->pitch_zero;
    }
    
    if (yaw != NULL && zero_ref->yaw_zero_set) {
        *yaw -= zero_ref->yaw_zero;
    }
    
    return ESP_OK;
}

esp_err_t lsm6ds3_clear_angle_zero(lsm6ds3_angle_zero_t *zero_ref)
{
    if (zero_ref == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(zero_ref, 0, sizeof(lsm6ds3_angle_zero_t));
    
    return ESP_OK;
}

esp_err_t lsm6ds3_save_angle_zero_to_nvs(const lsm6ds3_angle_zero_t *zero_ref, const char *namespace)
{
    if (zero_ref == NULL || namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }
    
    err = nvs_set_blob(nvs_handle, "angle_zero", zero_ref, sizeof(lsm6ds3_angle_zero_t));
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }
    
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    return err;
}

esp_err_t lsm6ds3_load_angle_zero_from_nvs(lsm6ds3_angle_zero_t *zero_ref, const char *namespace)
{
    if (zero_ref == NULL || namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }
    
    size_t required_size = sizeof(lsm6ds3_angle_zero_t);
    size_t size = required_size;
    
    err = nvs_get_blob(nvs_handle, "angle_zero", zero_ref, &size);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK && size == required_size) {
        return ESP_OK;
    }
    
    return err;
}

float lsm6ds3_calculate_angle_from_vertical(float roll_deg, float pitch_deg)
{
    float roll_rad = roll_deg * DEG_TO_RAD;
    float pitch_rad = pitch_deg * DEG_TO_RAD;
    
    float cos_roll = cosf(roll_rad);
    float cos_pitch = cosf(pitch_rad);
    float cos_angle = cos_roll * cos_pitch;
    
    cos_angle = fmaxf(-1.0f, fminf(1.0f, cos_angle));
    
    float angle_rad = acosf(cos_angle);
    float angle_deg = angle_rad * RAD_TO_DEG;
    
    return angle_deg;
}

