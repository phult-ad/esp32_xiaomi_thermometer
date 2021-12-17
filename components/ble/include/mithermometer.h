#ifndef _MI_THERMOMETER_H_
#define _MI_THERMOMETER_H_

#include "esp_err.h"
#include "esp_gatt_defs.h"

typedef enum {
    MI_INIT,
    MI_SCAN,
    MI_CONNECT,
    MI_SEARCH_SERVICE,
    MI_DISCONNECT,
    MI_READ_DEVICE,
    MI_READ_MODEL,
    MI_READ_SERIAL,
    MI_READ_FW_VER,
    MI_READ_HW_VER,
    MI_READ_SW_VER,
    MI_READ_BATTERY,
    MI_READ_TEMP_HUM,
    MI_IDLE,
} mi_state_t;

esp_err_t mi_init(void);
#endif