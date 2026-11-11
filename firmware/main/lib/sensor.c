#include "driver/uart.h"
#include "cluster.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "sml.h"
#include "sensor.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include <math.h>


#if CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif

#define TAG "SML"
#define ESP_XIAO 1

#define SML_UART_PORT UART_NUM_1
#define SENSOR_ACTIVE_PIN GPIO_NUM_21

#if 0 
    #define SML_PIN_TX GPIO_NUM_18
    #define SML_PIN_RX GPIO_NUM_20
    #pragma message "builting for DEVBOARD"
#else
    #define SML_PIN_TX GPIO_NUM_20
    #define SML_PIN_RX GPIO_NUM_18
    #pragma message "building for ESP_XIAO"
#endif

#define BUF_SIZE 1024
#define SML_BAUDRATE 9600
#define SML_UART_QUEUE_SIZE 20

#define SML_DEBUG_LOGGING 0

#define ZIGBEE_SENDING_DELAY 30000

#if SML_DEBUG_LOGGING
    #define SML_LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#else
    #define SML_LOGD(...) do {} while(0)
#endif

static const uint8_t OBIS_ENERGY_IN_TOTAL[]    = {0x01, 0x00, 0x01, 0x08, 0x00, 0xFF};  // 1.8.0
//static const uint8_t OBIS_ENERGY_IN_TARIFF1[]  = {0x01, 0x00, 0x01, 0x08, 0x01, 0xFF};  // 1.8.1
//static const uint8_t OBIS_ENERGY_IN_TARIFF2[]  = {0x01, 0x00, 0x01, 0x08, 0x02, 0xFF};  // 1.8.2

static const uint8_t OBIS_ENERGY_OUT_TOTAL[]   = {0x01, 0x00, 0x02, 0x08, 0x00, 0xFF};  // 2.8.0
//static const uint8_t OBIS_ENERGY_OUT_TARIFF1[] = {0x01, 0x00, 0x02, 0x08, 0x01, 0xFF};  // 2.8.1
//static const uint8_t OBIS_ENERGY_OUT_TARIFF2[] = {0x01, 0x00, 0x02, 0x08, 0x02, 0xFF};  // 2.8.2

static const uint8_t OBIS_POWER_SUM[]     = {0x01, 0x00, 0x10, 0x07, 0x00, 0xFF};  // 16.7.0
static const uint8_t OBIS_POWER_IMPORT[]  = {0x01, 0x00, 0x01, 0x07, 0x00, 0xFF};  // 1.7.0
static const uint8_t OBIS_POWER_ABS[]     = {0x01, 0x00, 0x0F, 0x07, 0x00, 0xFF};  // 15.7.0

static float last_in = NAN;
static float last_out = NAN;
static float last_current = NAN;
static bool has_valid_data = false;

static SemaphoreHandle_t s_data_mutex = NULL;
static QueueHandle_t s_uart_queue = NULL;

static TaskHandle_t s_uart_task_handle = NULL;

static esp_pm_lock_handle_t uart_pm_lock = NULL;
volatile bool zb_busy = false;

static uint8_t s_rx_buf[BUF_SIZE];

