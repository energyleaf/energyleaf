#include "cluster.h"
#include "esp_zigbee_type.h"
#include "zigbee.h"
#include "esp_log.h"
#include "zcl/esp_zigbee_zcl_common.h"
#include "zcl/esp_zigbee_zcl_basic.h"

#define TAG "CLUSTER"

// Attribute storage
static float value_in = NAN;
static float value_out = NAN;
static float value_current = NAN;

static esp_zb_cluster_list_t *zb_clusters;
static esp_zb_ep_list_t *zb_ep_list;

esp_zb_ep_list_t *cluster_init(void) {
    ESP_LOGD(TAG, "Initializing clusters...");

    zb_clusters = esp_zb_zcl_cluster_list_create();

    // Basic Cluster
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = 3,
        .power_source = ESP_ZB_AF_NODE_POWER_SOURCE_RECHARGEABLE_BATTERY, //battery powered
    };

    esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(&basic_cfg);

    esp_zb_basic_cluster_add_attr(basic_cluster,
                                  ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(basic_cluster,
                                  ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  MODEL_IDENTIFIER);
    esp_zb_basic_cluster_add_attr(basic_cluster,
                                  ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID,
                                  SW_BUILD_ID);

    esp_zb_cluster_list_add_basic_cluster(zb_clusters, basic_cluster,
                                          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(zb_clusters,
                                             esp_zb_identify_cluster_create(NULL),
                                             ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Custom Cluster
    esp_zb_attribute_list_t *custom_cluster = esp_zb_zcl_attr_list_create(CUSTOM_CLUSTER_ID);

    esp_zb_custom_cluster_add_custom_attr(
        custom_cluster,
        ATTR_ID_VALUE_IN,
        ESP_ZB_ZCL_ATTR_TYPE_SINGLE,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
        &value_in
    );

    esp_zb_custom_cluster_add_custom_attr(
        custom_cluster,
        ATTR_ID_VALUE_OUT,
        ESP_ZB_ZCL_ATTR_TYPE_SINGLE,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
        &value_out
    );

    esp_zb_custom_cluster_add_custom_attr(
        custom_cluster,
        ATTR_ID_VALUE_CURRENT,
        ESP_ZB_ZCL_ATTR_TYPE_SINGLE,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
        &value_current
    );

    esp_zb_cluster_list_add_custom_cluster(zb_clusters, custom_cluster,
                                           ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Endpoint
    zb_ep_list = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t ep_config = {
        .endpoint = HA_ENDPOINT,
        .app_profile_id = HA_PROFILE_ID,
        .app_device_id = HA_DEVICE_ID,
        .app_device_version = 0
    };

    esp_zb_ep_list_add_ep(zb_ep_list, zb_clusters, ep_config);

    ESP_LOGD(TAG, "Clusters initialized: Basic + Custom (double attributes)");

    return zb_ep_list;
}

void cluster_update_values(float in, float out, float current) {
    value_in = in;
    value_out = out;
    value_current = current;

    esp_zb_zcl_set_attribute_val(HA_ENDPOINT, CUSTOM_CLUSTER_ID,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ATTR_ID_VALUE_IN, &value_in, false);
    esp_zb_zcl_set_attribute_val(HA_ENDPOINT, CUSTOM_CLUSTER_ID,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ATTR_ID_VALUE_OUT, &value_out, false);
    esp_zb_zcl_set_attribute_val(HA_ENDPOINT, CUSTOM_CLUSTER_ID,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ATTR_ID_VALUE_CURRENT, &value_current, false);

    ESP_LOGD(TAG, "Updated cluster values: in=%.2f, out=%.2f, current=%.2f",
             in, out, current);
}
