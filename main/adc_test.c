/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include <esp_timer.h>
#include <sys/_types.h>

const static char *TAG = "EXAMPLE";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
static adc_channel_t button_adc_channels[5] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_5};
uint8_t channels_num = sizeof(button_adc_channels) / sizeof(adc_channel_t);
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

static int button_array_adc[10];

void app_main(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
	for(int i = 0; i < channels_num; i ++){
		ESP_LOGI(TAG, "CONFIG %d", i);
		ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, button_adc_channels[i], &config));
	}

    while (1) {
		int64_t timeB4 = esp_timer_get_time();
		for(int i = 0; i < channels_num; i ++){
			ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, button_adc_channels[i], &(button_array_adc[i])) );
		}
		int64_t timeAF = esp_timer_get_time();
		ESP_LOGI(TAG, "%d %d %d %d %d %d %d %d %d %d T:%ldus",
		    button_array_adc[0],
		    button_array_adc[1],
		    button_array_adc[2],
		    button_array_adc[3],
		    button_array_adc[4],
		    button_array_adc[5],
		    button_array_adc[6],
		    button_array_adc[7],
		    button_array_adc[8],
		    button_array_adc[9],
			timeAF - timeB4
		);  
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}