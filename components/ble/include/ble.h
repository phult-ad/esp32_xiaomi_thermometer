#ifndef _BLE_H_
#define _BLE_H_

#include "esp_err.h"
#include "esp_gatt_defs.h"

typedef enum {
    MI_EVENT_INIT,
    MI_EVENT_SCAN,
    MI_EVENT_CONNECT,
    MI_EVENT_DISCONNECT,
    MI_EVENT_DATA
} mi_event_t;

typedef struct hid_dev_data_read_t {
    uint8_t             *data;
    uint16_t            len;
    esp_gatt_status_t   status;
} hid_dev_data_read_t;

esp_err_t ble_init(void);
#endif