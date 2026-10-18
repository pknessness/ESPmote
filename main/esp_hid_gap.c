/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_hid_gap.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "host/ble_gap.h"
#include "host/ble_hs_adv.h"
#include "nimble/ble.h"
#include "host/ble_sm.h"
#else
#include "esp_bt_device.h"
#endif

static const char *TAG = "ESP_HID_GAP";

// uncomment to print all devices that were seen during a scan
#define GAP_DBG_PRINTF(...) //printf(__VA_ARGS__)
//static const char * gap_bt_prop_type_names[5] = {"","BDNAME","COD","RSSI","EIR"};

#if !CONFIG_BT_NIMBLE_ENABLED
static esp_hid_scan_result_t *bt_scan_results = NULL;
static size_t num_bt_scan_results = 0;

static esp_hid_scan_result_t *ble_scan_results = NULL;
static size_t num_ble_scan_results = 0;
#endif

static SemaphoreHandle_t bt_hidh_cb_semaphore = NULL;
#define WAIT_BT_CB() xSemaphoreTake(bt_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BT_CB() xSemaphoreGive(bt_hidh_cb_semaphore)

static SemaphoreHandle_t ble_hidh_cb_semaphore = NULL;
#define WAIT_BLE_CB() xSemaphoreTake(ble_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BLE_CB() xSemaphoreGive(ble_hidh_cb_semaphore)

#define SIZEOF_ARRAY(a) (sizeof(a)/sizeof(*a))

#if !CONFIG_BT_NIMBLE_ENABLED
static const char *ble_gap_evt_names[] = { "ADV_DATA_SET_COMPLETE", "SCAN_RSP_DATA_SET_COMPLETE", "SCAN_PARAM_SET_COMPLETE", "SCAN_RESULT", "ADV_DATA_RAW_SET_COMPLETE", "SCAN_RSP_DATA_RAW_SET_COMPLETE", "ADV_START_COMPLETE", "SCAN_START_COMPLETE", "AUTH_CMPL", "KEY", "SEC_REQ", "PASSKEY_NOTIF", "PASSKEY_REQ", "OOB_REQ", "LOCAL_IR", "LOCAL_ER", "NC_REQ", "ADV_STOP_COMPLETE", "SCAN_STOP_COMPLETE", "SET_STATIC_RAND_ADDR", "UPDATE_CONN_PARAMS", "SET_PKT_LENGTH_COMPLETE", "SET_LOCAL_PRIVACY_COMPLETE", "REMOVE_BOND_DEV_COMPLETE", "CLEAR_BOND_DEV_COMPLETE", "GET_BOND_DEV_COMPLETE", "READ_RSSI_COMPLETE", "UPDATE_WHITELIST_COMPLETE"};
static const char *bt_gap_evt_names[] = { "DISC_RES", "DISC_STATE_CHANGED", "RMT_SRVCS", "RMT_SRVC_REC", "AUTH_CMPL", "PIN_REQ", "CFM_REQ", "KEY_NOTIF", "KEY_REQ", "READ_RSSI_DELTA"};
static const char *ble_addr_type_names[] = {"PUBLIC", "RANDOM", "RPA_PUBLIC", "RPA_RANDOM"};

const char *ble_addr_type_str(esp_ble_addr_type_t ble_addr_type)
{
    if (ble_addr_type > BLE_ADDR_TYPE_RPA_RANDOM) {
        return "UNKNOWN";
    }
    return ble_addr_type_names[ble_addr_type];
}

const char *ble_gap_evt_str(uint8_t event)
{
    if (event >= SIZEOF_ARRAY(ble_gap_evt_names)) {
        return "UNKNOWN";
    }
    return ble_gap_evt_names[event];
}

const char *bt_gap_evt_str(uint8_t event)
{
    if (event >= SIZEOF_ARRAY(bt_gap_evt_names)) {
        return "UNKNOWN";
    }
    return bt_gap_evt_names[event];
}
#endif

#if !CONFIG_BT_NIMBLE_ENABLED
void esp_hid_scan_results_free(esp_hid_scan_result_t *results)
{
    esp_hid_scan_result_t *r = NULL;
    while (results) {
        r = results;
        results = results->next;
        if (r->name != NULL) {
            free((char *)r->name);
        }
        free(r);
    }
}
#endif

