#include "ble.h"
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

#define BLE_APPID           0
#define BLE_SERVICE_UUID    0xFE95
#define BLE_AVDTP_CHAR      0x0019
#define BLE_UPNP_CHAR       0x0010
#define REMOTE_NOTIFY_UUID    0x2A37

unsigned char CMD_GET_INFO[] = { 0xA2, 0x00, 0x00, 0x00};
unsigned char CMD_SET_KEY[] = { 0x15, 0x00, 0x00, 0x00};
unsigned char CMD_SEND_DATA[] = { 0x00, 0x00, 0x00, 0x03, 0x04, 0x00};
unsigned char CMD_WR_DID[] = { 0x00, 0x00, 0x00, 0x00, 0x02, 0x00};

unsigned char RCV_RDY[] = { 0x00, 0x00, 0x01, 0x01};
unsigned char RCV_OK[] = { 0x00, 0x00, 0x01, 0x00};
unsigned char RCV_TOUT[] = { 0x00, 0x00, 0x01, 0x05, 0x01, 0x00};

static const char *TAG = "ble";
esp_gatt_if_t       ble_gattcif = 0;
uint16_t            ble_conn_id;
esp_bd_addr_t       ble_bda;
EventGroupHandle_t  ble_event;
hid_dev_data_read_t ble_read;
bool    is_open = false;
bool    is_connected = false;
bool    is_scanned = false;

static const int EVT_READY = BIT0;
static const int EVT_SEARCH_DEVICE = BIT1;
static const int EVT_OPEN = BIT2;
static const int EVT_CLOSE = BIT3;
static const int EVT_SEARCH_SERVICE = BIT4;
static const int EVT_READ = BIT5;
static const int EVT_WRITE = BIT6;
static const int EVT_REGISTER = BIT7;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_RANDOM,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

typedef struct {
    uint16_t service_start_handle;
    uint16_t service_end_handle;
} service_t;

service_t service[16];
uint8_t service_cnt = 0;

static esp_gatt_status_t read_char(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t handle, esp_gatt_auth_req_t auth_req, uint8_t **out, uint16_t *out_len) {
    ble_read.data = NULL;
    ble_read.len = 0;
    xEventGroupClearBits(ble_event, EVT_READ);
    if (esp_ble_gattc_read_char(gattc_if, conn_id, handle, auth_req) != ESP_OK) {
        ESP_LOGE(TAG, "read_char failed");
        return ESP_GATT_ERROR;
    }
    if ((xEventGroupWaitBits(ble_event, EVT_READ, false, true, 10000/portTICK_RATE_MS) & EVT_READ) == 0) {
        ESP_LOGE(TAG, "hid button read time out");
        return ESP_GATT_ERROR;
    }
    if (ble_read.status == ESP_GATT_OK) {
        *out = ble_read.data;
        *out_len = ble_read.len;
    }
    return ble_read.status;
}

static esp_gatt_status_t read_descr(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t handle, esp_gatt_auth_req_t auth_req, uint8_t **out, uint16_t *out_len) {
    ble_read.data = NULL;
    ble_read.len = 0;
    xEventGroupClearBits(ble_event, EVT_READ);
    if (esp_ble_gattc_read_char_descr(gattc_if, conn_id, handle, auth_req) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gattc_read_char failed");
        return ESP_GATT_ERROR;
    }
    if ((xEventGroupWaitBits(ble_event, EVT_READ, false, true, 10000/portTICK_RATE_MS) & EVT_READ) == 0) {
        ESP_LOGE(TAG, "hid button read time out");
        return ESP_GATT_ERROR;
    }
    if (ble_read.status == ESP_GATT_OK) {
        *out = ble_read.data;
        *out_len = ble_read.len;
    }
    return ble_read.status;
}

