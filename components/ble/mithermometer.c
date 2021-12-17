#include "mithermometer.h"
#include <stdlib.h>
#include <string.h>
#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define ERROR_CHECKE(con, str, action)   if (con) {                                                                 \
    if(*str != '\0')                                                                                                \
        ESP_LOGE(TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);                                      \
    action;                                                                                                         \
}

static const char *TAG = "MI THERMOMETER";

#define MI_APPID                    0
const uint8_t MI_DATA_CHAR_UUID[] = {0xa6, 0xa3, 0x7d, 0x99, 0xf2, 0x6f, 0x1a, 0x8a, 0x0c, 0x4b, 0x0a, 0x7a, 0xc1, 0xcc, 0xe0, 0xeb};

typedef struct {
    uint8_t             handle;
    char                *data;
} mi_char_t;

typedef struct mi_thermometer {
    esp_gatt_if_t       gattcif;
    uint16_t            conn_id;
    esp_bd_addr_t       bda;
    EventGroupHandle_t  event;
    mi_state_t          state;
    mi_char_t           model_char;
    mi_char_t           serial_char;
    mi_char_t           fw_char;
    mi_char_t           hw_char;
    mi_char_t           sw_char;
    mi_char_t           battery_char;
    mi_char_t           temp_hum_char;
    uint8_t             handle_write;
    float               temp;
    uint8_t             hum;
} mi_thermometer_t;

mi_thermometer_t    mi_thermometer;

static const int EVT_READY          = BIT0;
static const int EVT_SEARCH_DEVICE  = BIT1;
static const int EVT_OPEN           = BIT2;
static const int EVT_CLOSE          = BIT3;
static const int EVT_SEARCH_SERVICE = BIT4;
static const int EVT_READ           = BIT5;
static const int EVT_WRITE          = BIT6;
static const int EVT_REGISTER       = BIT7;

