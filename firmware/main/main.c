#include "esp_log.h"
#include "esp_zigbee_core.h"
#include "esp_ieee802154.h"
#include "lib/zigbee.h"
#include "lib/button.h"
#include "lib/cluster.h"

#include "lib/watchdog.h"

#include "nvs_flash.h"
#include "lib/sensor.h"

#include "driver/gpio.h"

#if CONFIG_PM_ENABLE
#include "esp_pm.h"
#include "esp_sleep.h"
#endif

#define ESP_XIAO 1

#ifdef ESP_XIAO
#pragma message "building for ESP_XIAO"
#else
#endif

#define TAG "MAIN"

#if CONFIG_PM_ENABLE
static void esp_zb_power_save_init(void) {
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 10,
        .light_sleep_enable = true
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    ESP_ERROR_CHECK(esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF));
}
#endif


static void zigbee_task(void *pvParameters) {
    // Network config
    esp_zb_cfg_t zb_nwk_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
        .install_code_policy = false,
        .nwk_cfg.zed_cfg = {
            .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,
            .keep_alive = 30000,
        },
    };

#if 1 //enable external antenna
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_3) | (1UL << GPIO_NUM_14),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_3, 0);
    vTaskDelay(pdMS_TO_TICKS(150));
    gpio_set_level(GPIO_NUM_14, 1);
#endif

#if CONFIG_PM_ENABLE
    esp_zb_sleep_set_threshold(500);
    esp_zb_sleep_enable(true);
#endif

    esp_zb_init(&zb_nwk_cfg);

    esp_zb_set_tx_power(20);
    esp_zb_secur_network_min_join_lqi_set(0);

    esp_zb_set_rx_on_when_idle(false);

    esp_zb_ep_list_t *ep_list = cluster_init();
    esp_zb_device_register(ep_list);

    esp_zb_start(false);
    ESP_LOGD(TAG, "Zigbee stack started");

    esp_zb_stack_main_loop();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

#if CONFIG_PM_ENABLE
    esp_zb_power_save_init();
#endif

    // Platform
    esp_zb_platform_config_t config = {
        .radio_config = {
            .radio_mode = ZB_RADIO_MODE_NATIVE,
        },
        .host_config = {
            .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
        }
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    zigbee_event_init();

    // Start Zigbee task
    xTaskCreate(zigbee_task, "zigbee_task", 4096, NULL, 5, NULL);

    // Start button task
    button_init();
    ESP_LOGD(TAG, "Zigbee pairing button initialized");

    // Start SML sensor task if needed
    sml_init();
    ESP_LOGD(TAG, "SML sensor initialized");


    watchdog_init();
    ESP_LOGD(TAG, "Watchdog initialized");
}