#if (CONFIG_BT_HID_DEVICE_ENABLED || CONFIG_BT_BLE_ENABLED)
static esp_hid_scan_result_t *find_scan_result(esp_bd_addr_t bda, esp_hid_scan_result_t *results)
{
    esp_hid_scan_result_t *r = results;
    while (r) {
        if (memcmp(bda, r->bda, sizeof(esp_bd_addr_t)) == 0) {
            return r;
        }
        r = r->next;
    }
    return NULL;
}
#endif /* (CONFIG_BT_HID_DEVICE_ENABLED || CONFIG_BT_BLE_ENABLED) */

#if CONFIG_BT_HID_DEVICE_ENABLED
static void add_bt_scan_result(esp_bd_addr_t bda, esp_bt_cod_t *cod, esp_bt_uuid_t *uuid, uint8_t *name, uint8_t name_len, int rssi)
{
    esp_hid_scan_result_t *r = find_scan_result(bda, bt_scan_results);
    if (r) {
        //Some info may come later
        if (r->name == NULL && name && name_len) {
            char *name_s = (char *)malloc(name_len + 1);
            if (name_s == NULL) {
                ESP_LOGE(TAG, "Malloc result name failed!");
                return;
            }
            memcpy(name_s, name, name_len);
            name_s[name_len] = 0;
            r->name = (const char *)name_s;
        }
        if (r->bt.uuid.len == 0 && uuid->len) {
            memcpy(&r->bt.uuid, uuid, sizeof(esp_bt_uuid_t));
        }
        if (rssi != 0) {
            r->rssi = rssi;
        }
        return;
    }

    r = (esp_hid_scan_result_t *)malloc(sizeof(esp_hid_scan_result_t));
    if (r == NULL) {
        ESP_LOGE(TAG, "Malloc bt_hidh_scan_result_t failed!");
        return;
    }
    r->transport = ESP_HID_TRANSPORT_BT;
    memcpy(r->bda, bda, sizeof(esp_bd_addr_t));
    memcpy(&r->bt.cod, cod, sizeof(esp_bt_cod_t));
    memcpy(&r->bt.uuid, uuid, sizeof(esp_bt_uuid_t));
    r->usage = esp_hid_usage_from_cod((uint32_t)cod);
    r->rssi = rssi;
    r->name = NULL;
    if (name_len && name) {
        char *name_s = (char *)malloc(name_len + 1);
        if (name_s == NULL) {
            free(r);
            ESP_LOGE(TAG, "Malloc result name failed!");
            return;
        }
        memcpy(name_s, name, name_len);
        name_s[name_len] = 0;
        r->name = (const char *)name_s;
    }
    r->next = bt_scan_results;
    bt_scan_results = r;
    num_bt_scan_results++;
}
#endif

#if !CONFIG_BT_NIMBLE_ENABLED
void print_uuid(esp_bt_uuid_t *uuid)
{
    if (uuid->len == ESP_UUID_LEN_16) {
        GAP_DBG_PRINTF("UUID16: 0x%04x", uuid->uuid.uuid16);
    } else if (uuid->len == ESP_UUID_LEN_32) {
        GAP_DBG_PRINTF("UUID32: 0x%08x", uuid->uuid.uuid32);
    } else if (uuid->len == ESP_UUID_LEN_128) {
        GAP_DBG_PRINTF("UUID128: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x", uuid->uuid.uuid128[0],
                       uuid->uuid.uuid128[1], uuid->uuid.uuid128[2], uuid->uuid.uuid128[3],
                       uuid->uuid.uuid128[4], uuid->uuid.uuid128[5], uuid->uuid.uuid128[6],
                       uuid->uuid.uuid128[7], uuid->uuid.uuid128[8], uuid->uuid.uuid128[9],
                       uuid->uuid.uuid128[10], uuid->uuid.uuid128[11], uuid->uuid.uuid128[12],
                       uuid->uuid.uuid128[13], uuid->uuid.uuid128[14], uuid->uuid.uuid128[15]);
    }
}
#endif