static esp_err_t _mi_write_char_descr(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t handle) {
    xEventGroupClearBits(mi_thermometer.event, EVT_WRITE);
    uint8_t notify_en = 1;
    esp_ble_gattc_write_char_descr(gattc_if, conn_id, handle, sizeof(notify_en),(uint8_t *)&notify_en, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    if ((xEventGroupWaitBits(mi_thermometer.event, EVT_WRITE, false, true, 1000/portTICK_RATE_MS) & EVT_WRITE) == 0) {
        ESP_LOGE(TAG, "hid button write time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t _mi_register_for_notify(esp_gatt_if_t gattc_if, esp_bd_addr_t bda, uint16_t handle) {
    xEventGroupClearBits(mi_thermometer.event, EVT_REGISTER);
    esp_ble_gattc_register_for_notify(gattc_if, bda, handle);
    if ((xEventGroupWaitBits(mi_thermometer.event, EVT_REGISTER, false, true, 1000/portTICK_RATE_MS) & EVT_REGISTER) == 0) {
        ESP_LOGE(TAG, "hid button register time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void _mi_char_data(uint8_t *data, uint16_t len) {
    switch(mi_thermometer.state) {
        case MI_READ_MODEL:
            mi_thermometer.model_char.data = (char *)calloc(sizeof(char), len + 1);
            if (mi_thermometer.model_char.data) {
                memcpy(mi_thermometer.model_char.data, data, len);
            }
            break;
        case MI_READ_SERIAL:
            mi_thermometer.serial_char.data = (char *)calloc(sizeof(char), len + 1);
            if (mi_thermometer.serial_char.data) {
                memcpy(mi_thermometer.serial_char.data, data, len);
            }
            break;
        case MI_READ_FW_VER:
            mi_thermometer.fw_char.data = (char *)calloc(sizeof(char), len + 1);
            if (mi_thermometer.fw_char.data) {
                memcpy(mi_thermometer.fw_char.data, data, len);
            }
            break;
        case MI_READ_HW_VER:
            mi_thermometer.hw_char.data = (char *)calloc(sizeof(char), len + 1);
            if (mi_thermometer.hw_char.data) {
                memcpy(mi_thermometer.hw_char.data, data, len);
            }
            break;
        case MI_READ_SW_VER:
            mi_thermometer.sw_char.data = (char *)calloc(sizeof(char), len + 1);
            if (mi_thermometer.sw_char.data) {
                memcpy(mi_thermometer.sw_char.data, data, len);
            }
            break;
        case MI_READ_BATTERY:
            mi_thermometer.battery_char.data = (char *)calloc(sizeof(char), len + 1);
            if (mi_thermometer.battery_char.data) {
                memcpy(mi_thermometer.battery_char.data, data, len);
            }
            break;
        case MI_IDLE:
            mi_thermometer.temp = ((uint16_t)data[1]<<8)|data[0];
            mi_thermometer.hum = data[2];
            break;
        default:
            break;
    }
}

static esp_err_t _mi_read_handle(esp_gatt_if_t gattcif, uint16_t conn_id, uint16_t handle, TickType_t tick_to_wait) {
    xEventGroupClearBits(mi_thermometer.event, EVT_READ);
    esp_err_t ret = esp_ble_gattc_read_char(gattcif, conn_id, handle, ESP_GATT_AUTH_REQ_NONE);
    ERROR_CHECKE( ret != ESP_OK, "gattc read failed", return ret);
    if ((xEventGroupWaitBits(mi_thermometer.event, EVT_READ, false, true, 1000/portTICK_RATE_MS) & EVT_READ) == 0) {
        ESP_LOGE(TAG, "hid button read time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void _mi_read_device_services(void) {
    esp_gattc_service_elem_t service_result[10];
    uint16_t scount = 10;
    if (esp_ble_gattc_get_service(mi_thermometer.gattcif, mi_thermometer.conn_id, NULL, service_result, &scount, 0) == ESP_OK) {
        for (uint16_t s = 0; s < scount; s++) {
            esp_gattc_char_elem_t char_result[20];
            uint16_t ccount = 20;
            if (esp_ble_gattc_get_all_char(mi_thermometer.gattcif, mi_thermometer.conn_id, service_result[s].start_handle, service_result[s].end_handle, char_result, &ccount, 0) == ESP_OK) {
                for (uint16_t c = 0; c < ccount; c++) {
                    if(char_result[c].uuid.len == ESP_UUID_LEN_16) {
                        if(char_result[c].uuid.uuid.uuid16 == ESP_GATT_UUID_MODEL_NUMBER_STR) {
                            mi_thermometer.model_char.handle = char_result[c].char_handle;
                        }
                        else if(char_result[c].uuid.uuid.uuid16 == ESP_GATT_UUID_SERIAL_NUMBER_STR) {
                            mi_thermometer.serial_char.handle = char_result[c].char_handle;
                        }
                        else if(char_result[c].uuid.uuid.uuid16 == ESP_GATT_UUID_FW_VERSION_STR) {
                            mi_thermometer.fw_char.handle = char_result[c].char_handle;
                        }
                        else if(char_result[c].uuid.uuid.uuid16 == ESP_GATT_UUID_HW_VERSION_STR) {
                            mi_thermometer.hw_char.handle = char_result[c].char_handle;
                        }
                        else if(char_result[c].uuid.uuid.uuid16 == ESP_GATT_UUID_SW_VERSION_STR) {
                            mi_thermometer.sw_char.handle = char_result[c].char_handle;
                        }
                        else if(char_result[c].uuid.uuid.uuid16 == ESP_GATT_UUID_BATTERY_LEVEL) {
                            mi_thermometer.battery_char.handle = char_result[c].char_handle;
                        }
                    }
                    else {
                        if(memcmp(char_result[c].uuid.uuid.uuid128, MI_DATA_CHAR_UUID, char_result[c].uuid.len) == 0) {
                            mi_thermometer.temp_hum_char.handle = char_result[c].char_handle;
                            esp_gattc_descr_elem_t descr_result[20];
                            uint16_t dcount = 20;
                            if (esp_ble_gattc_get_all_descr(mi_thermometer.gattcif, mi_thermometer.conn_id, char_result[c].char_handle, descr_result, &dcount, 0) == ESP_OK) {
                                for (uint16_t d = 0; d < dcount; d++) {
                                    if (descr_result[d].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
                                        mi_thermometer.handle_write = descr_result[d].handle;
                                    }
                                }
                            } 
                        }
                    }
                }
            }
        }
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            if (param->reg.app_id == MI_APPID) {
                if(mi_thermometer.gattcif == 0)
                    mi_thermometer.gattcif = gattc_if;
                xEventGroupSetBits(mi_thermometer.event, EVT_READY);
            }
        } else {
            ESP_LOGE(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }

    if (gattc_if == ESP_GATT_IF_NONE || gattc_if == mi_thermometer.gattcif) {
        esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
        switch ((int)event) {
        case ESP_GATTC_READ_CHAR_EVT:
        case ESP_GATTC_READ_DESCR_EVT: {
            if(p_data->read.conn_id == mi_thermometer.conn_id) {
                _mi_char_data(p_data->read.value, p_data->read.value_len);
                xEventGroupSetBits(mi_thermometer.event, EVT_READ);
            }
            break;
        }
        case ESP_GATTC_CONNECT_EVT: {
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(mi_thermometer.gattcif, mi_thermometer.conn_id);
            if (mtu_ret) {
                ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
            }
            break;
        }
        case ESP_GATTC_CFG_MTU_EVT:
            if (param->cfg_mtu.status != ESP_GATT_OK) {
                ESP_LOGE(TAG,"Config mtu failed");
            }
            xEventGroupSetBits(mi_thermometer.event, EVT_OPEN);
            break;
        case ESP_GATTC_WRITE_DESCR_EVT: {
            if(p_data->write.conn_id == mi_thermometer.conn_id)
                xEventGroupSetBits(mi_thermometer.event, EVT_WRITE);
            break;
        }
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            xEventGroupSetBits(mi_thermometer.event, EVT_REGISTER);
            break;
        }
        case ESP_GATTC_NOTIFY_EVT: {
            if(p_data->notify.conn_id == mi_thermometer.conn_id) {
                _mi_char_data(p_data->notify.value, p_data->notify.value_len);
            }
            break;
        }
        case ESP_GATTC_OPEN_EVT:
            if ((p_data->open.status == ESP_GATT_OK) || (p_data->open.status == ESP_GATT_ALREADY_OPEN)) {
                mi_thermometer.conn_id = p_data->open.conn_id;
            }
            else {
                ESP_LOGE(TAG, "connect device failed, status %02x", p_data->open.status);
            }
            break;
        case ESP_GATTC_SEARCH_CMPL_EVT:
            if (p_data->search_cmpl.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
                break;
            }
            xEventGroupSetBits(mi_thermometer.event, EVT_SEARCH_SERVICE);
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            xEventGroupSetBits(mi_thermometer.event, EVT_SEARCH_SERVICE);
            break;
        default:
            break;
        }
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    uint8_t *adv_name = NULL;
    esp_ble_gap_cb_param_t *scan_result;
    uint8_t adv_name_len = 0;
    switch ((int)event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            bool is_exist = false;
            if (adv_name_len > 0 && strcmp((char*)"LYWSD03MMC", (char*)adv_name) == 0) {
                is_exist = true;
            }
            if (is_exist) {
                ESP_LOGW(TAG, "Add device: %s ["MACSTR"] RSSI: %d", adv_name, MAC2STR(scan_result->scan_rst.bda), scan_result->scan_rst.rssi);
                memcpy(mi_thermometer.bda, scan_result->scan_rst.bda, sizeof(esp_bd_addr_t));
                esp_ble_gap_stop_scanning();
                xEventGroupSetBits(mi_thermometer.event, EVT_SEARCH_DEVICE);
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void mi_task(void *pvParameters) {
    ESP_LOGI(TAG, "BLE start...");
    while (1) {
        switch(mi_thermometer.state) {
            case MI_INIT:
                if ((xEventGroupWaitBits(mi_thermometer.event, EVT_READY, false, true, 1000/portTICK_RATE_MS) & EVT_READY) == 0) { 
                    goto _continue;
                }
                ESP_LOGI(TAG, "Start scan device");
                esp_ble_gap_start_scanning(10000);
                mi_thermometer.state = MI_SCAN;
                break;
            case MI_SCAN:
                if ((xEventGroupWaitBits(mi_thermometer.event, EVT_SEARCH_DEVICE, false, true, 1000/portTICK_RATE_MS) & EVT_SEARCH_DEVICE) == 0) { 
                    goto _continue;
                }
                ESP_LOGI(TAG, "Open connect to ["ESP_BD_ADDR_STR"]", ESP_BD_ADDR_HEX(mi_thermometer.bda));
                esp_err_t ret = esp_ble_gattc_open(mi_thermometer.gattcif, mi_thermometer.bda, BLE_ADDR_TYPE_PUBLIC, true);
                ERROR_CHECKE(ret != ESP_OK, "gattc open failed", continue);
                mi_thermometer.state = MI_CONNECT;
                break;
            case MI_CONNECT:
                if ((xEventGroupWaitBits(mi_thermometer.event, EVT_OPEN, false, true, 1000/portTICK_RATE_MS) & EVT_OPEN) == 0) { 
                    goto _continue;
                }
                mi_thermometer.state = MI_SEARCH_SERVICE;
                break;
            case MI_SEARCH_SERVICE:
                _mi_read_device_services();
                mi_thermometer.state = MI_READ_MODEL;
                break;
            case MI_READ_MODEL:
                if(_mi_read_handle(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.model_char.handle, 200/portTICK_RATE_MS) != ESP_OK)
                    goto _continue;
                ESP_LOGI(TAG, "Read model: \t%s", mi_thermometer.model_char.data);
                mi_thermometer.state = MI_READ_SERIAL;
                break;
            case MI_READ_SERIAL:
                if(_mi_read_handle(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.serial_char.handle, 200/portTICK_RATE_MS) != ESP_OK)
                    goto _continue;
                ESP_LOGI(TAG, "Read serial: \t%s", mi_thermometer.serial_char.data);
                mi_thermometer.state = MI_READ_FW_VER;
                break;
            case MI_READ_FW_VER:
                if(_mi_read_handle(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.fw_char.handle, 200/portTICK_RATE_MS) != ESP_OK)
                    goto _continue;
                ESP_LOGI(TAG, "Read fw ver: \t%s", mi_thermometer.fw_char.data);
                mi_thermometer.state = MI_READ_HW_VER;
                break;
            case MI_READ_HW_VER:
                if(_mi_read_handle(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.hw_char.handle, 200/portTICK_RATE_MS) != ESP_OK)
                    goto _continue;
                ESP_LOGI(TAG, "Read hw ver: \t%s", mi_thermometer.hw_char.data);
                mi_thermometer.state = MI_READ_SW_VER;
                break;
             case MI_READ_SW_VER:
                if(_mi_read_handle(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.sw_char.handle, 200/portTICK_RATE_MS) != ESP_OK)
                    goto _continue;
                ESP_LOGI(TAG, "Read sw ver: \t%s", mi_thermometer.sw_char.data);
                mi_thermometer.state = MI_READ_BATTERY;
                break;
            case MI_READ_BATTERY:
                if(_mi_read_handle(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.battery_char.handle, 200/portTICK_RATE_MS) != ESP_OK)
                    goto _continue;
                ESP_LOGI(TAG, "Read battery: %d", mi_thermometer.battery_char.data[0]);
                mi_thermometer.state = MI_READ_TEMP_HUM;
                break;
            case MI_READ_TEMP_HUM:
                if(_mi_register_for_notify(mi_thermometer.gattcif, mi_thermometer.bda, mi_thermometer.temp_hum_char.handle) != ESP_OK)
                    goto _continue;
                if(_mi_write_char_descr(mi_thermometer.gattcif, mi_thermometer.conn_id, mi_thermometer.handle_write) != ESP_OK)
                    goto _continue;
                mi_thermometer.state = MI_IDLE;
                break;
            case MI_IDLE:
                if((mi_thermometer.temp != 0) && (mi_thermometer.hum != 0))
                    ESP_LOGI(TAG, "Read temp: %2.1f, hum: %d", mi_thermometer.temp/100, mi_thermometer.hum);
                break;
            default:
                break;
        }
_continue:
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

esp_err_t mi_init(void) {
    esp_err_t ret = ESP_FAIL;
    mi_thermometer.state = MI_INIT;
    mi_thermometer.event = xEventGroupCreate();
    xEventGroupClearBits(mi_thermometer.event, EVT_READY);
    xEventGroupClearBits(mi_thermometer.event, EVT_SEARCH_DEVICE);
    xEventGroupClearBits(mi_thermometer.event, EVT_OPEN);
    xEventGroupClearBits(mi_thermometer.event, EVT_CLOSE);
    xEventGroupClearBits(mi_thermometer.event, EVT_SEARCH_SERVICE);
    xEventGroupClearBits(mi_thermometer.event, EVT_READ);
    xEventGroupClearBits(mi_thermometer.event, EVT_WRITE);
    xEventGroupClearBits(mi_thermometer.event, EVT_REGISTER);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ERROR_CHECKE( ret != ESP_OK, "initialize controller failed", return ret);
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ERROR_CHECKE( ret != ESP_OK, "enable controller failed", return ret);
    ret = esp_bluedroid_init();
    ERROR_CHECKE( ret != ESP_OK, "init bluedroid failed", return ret);
    ret = esp_bluedroid_enable();
    ERROR_CHECKE( ret != ESP_OK, "enable bluedroid failed", return ret);
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    ERROR_CHECKE( ret != ESP_OK, "gap register failed", return ret);
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    ERROR_CHECKE( ret != ESP_OK, "gattc register failed", return ret);
    ret = esp_ble_gattc_app_register(MI_APPID);
    ERROR_CHECKE( ret != ESP_OK, "gattc app register failed", return ret);
    ret = esp_ble_gatt_set_local_mtu(200);
    ERROR_CHECKE( ret != ESP_OK, "set local MTU failed", return ret);
    xTaskCreate(&mi_task, "ble_task", 4 * 1024, NULL, 5, NULL);
    return ESP_OK;
}