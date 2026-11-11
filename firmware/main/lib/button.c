#include "button.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_zigbee_core.h"
#include "esp_sleep.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "portmacro.h"

#define TAG "BUTTON"

#define BUTTON_GPIO 9
#define BUTTON_STARTUP_IGNORE_MS 2000

static int64_t s_boot_ts_us = 0;
static TaskHandle_t steering_task_handle = NULL;

static void steering_task(void *pvParameter) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int64_t now_us = esp_timer_get_time();
        if ((now_us - s_boot_ts_us) / 1000 < BUTTON_STARTUP_IGNORE_MS) {
            ESP_LOGD(TAG, "Ignore button event during startup window");
            continue;
        }

        ESP_LOGD(TAG, "Try starting Zigbee network steering for 1 minute...");
        if (esp_zb_lock_acquire(pdMS_TO_TICKS(portMAX_DELAY))) {
            ESP_LOGD(TAG, "Zigbee network steering for 1 minute...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            esp_zb_lock_release();
        }
    }
}

static void IRAM_ATTR button_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(steering_task_handle, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void button_init(void) {
    s_boot_ts_us = esp_timer_get_time();
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);

    xTaskCreate(steering_task, "steering_task", 2048, NULL, 10, &steering_task_handle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    gpio_wakeup_enable(BUTTON_GPIO, GPIO_INTR_LOW_LEVEL);

    ESP_LOGD(TAG, "Button on GPIO %d initialized for Zigbee steering", BUTTON_GPIO);
}