#if CONFIG_BT_HID_DEVICE_ENABLED
static void handle_bt_device_result(struct disc_res_param *disc_res)
{
    GAP_DBG_PRINTF("BT : " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(disc_res->bda));
    uint32_t codv = 0;
    esp_bt_cod_t *cod = (esp_bt_cod_t *)&codv;
    int8_t rssi = 0;
    uint8_t *name = NULL;
    uint8_t name_len = 0;
    esp_bt_uuid_t uuid;

    uuid.len = ESP_UUID_LEN_16;
    uuid.uuid.uuid16 = 0;

    for (int i = 0; i < disc_res->num_prop; i++) {
        esp_bt_gap_dev_prop_t *prop = &disc_res->prop[i];
        if (prop->type != ESP_BT_GAP_DEV_PROP_EIR) {
            GAP_DBG_PRINTF(", %s: ", gap_bt_prop_type_names[prop->type]);
        }
        if (prop->type == ESP_BT_GAP_DEV_PROP_BDNAME) {
            if (prop->val == NULL) {
                continue;
            }
            name = (uint8_t *)prop->val;
            name_len = strlen((const char *)name);
            GAP_DBG_PRINTF("%s", (const char *)name);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_RSSI) {
            rssi = *((int8_t *)prop->val);
            GAP_DBG_PRINTF("%d", rssi);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_COD) {
            memcpy(&codv, prop->val, sizeof(uint32_t));
            GAP_DBG_PRINTF("major: %s, minor: %d, service: 0x%03x", esp_hid_cod_major_str(cod->major), cod->minor, cod->service);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_EIR) {
            uint8_t len = 0;
            uint8_t *data = 0;

            data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_16BITS_UUID, &len);
            if (data == NULL) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_INCMPL_16BITS_UUID, &len);
            }
            if (data && len == ESP_UUID_LEN_16) {
                uuid.len = ESP_UUID_LEN_16;
                uuid.uuid.uuid16 = data[0] + (data[1] << 8);
                GAP_DBG_PRINTF(", "); print_uuid(&uuid);
                continue;
            }

            data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_32BITS_UUID, &len);
            if (data == NULL) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_INCMPL_32BITS_UUID, &len);
            }
            if (data && len == ESP_UUID_LEN_32) {
                uuid.len = len;
                memcpy(&uuid.uuid.uuid32, data, sizeof(uint32_t));
                GAP_DBG_PRINTF(", "); print_uuid(&uuid);
                continue;
            }

            data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_128BITS_UUID, &len);
            if (data == NULL) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_INCMPL_128BITS_UUID, &len);
            }
            if (data && len == ESP_UUID_LEN_128) {
                uuid.len = len;
                memcpy(uuid.uuid.uuid128, (uint8_t *)data, len);
                GAP_DBG_PRINTF(", "); print_uuid(&uuid);
                continue;
            }

            //try to find a name
            if (name == NULL) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
                if (data == NULL) {
                    data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
                }
                if (data && len) {
                    name = data;
                    name_len = len;
                    GAP_DBG_PRINTF(", NAME: ");
                    for (int x = 0; x < len; x++) {
                        GAP_DBG_PRINTF("%c", (char)data[x]);
                    }
                }
            }
        }
    }
    GAP_DBG_PRINTF("\n");

    if (cod->major == ESP_BT_COD_MAJOR_DEV_PERIPHERAL || (find_scan_result(disc_res->bda, bt_scan_results) != NULL)) {
        add_bt_scan_result(disc_res->bda, cod, &uuid, name, name_len, rssi);
    }
}
#endif

#if CONFIG_BT_HID_DEVICE_ENABLED
/*
 * BT GAP
 * */

void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        ESP_LOGV(TAG, "BT GAP DISC_STATE %s", (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) ? "START" : "STOP");
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            SEND_BT_CB();
        }
        break;
    }
    case ESP_BT_GAP_DISC_RES_EVT: {
        handle_bt_device_result(&param->disc_res);
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT: {
        ESP_LOGI(TAG, "BT GAP PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            //ESP_LOGI(TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
			for(int i = 0; i < 6; i ++){
				pin_code[i] = param->pin_req.bda[5-i];
			}
			ESP_LOGI(TAG, "Pin: %x %x %x %x %x %x\n",pin_code[0],pin_code[1],pin_code[2],pin_code[3],pin_code[4],pin_code[5]);
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 6, pin_code);
        }
        break;
    }

    case ESP_BT_GAP_MODE_CHG_EVT:
        //ESP_LOGI(TAG, "BT GAP MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;
    default:
        ESP_LOGV(TAG, "BT GAP EVENT %s", bt_gap_evt_str(event));
        break;
    }
}
#endif


