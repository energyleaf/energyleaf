#include "watchdog.h"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"

/*
 * WATCHDOG
 * The ESP is software reset every couple of hours.
 * This is to reset the zigbee stack and help with hangups which lead the sensors to stop transmitting data.
 */
#define TAG "WATCHDOG"
#define WATCHDOG_TIMEOUT_MS (12 * 60 * 60 * 1000)

static void watchdog_task(void *pvParameter) {
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS));

        esp_restart();
    }
}

void watchdog_init(void) {
    xTaskCreate(watchdog_task, "watchdog_task", 2048, NULL, 10, NULL);
}