static esp_err_t register_for_notify(esp_gatt_if_t gattc_if, esp_bd_addr_t bda, uint16_t handle) {
    xEventGroupClearBits(ble_event, EVT_REGISTER);
    esp_ble_gattc_register_for_notify(gattc_if, bda, handle);
    if ((xEventGroupWaitBits(ble_event, EVT_REGISTER, false, true, 1000/portTICK_RATE_MS) & EVT_REGISTER) == 0) {
        ESP_LOGE(TAG, "hid button register time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t write_char_descr(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t handle, uint16_t value_len, uint8_t *value, esp_gatt_write_type_t write_type, esp_gatt_auth_req_t auth_req) {
    xEventGroupClearBits(ble_event, EVT_WRITE);
    esp_ble_gattc_write_char_descr(gattc_if, conn_id, handle, value_len, value, write_type, auth_req);
    if ((xEventGroupWaitBits(ble_event, EVT_WRITE, false, true, 1000/portTICK_RATE_MS) & EVT_WRITE) == 0) {
        ESP_LOGE(TAG, "hid button write time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ble_read_handle(esp_gatt_if_t gattcif, uint16_t conn_id, uint16_t handle, TickType_t tick_to_wait) {
    xEventGroupClearBits(ble_event, EVT_READ);
    ESP_LOGI(TAG, "Read gattcif: %d, conn_id: %d, handle: %x", gattcif, conn_id, handle);
    esp_err_t ret = esp_ble_gattc_read_char(gattcif, conn_id, handle, ESP_GATT_AUTH_REQ_NONE);
    ERROR_CHECKE( ret != ESP_OK, "gattc read failed", return ret);
    if ((xEventGroupWaitBits(ble_event, EVT_READ, false, true, 1000/portTICK_RATE_MS) & EVT_READ) == 0) {
        ESP_LOGE(TAG, "hid button read time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ble_write_magic(esp_gatt_if_t gattcif, uint16_t conn_id, uint16_t handle, uint16_t value_len, uint8_t *value, TickType_t tick_to_wait) {
    xEventGroupClearBits(ble_event, EVT_WRITE);
    ESP_LOGI(TAG, "Write gattcif: %d, conn_id: %d, handle_write: %x", gattcif, conn_id, handle);
    esp_err_t ret = esp_ble_gattc_write_char(gattcif, conn_id, handle, value_len, value, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    ERROR_CHECKE( ret != ESP_OK, "gattc write char failed", return ret);
    if ((xEventGroupWaitBits(ble_event, EVT_WRITE, false, true, tick_to_wait) & EVT_WRITE) == 0) {
        ESP_LOGE(TAG, "hid button write time out");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void hid_read_device_services(void) {
    static bool is_register = false;
    if(is_register) return;
    esp_gattc_service_elem_t service_result[10];
    uint16_t dcount = 10;
    uint8_t hidindex = 0;
    uint16_t suuid, cuuid, duuid;
    uint16_t chandle, dhandle;
    uint8_t *rdata = 0;
    uint16_t rlen = 0;
    esp_bt_uuid_t service = {
        .len = ESP_UUID_LEN_16,
        //.uuid.uuid16 = 0xFE95,    //FE95_ServiceUUID
        //.uuid.uuid16 = 0x1f1f,    //RxTxUUID
        .uuid.uuid16 = 0x1f10,    //RxTx_ServiceUUID
        //.uuid.uuid16 = 0x181A,      //tempServiceUUID
        //.uuid.uuid16 = 0x2A1F,    //tempCharUUID
        //.uuid.uuid16 = 0x2A6F,    //humiCharUUID
    };
    if (esp_ble_gattc_get_service(ble_gattcif, ble_conn_id, &service, service_result, &dcount, 0) == ESP_OK) {
        //if (esp_ble_gattc_get_service(ble_gattcif, ble_conn_id, NULL, service_result, &dcount, 0) == ESP_OK) {
        ESP_LOGW(TAG, "Found %d HID Services", dcount);
        for (uint16_t s = 0; s < dcount; s++) {
            suuid = service_result[s].uuid.uuid.uuid16;
            ESP_LOGW(TAG, "SRV(%d) %s start_handle 0x%04x, end_handle 0x%04x, suuid: 0x%04x", s, service_result[s].is_primary ? " PRIMARY" : "", service_result[s].start_handle, service_result[s].end_handle, suuid);
            esp_gattc_char_elem_t char_result[20];
            uint16_t ccount = 20;
            if (esp_ble_gattc_get_all_char(ble_gattcif, ble_conn_id, service_result[s].start_handle, service_result[s].end_handle, char_result, &ccount, 0) == ESP_OK) {
                for (uint16_t c = 0; c < ccount; c++) {
                    cuuid = char_result[c].uuid.uuid.uuid16;
                    chandle = char_result[c].char_handle;
                    ESP_LOGW(TAG, "  CHAR:(%d), handle: %x, perm: 0x%02x, cuuid: 0x%04x", c + 1, chandle, char_result[c].properties, cuuid);
                    if(cuuid == ESP_GATT_UUID_GAP_DEVICE_NAME) {
                        ESP_LOGI(TAG, "Read device name");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_MODEL_NUMBER_STR) {
                        ESP_LOGI(TAG, "Read model number");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_SERIAL_NUMBER_STR) {
                        ESP_LOGI(TAG, "Read serial number");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_FW_VERSION_STR) {
                        ESP_LOGI(TAG, "Read fw version");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_HW_VERSION_STR) {
                        ESP_LOGI(TAG, "Read hw version");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_SW_VERSION_STR) {
                        ESP_LOGI(TAG, "Read sw version");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_MANU_NAME) {
                        ESP_LOGI(TAG, "Read manu name");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_GAP_PREF_CONN_PARAM) {
                        ESP_LOGI(TAG, "Read conn param");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else if(cuuid == ESP_GATT_UUID_BATTERY_LEVEL) {
                        ESP_LOGI(TAG, "Read battery level");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    else {
                        ESP_LOGI(TAG, "Read...");
                        ble_read_handle(ble_gattcif, ble_conn_id, chandle, 200/portTICK_RATE_MS);
                    }
                    // if(cuuid == 0x0017) {
                    //     register_for_notify(ble_gattcif, ble_bda, chandle);
                    //     is_register = true;
                    esp_gattc_descr_elem_t descr_result[20];
                    uint16_t dcount = 20;
                    if (esp_ble_gattc_get_all_descr(ble_gattcif, ble_conn_id, char_result[c].char_handle, descr_result, &dcount, 0) == ESP_OK) {
                        for (uint16_t d = 0; d < dcount; d++) {
                            duuid = descr_result[d].uuid.uuid.uuid16;
                            dhandle = descr_result[d].handle;
                            ESP_LOGI(TAG, "    DESCR:(%d), handle: %x, duuid: 0x%04x", d + 1, dhandle, duuid);
                            if (duuid == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
                                register_for_notify(ble_gattcif, ble_bda, dhandle);
                                uint16_t ccc_data = 1;
                                write_char_descr(ble_gattcif, ble_conn_id, dhandle, 2, (uint8_t *)&ccc_data, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
                            }
                        }
                    }
                    // }
                }
            }
            // if (suuid != ESP_GATT_UUID_BATTERY_SERVICE_SVC
            //         && suuid != ESP_GATT_UUID_DEVICE_INFO_SVC
            //         && suuid != ESP_GATT_UUID_HID_SVC
            //         && suuid != 0x1800) {   //device name?
            //     continue;
            // }
            // esp_gattc_char_elem_t char_result[20];
            // uint16_t ccount = 20;
            // if (esp_ble_gattc_get_all_char(ble_gattcif, ble_conn_id, service_result[s].start_handle, service_result[s].end_handle, char_result, &ccount, 0) == ESP_OK) {
            //     for (uint16_t c = 0; c < ccount; c++) {
            //         cuuid = char_result[c].uuid.uuid.uuid16;
            //         chandle = char_result[c].char_handle;
            //         ESP_LOGW(TAG, "  CHAR:(%d), handle: %d, perm: 0x%02x, uuid: 0x%04x", c + 1, chandle, char_result[c].properties, cuuid);
            //         if (suuid == 0x1800) {
            //             if (cuuid == 0x2a00 && (char_result[c].properties & ESP_GATT_CHAR_PROP_BIT_READ) != 0) {
            //                 if (read_char(ble_gattcif, ble_conn_id, chandle, ESP_GATT_AUTH_REQ_NO_MITM, &rdata, &rlen) == ESP_GATT_OK && rlen) {
            //                     ESP_LOGI(TAG, "Device name %s", (const char *)rdata);
            //                 }
            //             } else {
            //                 continue;
            //             }
            //         } else if (suuid == ESP_GATT_UUID_BATTERY_SERVICE_SVC) {
            //             if (cuuid == ESP_GATT_UUID_BATTERY_LEVEL && (char_result[c].properties & ESP_GATT_CHAR_PROP_BIT_READ) != 0) {
            //                 ESP_LOGI(TAG, "battery_handle 0x%04x", chandle);
            //             } else {
            //                 continue;
            //             }
            //         } else if (suuid == ESP_GATT_UUID_DEVICE_INFO_SVC) {
            //             if (char_result[c].properties & ESP_GATT_CHAR_PROP_BIT_READ) {
            //                 if (cuuid == ESP_GATT_UUID_PNP_ID) {
            //                     if (read_char(ble_gattcif, ble_conn_id, chandle, ESP_GATT_AUTH_REQ_NO_MITM, &rdata, &rlen) == ESP_GATT_OK && rlen == 7) {
            //                         ESP_LOGI(TAG, "vendor_id %04x product_id %04x version %04x", *((uint16_t *)&rdata[1]), *((uint16_t *)&rdata[3]), *((uint16_t *)&rdata[5]));
            //                     }
            //                     free(rdata);
            //                 } else if (cuuid == ESP_GATT_UUID_MANU_NAME) {
            //                     if (read_char(ble_gattcif, ble_conn_id, chandle, ESP_GATT_AUTH_REQ_NO_MITM, &rdata, &rlen) == ESP_GATT_OK && rlen) {
            //                         ESP_LOGI(TAG, "manufacturer_name %s", (const char *)rdata);
            //                     }
            //                 } else if (cuuid == ESP_GATT_UUID_SERIAL_NUMBER_STR) {
            //                     if (read_char(ble_gattcif, ble_conn_id, chandle, ESP_GATT_AUTH_REQ_NO_MITM, &rdata, &rlen) == ESP_GATT_OK && rlen) {
            //                         ESP_LOGI(TAG, "serial_number %s", (const char *)rdata);
            //                     }
            //                 }
            //             }
            //             continue;
            //         } else {
            //             if (cuuid == ESP_GATT_UUID_HID_REPORT_MAP) {
            //                 if (char_result[c].properties & ESP_GATT_CHAR_PROP_BIT_READ) {
            //                     if (read_char(ble_gattcif, ble_conn_id, chandle, ESP_GATT_AUTH_REQ_NO_MITM, &rdata, &rlen) == ESP_GATT_OK && rlen) {
            //                         ESP_LOGI(TAG, "report_maps data %s len %d", (const uint8_t *)rdata, rlen);
            //                     }
            //                 }
            //                 continue;
            //             } else if (cuuid == ESP_GATT_UUID_HID_BT_KB_INPUT || cuuid == ESP_GATT_UUID_HID_BT_KB_OUTPUT || cuuid == ESP_GATT_UUID_HID_BT_MOUSE_INPUT || cuuid == ESP_GATT_UUID_HID_REPORT) {
            //                 ESP_LOGI(TAG, "report permissions %d handle %04x map_index %d", char_result[c].properties, chandle, hidindex);
            //                 // if (cuuid == ESP_GATT_UUID_HID_BT_KB_INPUT) {
            //                 //     report->protocol_mode = ESP_HID_PROTOCOL_MODE_BOOT;
            //                 //     report->report_type = ESP_HID_REPORT_TYPE_INPUT;
            //                 //     report->usage = ESP_HID_USAGE_KEYBOARD;
            //                 //     report->value_len = 8;
            //                 // } else if (cuuid == ESP_GATT_UUID_HID_BT_KB_OUTPUT) {
            //                 //     report->protocol_mode = ESP_HID_PROTOCOL_MODE_BOOT;
            //                 //     report->report_type = ESP_HID_REPORT_TYPE_OUTPUT;
            //                 //     report->usage = ESP_HID_USAGE_KEYBOARD;
            //                 //     report->value_len = 8;
            //                 // } else if (cuuid == ESP_GATT_UUID_HID_BT_MOUSE_INPUT) {
            //                 //     report->protocol_mode = ESP_HID_PROTOCOL_MODE_BOOT;
            //                 //     report->report_type = ESP_HID_REPORT_TYPE_INPUT;
            //                 //     report->usage = ESP_HID_USAGE_MOUSE;
            //                 //     report->value_len = 8;
            //                 // } else {
            //                 //     report->protocol_mode = ESP_HID_PROTOCOL_MODE_REPORT;
            //                 //     report->report_type = 0;
            //                 //     report->usage = ESP_HID_USAGE_GENERIC;
            //                 //     report->value_len = 0;
            //                 // }
            //             } else {
            //                 continue;
            //             }
            //         }
            //         esp_gattc_descr_elem_t descr_result[20];
            //         uint16_t dcount = 20;
            //         if (esp_ble_gattc_get_all_descr(ble_gattcif, ble_conn_id, char_result[c].char_handle, descr_result, &dcount, 0) == ESP_OK) {
            //             for (uint16_t d = 0; d < dcount; d++) {
            //                 duuid = descr_result[d].uuid.uuid.uuid16;
            //                 dhandle = descr_result[d].handle;
            //                 ESP_LOGI(TAG, "    DESCR:(%d), handle: %d, uuid: 0x%04x", d + 1, dhandle, duuid);
            //                 if (suuid == ESP_GATT_UUID_BATTERY_SERVICE_SVC) {
            //                     if (duuid == ESP_GATT_UUID_CHAR_CLIENT_CONFIG && (char_result[c].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY) != 0) {
            //                         ESP_LOGI(TAG, "battery_ccc_handle 0x%04x", dhandle);
            //                     }
            //                 } else if (suuid == ESP_GATT_UUID_HID_SVC) {
            //                     if (duuid == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
            //                         ESP_LOGI(TAG, "ccc_handle 0x%04x", dhandle);
            //                     } else if (duuid == ESP_GATT_UUID_RPT_REF_DESCR) {
            //                         if (read_descr(ble_gattcif, ble_conn_id, dhandle, ESP_GATT_AUTH_REQ_NO_MITM, &rdata, &rlen) == ESP_GATT_OK && rlen) {
            //                             ESP_LOGI(TAG, "report_id %d report_type %d", rdata[0], rdata[1]);
            //                             free(rdata);
            //                         }
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }
        }
    }
    is_register = true;
    /*ble_read_handle(ble_gattcif, ble_conn_id, 0x0018, 200/portTICK_RATE_MS);
    ble_write_magic(ble_gattcif, ble_conn_id, 0x0093, 4, RCV_RDY, 200/portTICK_RATE_MS);
    ble_write_magic(ble_gattcif, ble_conn_id, 0x0093, 4, RCV_OK, 200/portTICK_RATE_MS);
    ble_write_magic(ble_gattcif, ble_conn_id, 0x0081, 4, CMD_SET_KEY, 200/portTICK_RATE_MS);
    ble_write_magic(ble_gattcif, ble_conn_id, 0x0093, 6, CMD_SEND_DATA, 200/portTICK_RATE_MS);*/
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    // fail_debug_flag = true;
    // if(fail_debug_flag)
    //     ESP_LOGW(TAG, "gattc_cb: %d, gattc_if: %d", (int)event, gattc_if);
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            if (param->reg.app_id == BLE_APPID) {
                ESP_LOGW(TAG, "Reg app_id %04x, status %d, gattc_if %d", param->reg.app_id, param->reg.status, gattc_if);
                if(ble_gattcif == 0)
                    ble_gattcif = gattc_if;
                xEventGroupSetBits(ble_event, EVT_READY);
            }
        } else {
            ESP_LOGE(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }

    if (gattc_if == ESP_GATT_IF_NONE || gattc_if == ble_gattcif) {
        esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
        switch ((int)event) {
        case ESP_GATTC_READ_CHAR_EVT:
        case ESP_GATTC_READ_DESCR_EVT: {
            ESP_LOGW(TAG, "READ: %d", p_data->read.conn_id);
            ESP_LOG_BUFFER_HEX(TAG, p_data->read.value, p_data->read.value_len);
            if(p_data->read.value_len) ESP_LOGW(TAG, "READ DATA: %s", p_data->read.value);
            if(p_data->read.conn_id == ble_conn_id) {
                ble_read.status = p_data->read.status;
                ble_read.len = 0;
                ble_read.data = NULL;
                if (ble_read.status == 0 && p_data->read.value_len > 0) {
                    ble_read.len = p_data->read.value_len;
                    ble_read.data = (uint8_t *)malloc(ble_read.len + 1);
                    if (ble_read.data) {
                        memcpy(ble_read.data, p_data->read.value, ble_read.len);
                        ble_read.data[ble_read.len] = 0;
                    }
                }
                xEventGroupSetBits(ble_event, EVT_READ);
            }
            break;
        }
        case ESP_GATTC_CONNECT_EVT:
            ESP_LOGW(TAG, "ESP_GATTC_CONNECT_EVT bda " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->connect.remote_bda));
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(ble_gattcif, ble_conn_id);
            if (mtu_ret) {
                ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
            }
            xEventGroupSetBits(ble_event, EVT_OPEN);
            // esp_ble_conn_update_params_t conn_params = {0};
            // memcpy(conn_params.bda, mi_device->bda, sizeof(esp_bd_addr_t));
            // conn_params.latency = 0;
            // conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            // conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            // conn_params.timeout = 32000;   // timeout = 3200*10ms = 32000ms
            // esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTC_CFG_MTU_EVT:
            ESP_LOGW(TAG, "ESP_GATTC_CFG_MTU_EVT");
            if (param->cfg_mtu.status != ESP_GATT_OK) {
                ESP_LOGE(TAG,"Config mtu failed");
            }
            esp_ble_gattc_search_service(ble_gattcif, ble_conn_id, NULL);
            break;
        case ESP_GATTC_WRITE_CHAR_EVT:
            ESP_LOGW(TAG, "ESP_GATTC_WRITE_CHAR_EVT: conn_id %d, status %x", p_data->write.conn_id, p_data->write.status);
            if (p_data->write.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "write char failed, error status = %x", p_data->write.status);
            } else {
                ESP_LOGI(TAG, "write char success");
                xEventGroupSetBits(ble_event, EVT_WRITE);
            }
            break;
        case ESP_GATTC_WRITE_DESCR_EVT: {
            ESP_LOGW(TAG, "WRITE: %d", p_data->write.conn_id);
            if(p_data->write.conn_id == ble_conn_id)
                xEventGroupSetBits(ble_event, EVT_WRITE);
            break;
        }
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            ESP_LOGI(TAG, " ESP_GATTC_REG_FOR_NOTIFY_EVT status: %d, handle: %08x", p_data->reg_for_notify.status, p_data->reg_for_notify.handle);
            xEventGroupSetBits(ble_event, EVT_REGISTER);
            uint16_t count = 0;
            uint16_t offset = 0;
            uint16_t notify_en = 1;
            uint16_t service_start_handle = 0x21;
            uint16_t service_end_handle = 0x4B;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(ble_gattcif,
                                           ble_conn_id,
                                           ESP_GATT_DB_DESCRIPTOR,
                                           service_start_handle,
                                           service_end_handle,
                                           p_data->reg_for_notify.handle,
                                           &count);
            if (ret_status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0) {
                esp_gattc_descr_elem_t *descr_elem_result = NULL;
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result) {
                    ESP_LOGE(TAG, "malloc error, gattc no mem");
                } else {
                    ret_status = esp_ble_gattc_get_all_descr(ble_gattcif,
                                 ble_conn_id,
                                 p_data->reg_for_notify.handle,
                                 descr_elem_result,
                                 &count,
                                 offset);
                    if (ret_status != ESP_GATT_OK) {
                        ESP_LOGE(TAG, "esp_ble_gattc_get_all_descr error, %d", __LINE__);
                    }
                    for (int i = 0; i < count; ++i)
                    {
                        if(descr_elem_result[i].uuid.len == ESP_UUID_LEN_16) {
                            ESP_LOGW(TAG, "uuid %04x", descr_elem_result[i].uuid.uuid.uuid16);
                        }
                        else {
                            ESP_LOGW(TAG, "uuid len %d handle %x", descr_elem_result[i].uuid.len, descr_elem_result[i].handle);
                            ESP_LOG_BUFFER_HEX(TAG, descr_elem_result[i].uuid.uuid.uuid128, descr_elem_result[i].uuid.len);
                        }
                        if (descr_elem_result[i].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[i].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                        {
                            ESP_LOGI(TAG, "write_char_descr handle %x uuid %04x", descr_elem_result[i].handle, descr_elem_result[i].uuid.uuid.uuid16);
                            esp_ble_gattc_write_char_descr (ble_gattcif,
                                                            ble_conn_id,
                                                            descr_elem_result[i].handle,
                                                            sizeof(notify_en),
                                                            (uint8_t *)&notify_en,
                                                            ESP_GATT_WRITE_TYPE_RSP,
                                                            ESP_GATT_AUTH_REQ_NONE);

                            break;
                        }
                    }
                }
                free(descr_elem_result);
            }
            break;
        }
        case ESP_GATTC_NOTIFY_EVT: {
            ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT");
            esp_log_buffer_hex(TAG, p_data->notify.value, p_data->notify.value_len);
            float temp = ((uint16_t)p_data->notify.value[1]<<8)|p_data->notify.value[0];
            uint8_t hum = p_data->notify.value[2];
            ESP_LOGW(TAG, "Mi temp %2.1f, hum %d...", temp/100, hum);
            break;
        }
        case ESP_GATTC_REG_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_REG_EVT");
            //esp_ble_gap_config_local_privacy(true);
            break;
        case ESP_GATTC_OPEN_EVT:
            ESP_LOGW(TAG, "OPEN bda " ESP_BD_ADDR_STR ", conn_id %d, status 0x%x, mtu %d", ESP_BD_ADDR_HEX(p_data->open.remote_bda), p_data->open.conn_id, p_data->open.status, p_data->open.mtu);
            if ((p_data->open.status == ESP_GATT_OK) || (p_data->open.status == ESP_GATT_ALREADY_OPEN)) {
                ble_conn_id = p_data->open.conn_id;
            }
            else {
                ESP_LOGE(TAG, "connect device failed, status %02x", p_data->open.status);
            }
            break;
        case ESP_GATTC_CLOSE_EVT:
            ESP_LOGW(TAG, "ESP_GATTC_CLOSE_EVT bda " ESP_BD_ADDR_STR ", conn_id %d, status 0x%x, reason 0x%x", ESP_BD_ADDR_HEX(p_data->close.remote_bda), p_data->close.conn_id, p_data->close.status, p_data->close.reason);
            break;
        case ESP_GATTC_DISCONNECT_EVT:
            ESP_LOGW(TAG, "DISCONNECT: bda " ESP_BD_ADDR_STR ", conn_id %u, reason 0x%x", ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.conn_id, p_data->disconnect.reason);
            break;
        case ESP_GATTC_SEARCH_RES_EVT:
            //ESP_LOGI(TAG, "SEARCH RESULT: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
            ESP_LOGI(TAG, "SEARCH RESULT: start handle %x end handle %x current handle value %d",
                     p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
            service[service_cnt].service_start_handle = p_data->search_res.start_handle;
            service[service_cnt].service_end_handle = p_data->search_res.end_handle;
            service_cnt++;
            break;
        case ESP_GATTC_SEARCH_CMPL_EVT:
            if (p_data->search_cmpl.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
                break;
            }
            ESP_LOGI(TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
            xEventGroupSetBits(ble_event, EVT_SEARCH_SERVICE);
            for(uint8_t cnt = 0; cnt < service_cnt; cnt++) {
                uint16_t count  = 0;
                uint16_t offset = 0;
                uint16_t service_start_handle = service[cnt].service_start_handle;
                uint16_t service_end_handle = service[cnt].service_end_handle;
                esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(ble_gattcif,
                                               ble_conn_id,
                                               ESP_GATT_DB_CHARACTERISTIC,
                                               service_start_handle,
                                               service_end_handle,
                                               0,
                                               &count);
                if (ret_status != ESP_GATT_OK) {
                    ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
                }
                if (count > 0) {
                    esp_gattc_char_elem_t *char_elem_result   = NULL;
                    char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                    if (!char_elem_result) {
                        ESP_LOGE(TAG, "gattc no mem");
                    } else {
                        ret_status = esp_ble_gattc_get_all_char(ble_gattcif,
                                                                ble_conn_id,
                                                                service_start_handle,
                                                                service_end_handle,
                                                                char_elem_result,
                                                                &count,
                                                                offset);
                        if (ret_status != ESP_GATT_OK) {
                            ESP_LOGE(TAG, "esp_ble_gattc_get_all_char error, %d", __LINE__);
                        }
                        if (count > 0) {
                            for (int i = 0; i < count; ++i) {
                                if(char_elem_result[i].uuid.len == ESP_UUID_LEN_16)
                                    ESP_LOGW(TAG, "uuid %04x", char_elem_result[i].uuid.uuid.uuid16);
                                else {
                                    ESP_LOGW(TAG, "uuid len %d handle %x", char_elem_result[i].uuid.len, char_elem_result[i].char_handle);
                                    ESP_LOG_BUFFER_HEX(TAG, char_elem_result[i].uuid.uuid.uuid128, char_elem_result[i].uuid.len);
                                }
                                if(char_elem_result[i].char_handle == 0x36) {
                                    ESP_LOGI(TAG, "register_for_notify start_handle %x end_handle %x uuid %04x", service_start_handle, service_end_handle, char_elem_result[i].uuid.uuid.uuid16);
                                    esp_ble_gattc_register_for_notify (ble_gattcif,
                                                                       ble_bda,
                                                                       char_elem_result[i].char_handle);
                                }
                            }
                        }
                    }
                    free(char_elem_result);
                }
            }
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_DIS_SRVC_CMPL_EVT");
            xEventGroupSetBits(ble_event, EVT_SEARCH_SERVICE);
            break;
        default:
            ESP_LOGW(TAG, "EVT %d", event);
            break;
        }
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    uint8_t *adv_name = NULL;
    esp_ble_gap_cb_param_t *scan_result;
    uint8_t adv_name_len = 0;
    switch ((int)event) {
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "config local privacy failed, error code =%x", param->local_privacy_cmpl.status);
            break;
        }
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret) {
            ESP_LOGE(TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Scan start success");
        } else {
            ESP_LOGE(TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            //ESP_LOGI(TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            //ESP_LOGI(TAG, "Searched Device Name Len %d", adv_name_len);
            bool is_exist = false;
            if (adv_name_len > 0 && strcmp((char*)"LYWSD03MMC", (char*)adv_name) == 0) {
                is_exist = true;
            }
            if (is_exist) {
                ESP_LOGW(TAG, "Add device: %s["MACSTR"] RSSI: %d TYPE %d", adv_name, MAC2STR(scan_result->scan_rst.bda), scan_result->scan_rst.rssi, scan_result->scan_rst.ble_addr_type);
                memcpy(ble_bda, scan_result->scan_rst.bda, sizeof(esp_bd_addr_t));
                ESP_LOGI(TAG, "esp_ble_gap_stop_scanning");
                esp_ble_gap_stop_scanning();
                xEventGroupSetBits(ble_event, EVT_SEARCH_DEVICE);
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(TAG, "Stop scan successfully");
        break;
    default:
        ESP_LOGW(TAG, "GAP evt %d", event);
        break;
    }
}

void ble_task(void *pvParameters) {
    while (1) {
        if ((xEventGroupWaitBits(ble_event, EVT_READY, false, true, 5000/portTICK_RATE_MS) & EVT_READY) == 0) { 
            goto _continue;
        }
        if ((xEventGroupWaitBits(ble_event, EVT_SEARCH_DEVICE, false, true, 10000/portTICK_RATE_MS) & EVT_SEARCH_DEVICE) == 0) { 
            ESP_LOGI(TAG, "Start scan device");
            esp_ble_gap_start_scanning(10000);
            goto _continue;
        }
        if ((xEventGroupWaitBits(ble_event, EVT_OPEN, false, true, 10000/portTICK_RATE_MS) & EVT_OPEN) == 0) { 
            ESP_LOGI(TAG, "Open connect to "ESP_BD_ADDR_STR " gattc_if: %d", ESP_BD_ADDR_HEX(ble_bda), ble_gattcif);
            esp_err_t ret = esp_ble_gattc_open(ble_gattcif, ble_bda, BLE_ADDR_TYPE_PUBLIC, true);
            ERROR_CHECKE(ret != ESP_OK, "gattc open failed", continue);
            goto _continue;
        }
        if ((xEventGroupWaitBits(ble_event, EVT_SEARCH_SERVICE, false, true, 10000/portTICK_RATE_MS) & EVT_SEARCH_SERVICE) == 0) { 
            ESP_LOGI(TAG, "Search service "ESP_BD_ADDR_STR " gattc_if: %d", ESP_BD_ADDR_HEX(ble_bda), ble_gattcif);
            esp_err_t ret = esp_ble_gattc_search_service(ble_gattcif, ble_bda, NULL);
            ERROR_CHECKE(ret != ESP_OK, "search service failed", return ret);
            goto _continue;
        }
        hid_read_device_services();
_continue:
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

esp_err_t ble_init_security(void) {
    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    return ESP_OK;
}

esp_err_t ble_init(void) {
    esp_err_t ret = ESP_FAIL;
    ble_event = xEventGroupCreate();
    xEventGroupClearBits(ble_event, EVT_READY);
    xEventGroupClearBits(ble_event, EVT_SEARCH_DEVICE);
    xEventGroupClearBits(ble_event, EVT_OPEN);
    xEventGroupClearBits(ble_event, EVT_CLOSE);
    xEventGroupClearBits(ble_event, EVT_SEARCH_SERVICE);
    xEventGroupClearBits(ble_event, EVT_READ);
    xEventGroupClearBits(ble_event, EVT_WRITE);
    xEventGroupClearBits(ble_event, EVT_REGISTER);
    ret = esp_bt_controller_get_status();
    ERROR_CHECKE( ret != ESP_BT_CONTROLLER_STATUS_IDLE, "bt controller not idle", return ret);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ERROR_CHECKE( ret != ESP_OK, "initialize controller failed", return ret);
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ERROR_CHECKE( ret != ESP_OK, "enable controller failed", return ret);
    ret = esp_bluedroid_get_status();
    ERROR_CHECKE( ret != ESP_BLUEDROID_STATUS_UNINITIALIZED, "bt bluedroid not idle", return ret);
    ret = esp_bluedroid_init();
    ERROR_CHECKE( ret != ESP_OK, "init bluedroid failed", return ret);
    ret = esp_bluedroid_enable();
    ERROR_CHECKE( ret != ESP_OK, "enable bluedroid failed", return ret);
    // LOGD("BLUFI VERSION %04x", esp_blufi_get_version());
    ERROR_CHECKE( esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED, "bluedroid disable", return ESP_FAIL);
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    ERROR_CHECKE( ret != ESP_OK, "gap register failed", return ret);
    ERROR_CHECKE( esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED, "bluedroid disable", return ESP_FAIL);
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    ERROR_CHECKE( ret != ESP_OK, "gattc register failed", return ret);
    ret = esp_ble_gattc_app_register(0);
    ERROR_CHECKE( ret != ESP_OK, "gattc app register failed", return ret);
    ret = esp_ble_gatt_set_local_mtu(200);
    ERROR_CHECKE( ret != ESP_OK, "set local MTU failed", return ret);
    //ble_init_security();
    xTaskCreate(&ble_task, "ble_task", 6 * 1024, NULL, 5, NULL);
    ESP_LOGI(TAG, "BLE start...");
    return ESP_OK;
}