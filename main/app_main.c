#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "ssd1306.h"
#include "mithermometer.h"
#include "sdkconfig.h"

static const char *TAG = "main";

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("main", ESP_LOG_INFO);
    esp_log_level_set("MI THERMOMETER", ESP_LOG_INFO);

    esp_err_t ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, " IDF:                 %s", IDF_VER);
    oled_ssd1306_init();
    oled_ssd1306_clear_all();
    oled_ssd1306_print(0, "Hello");
    mi_init();

    while (1) {
        //ESP_LOGI(TAG, "Run....");
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
