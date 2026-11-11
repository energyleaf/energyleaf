#include "zigbee.h"
#include "esp_log.h"
#include "esp_zigbee_core.h"
#include "esp_err.h"
#include "nvs.h"
#include <stdbool.h>

#include "sensor.h"

#define TAG "ZB_EVENTS"

#define ZB_NVS_NAMESPACE "zigbee"
#define ZB_NVS_KEY_PROVISIONED "provisioned"

static bool zb_is_provisioned(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(ZB_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return false;
    }

    uint8_t val = 0;
    err = nvs_get_u8(h, ZB_NVS_KEY_PROVISIONED, &val);
    nvs_close(h);
    if (err == ESP_OK) {
        return val == 1;
    }
    return false;
}

static void zb_set_provisioned(bool provisioned) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(ZB_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u8(h, ZB_NVS_KEY_PROVISIONED, provisioned ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS set failed: %s", esp_err_to_name(err));
        nvs_close(h);
        return;
    }

    err = nvs_commit(h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS commit failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGD(TAG, "Provisioned flag set to: %s",
                 provisioned ? "true" : "false");
    }
    nvs_close(h);
}

void zigbee_event_init(void) {
    ESP_LOGD(TAG, "Initializing Zigbee events...");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    esp_zb_app_signal_type_t sig_type = *signal_struct->p_app_signal;
    esp_err_t status = signal_struct->esp_err_status;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGD(TAG, "Zigbee stack initialized");
        if (zb_is_provisioned()) {
            ESP_LOGD(TAG, "Start network steering...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            ESP_LOGD(TAG, "Press button to start network steering");
        }
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (status == ESP_OK) {
            ESP_LOGD(TAG, "Joined Zigbee network successfully");
            zb_set_provisioned(true);
        } else {
            if (zb_is_provisioned()) {
                ESP_LOGW(TAG, "Was previously connected, try to reconnect...");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGW(TAG, "Failed to join Zigbee network (status: %d)", status);
            }
        }
        break;

    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGD(TAG, "Device left the Zigbee network");
        zb_set_provisioned(false);
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        ESP_LOGD(TAG, "Network steering started");
        if (status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);

            zb_set_provisioned(true);

            ESP_LOGD(TAG,
                     "Joined network successfully "
                     "(Extended PAN ID: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, "
                     "PAN ID: 0x%04X, Channel: %d, Short Address: 0x%04X)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5],
                     extended_pan_id[4], extended_pan_id[3], extended_pan_id[2],
                     extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(),
                     esp_zb_get_current_channel(),
                     esp_zb_get_short_address());
        } else {
            ESP_LOGW(TAG,
                     "Network steering was not successful (status: %d)",
                     status);
        }
        break;

    case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
        if (!zb_busy) {
            esp_zb_sleep_now();
        }
        break;

    default:
        ESP_LOGD(TAG, "Unhandled Zigbee signal: %d, status: %d", sig_type, status);
        break;
    }
}
