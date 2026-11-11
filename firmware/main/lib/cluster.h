#pragma once

#include "esp_zigbee_core.h"
#include "zigbee.h"

#define CUSTOM_CLUSTER_ID 0xFF00

#define ATTR_ID_VALUE_IN      0x0001
#define ATTR_ID_VALUE_OUT     0x0002
#define ATTR_ID_VALUE_CURRENT 0x0003

#define MODEL_IDENTIFIER "\x15""A-Energyleaf-Digital"
#define MANUFACTURER_NAME "\x0A""Energyleaf"
#define SW_BUILD_ID "\x05""1.0.0"

esp_zb_ep_list_t *cluster_init(void);
void cluster_update_values(float in, float out, float current);