static void sml_uart_task(void *arg) {
    bool in_frame = false;
    uart_event_t event;

    while (1) {
        if (!xQueueReceive(s_uart_queue, &event, pdMS_TO_TICKS(50))) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (event.type != UART_DATA) {
            if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL) {
                ESP_LOGW(TAG, "UART overflow, flushing");
                uart_flush_input(SML_UART_PORT);
                xQueueReset(s_uart_queue);
                in_frame = false;
            }
            continue;
        }

        int len = uart_read_bytes(SML_UART_PORT, s_rx_buf, 
                                  (event.size < BUF_SIZE) ? event.size : BUF_SIZE, 0);

        if (len <= 0) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        for (int i = 0; i < len; i++) {
            sml_states_t state = smlState(&s_rx_buf[i]);

            if (!in_frame && state == SML_VERSION) {
                in_frame = true;
                SML_LOGD("SML frame started");
            }

            if (!in_frame) {
                continue;
            }

            if (state == SML_LISTEND) {
                float new_in = NAN, new_out = NAN, new_power = NAN;
                double tmp;
/*
                // Energy In: Try 1.8.0 (total), 1.8.1 (tariff 1), 1.8.2 (tariff 2)
                if (smlOBISCheck(OBIS_ENERGY_IN_TOTAL) ||
                    smlOBISCheck(OBIS_ENERGY_IN_TARIFF1) ||
                    smlOBISCheck(OBIS_ENERGY_IN_TARIFF2)) {
                    smlOBISWh(&tmp);
                    new_in = (float)tmp / 1000;
                    SML_LOGD("Parsed OBIS IN: %.3f kWh", new_in);
                }
                // Energy Out: Try 2.8.0 (total), 2.8.1 (tariff 1), 2.8.2 (tariff 2)
                if (smlOBISCheck(OBIS_ENERGY_OUT_TOTAL) ||
                    smlOBISCheck(OBIS_ENERGY_OUT_TARIFF1) ||
                    smlOBISCheck(OBIS_ENERGY_OUT_TARIFF2)) {
                    smlOBISWh(&tmp);
                    new_out = (float)tmp / 1000;
                    SML_LOGD("Parsed OBIS OUT: %.3f kWh", new_out);
                }
*/
                // Energy In: Try 1.8.0 (total), 1.8.1 (tariff 1), 1.8.2 (tariff 2)
                if (smlOBISCheck(OBIS_ENERGY_IN_TOTAL)) {
                    smlOBISWh(&tmp);
                    new_in = (float)tmp / 1000;
                    SML_LOGD("Parsed OBIS IN: %.3f kWh", new_in);
                }
                // Energy Out: Try 2.8.0 (total), 2.8.1 (tariff 1), 2.8.2 (tariff 2)
                if (smlOBISCheck(OBIS_ENERGY_OUT_TOTAL)) {
                    smlOBISWh(&tmp);
                    new_out = (float)tmp / 1000;
                    SML_LOGD("Parsed OBIS OUT: %.3f kWh", new_out);
                }
                // Power: Try 16.7.0 (sum), 1.7.0 (import), 15.7.0 (absolute)
                if (smlOBISCheck(OBIS_POWER_SUM) ||
                    smlOBISCheck(OBIS_POWER_IMPORT) ||
                    smlOBISCheck(OBIS_POWER_ABS)) {
                    smlOBISW(&tmp);
                    new_power = (float)tmp; //current power is transmitted in W, not kW (unlike the cap which is kWh)
                    SML_LOGD("Parsed OBIS Power: %.3f W", new_power);
                }

                if (!isnan(new_in) || !isnan(new_out) || !isnan(new_power)) {
                    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
                    if (!isnan(new_in)){
                        last_in = new_in;
                        has_valid_data = true;
                    }
                    if (!isnan(new_out)) last_out = new_out;
                    if (!isnan(new_power)) last_current = new_power;

                    xSemaphoreGive(s_data_mutex);
                }
            }

            if (state == SML_FINAL) {
                SML_LOGD("SML frame complete, suspending UART task");
                in_frame = false;
                vTaskSuspend(NULL);
                break;
            }

            if (state == SML_CHECKSUM_ERROR || state == SML_UNEXPECTED) {
                ESP_LOGD(TAG, "SML parse error (state=%d), resetting", state);
                in_frame = false;
            }
        }
    }
}

static void sml_zigbee_task(void *arg) {

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ZIGBEE_SENDING_DELAY));

#if DEVBOARD
        gpio_set_level(SENSOR_ACTIVE_PIN, 1);
#endif

#if CONFIG_PM_ENABLE
        // Single lock per cycle, released after reporting
        esp_pm_lock_acquire(uart_pm_lock);
#endif
        uart_flush_input(SML_UART_PORT);
        xQueueReset(s_uart_queue);
        vTaskResume(s_uart_task_handle);

        // Wait for the UART task to receive a full frame
        // (it suspends itself once SML_FINAL is parsed)
        TickType_t wait_start = xTaskGetTickCount();
        while (eTaskGetState(s_uart_task_handle) != eSuspended &&
               (xTaskGetTickCount() - wait_start) < pdMS_TO_TICKS(1500)) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        // Timeout: frame incomplete / meter silent, force-suspend
        if (eTaskGetState(s_uart_task_handle) != eSuspended) {
            vTaskSuspend(s_uart_task_handle);
        }

#if DEVBOARD
        gpio_set_level(SENSOR_ACTIVE_PIN, 0);
#endif

        xSemaphoreTake(s_data_mutex, portMAX_DELAY);
        bool valid = has_valid_data;
        float in = last_in;
        float out = last_out;
        float current = last_current;
        xSemaphoreGive(s_data_mutex);

        if (valid) {
            ESP_LOGD(TAG, "Updating Zigbee: in=%.3f, out=%.3f, current=%.3f", in, out, current);
            zb_busy = true;
            cluster_update_values(in, out, current);

#if REPORTING
            esp_zb_zcl_report_attr_cmd_t cluster_cmd = {
                .zcl_basic_cmd.dst_addr_u.addr_short = 0x0000,  // coordinator
                .zcl_basic_cmd.dst_endpoint = 1, //coordinator endpoint
                .zcl_basic_cmd.src_endpoint = HA_ENDPOINT,
                .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                .clusterID = CUSTOM_CLUSTER_ID,
                .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
                .attributeID = ATTR_ID_VALUE_IN,
            };
     
            esp_zb_lock_acquire(portMAX_DELAY);

            esp_zb_zcl_report_attr_cmd_req(&cluster_cmd);
            
            cluster_cmd.attributeID = ATTR_ID_VALUE_OUT;
            esp_zb_zcl_report_attr_cmd_req(&cluster_cmd);

            cluster_cmd.attributeID = ATTR_ID_VALUE_CURRENT;
            esp_zb_zcl_report_attr_cmd_req(&cluster_cmd);

            esp_zb_lock_release();
#endif

        } else {
            ESP_LOGD(TAG, "No valid SML data yet, skipping Zigbee update");
        }

#if CONFIG_PM_ENABLE
        // Reports are queued; the 802.15.4 driver holds its own PM locks
        // during radio activity, so light sleep is safe from here on
        esp_pm_lock_release(uart_pm_lock);
#endif

        // Give the Zigbee TX time to finish before clearing zb_busy
        vTaskDelay(pdMS_TO_TICKS(500));

        if (zb_busy) {
            zb_busy = false;
        } //zb_can_sleep will now work
    }
}

void sml_init(void) {
#if CONFIG_PM_ENABLE
    esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "uart", &uart_pm_lock);
#endif

    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

#if DEVBOARD
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << SENSOR_ACTIVE_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
#endif

    uart_config_t uart_config = {
        .baud_rate = SML_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_XTAL,
    };

    uart_driver_install(SML_UART_PORT, BUF_SIZE * 2, 0, 
                        SML_UART_QUEUE_SIZE, &s_uart_queue, 0);
    uart_param_config(SML_UART_PORT, &uart_config);
    uart_set_pin(SML_UART_PORT, SML_PIN_TX, SML_PIN_RX,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(sml_uart_task, "sml_uart_task", 4096, NULL, 5, &s_uart_task_handle);
    vTaskSuspend(s_uart_task_handle);

    xTaskCreate(sml_zigbee_task, "sml_zigbee_task", 2048, NULL, 5, NULL);

    ESP_LOGD(TAG, "SML sensor tasks started");
}
