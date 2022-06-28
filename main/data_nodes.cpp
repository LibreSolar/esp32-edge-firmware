/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST
#include "data_nodes.h"
#include "esp_system.h"
#include "esp_log.h"
#include "../lib/thingset/src/thingset.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include "string.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
// assumption that config data is smaller than 1024 bytes
#define BUFFER_SIZE 1024;

static const char *TAG = "config_nodes";

EmoncmsConfig emon_config;
MqttConfig mqtt_config;
GeneralConfig general_config;

char device_id[9];
const char manufacturer[] = "Libre Solar";
char firmware_version[32];

static DataNode data_nodes[] = {
    TS_NODE_PATH(ID_INFO, "info", 0, NULL),

    TS_NODE_STRING(0x19, "DeviceID", device_id, sizeof(device_id),
        ID_INFO, TS_ANY_R | TS_MKR_W, 0),

    TS_NODE_STRING(0x1A, "Manufacturer", manufacturer, sizeof(manufacturer),
        ID_INFO, TS_ANY_R | TS_MKR_W, 0),

    TS_NODE_STRING(0x1B, "FirmwareVersion", firmware_version, sizeof(firmware_version),
        ID_INFO, TS_ANY_R | TS_MKR_W, 0),

    TS_NODE_PATH(ID_CONF, DATA_NODE_CONF, 0, NULL),

    TS_NODE_PATH(ID_CONF_GENERAL, DATA_NODE_GENERAL, ID_CONF, &save_general),

    TS_NODE_STRING(0x32, "WifiSSID", general_config.wifi_ssid, STRING_LEN,
        ID_CONF_GENERAL, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x33, "WifiPassword", general_config.wifi_password, STRING_LEN,
        ID_CONF_GENERAL, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x34, "MdnsHostname", general_config.mdns_hostname, STRING_LEN,
        ID_CONF_GENERAL, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_BOOL(0x35, "TsUseCan", &(general_config.ts_can_active),
        ID_CONF_GENERAL, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_BOOL(0x36, "TsUseSerial", &(general_config.ts_serial_active),
        ID_CONF_GENERAL, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_PATH(ID_CONF_EMONCMS, DATA_NODE_EMONCMS, ID_CONF, &save_emon),

    TS_NODE_BOOL(0x38, "Activate", &(emon_config.active),
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x39, "Hostname", emon_config.emoncms_hostname, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x3A, "Apikey", emon_config.api_key, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x3B, "Url", emon_config.url, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x3C, "SerialNode", emon_config.serial_node, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x3D, "MPPT", emon_config.mppt, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x3E, "BMS", emon_config.bms, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x3F, "Port", emon_config.port, STRING_LEN,
        ID_CONF_EMONCMS, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_PATH(ID_CONF_MQTT, DATA_NODE_MQTT, ID_CONF, &save_mqtt),

    TS_NODE_BOOL(0x41, "Activate", &(mqtt_config.active),
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x42, "BrokerHostname", mqtt_config.broker_hostname, STRING_LEN,
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_BOOL(0x43, "UseSSL", &(mqtt_config.use_ssl),
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_BOOL(0x44, "UseBrokerAuth", &(mqtt_config.use_broker_auth),
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x45, "Username", mqtt_config.username, STRING_LEN,
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_STRING(0x46, "Password", mqtt_config.password, STRING_LEN,
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT32(0x47, "PubInterval", &(mqtt_config.pub_interval),
        ID_CONF_MQTT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_PATH(ID_EXEC, "rpc", 0, NULL),

    TS_NODE_EXEC(0xE1, "x-reset", &reset_device, ID_EXEC, TS_ANY_RW),
};

ThingSet ts(data_nodes, sizeof(data_nodes)/sizeof(DataNode));

/*
* String array to loop over
*/
const char *nodes[] = {DATA_NODE_GENERAL, DATA_NODE_EMONCMS, DATA_NODE_MQTT, NULL};

void data_nodes_init()
{
    uint64_t id64 = 0;
    // MAC Address of WiFi Station equals base adress
    esp_read_mac(((uint8_t *) &id64) + 2, ESP_MAC_WIFI_STA);
    id64 &= ~((uint64_t) 0xFFFFFFFF << 32);
    id64 += ((uint64_t)LIBRE_SOLAR_TYPE_ID) << 32;
    uint64_to_base32(id64, device_id, sizeof(device_id));

    const esp_app_desc_t *description = esp_ota_get_app_description();
    strncpy(firmware_version, description->version, sizeof(firmware_version));

    esp_err_t ret = nvs_flash_init_partition(PARTITION);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to init config partition");
        //continueing makes no sense at this point
        esp_restart();
    }

    nvs_handle_t handle;
    ret = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to init config partition");
        //continueing makes no sense at this point
        esp_restart();
    }

    nvs_iterator_t it =  nvs_entry_find(PARTITION, NAMESPACE, NVS_TYPE_BLOB);
    if (it == NULL) {
        // Never written a blob to NVS after last flash erase
        config_nodes_load_kconfig();
        save_mqtt();
        save_general();
        save_emon();
    } else {
        config_nodes_load();
    }
}

char *process_ts_request(uint8_t *req, uint32_t query_size, uint8_t can_address, uint32_t *block_len)
{
    char * r = (char *) req;
    if (r[strlen(r) - 1] == '\n') {
        r[strlen(r) - 1] = '\0';
    }
    size_t res_len = BUFFER_SIZE;
    char buf[res_len];
    int len = ts.process((uint8_t *) r, strlen(r), (uint8_t *) buf, res_len);
    if (len == 0) {
        return NULL;
    }
    char *resp = (char *) malloc(len + 1);
    if (resp == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for TSResponse");
        return NULL;
    }
    memcpy(resp, buf, len);
    resp[len] = '\0';
    *block_len = len;
    return resp;
}

TSResponse *process_local_request(char *req, uint8_t can_address)
{
    if (req[strlen(req) - 1] == '\n') {
        req[strlen(req) - 1] = '\0';
    }
    TSResponse *res = reinterpret_cast<TSResponse*>(malloc(sizeof(TSResponse)));

    res->block = process_ts_request((uint8_t *)req, 0, 0, 0);
    res->data = ts_serial_resp_data(res);

    if (res->block == NULL) {
        ESP_LOGE(TAG, "ThingsetError, unable to process request");
        res->ts_status_code = TS_STATUS_INTERNAL_SERVER_ERR;
        return res;
    }
    res->ts_status_code = ts_serial_resp_status(res);
    return res;
}

void config_nodes_load()
{
    nvs_handle_t handle;
    size_t len = 0;
    esp_err_t ret = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to open nvs from partition");
        return;
    }
    for (const char **node = nodes; *node != NULL; node++) {
        nvs_get_blob(handle, *node, NULL, &len);
        uint8_t *buf = (uint8_t *) malloc(len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Unable to allocate buffer for config");
            break;
        }
        nvs_get_blob(handle, *node, buf, &len);
        buf[len] = '\0';
        ESP_LOGD(TAG, "Content of NVS blob: %s", buf);

        char *ts_request = build_query(TS_PATCH, (char *) *node, (char *) buf);

        size_t res_len = 16;
        uint8_t response[res_len];
        ts.process((uint8_t *) ts_request, strlen(ts_request), response, res_len);
        ESP_LOGD(TAG, "TS Response: %s", response);
        free(buf);
        free(ts_request);
    }
}
static void reset_cb(void* arg)
{
    esp_restart();
}
void reset_device()
{
    const esp_timer_create_args_t reset_timer_args = {
            .callback = &reset_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "one-shot"
    };
    esp_timer_handle_t reset_timer;
    ESP_ERROR_CHECK(esp_timer_create(&reset_timer_args, &reset_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(reset_timer, 2*1000000));
}

void save_general()
{
    config_nodes_save(DATA_NODE_GENERAL);
}

void save_mqtt()
{
    config_nodes_save(DATA_NODE_MQTT);
}

void save_emon()
{
    config_nodes_save(DATA_NODE_EMONCMS);
}

void config_nodes_save(const char *node)
{
    size_t res_len = BUFFER_SIZE;
    char response[res_len];
    nvs_handle_t handle;
    esp_err_t ret = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to open nvs from partition");
        return;
    }
    char *ts_request = build_query(TS_GET, (char*) node, NULL);
    int len = ts.process((uint8_t *) ts_request, strlen(ts_request), (uint8_t *) response, res_len);
    ESP_LOGD(TAG, "Got response to query: %s", response);
    TSResponse res;
    res.block = response;
    char *json_start = ts_serial_resp_data(&res);
    len = len - (json_start - response);
    ret = nvs_set_blob(handle, node, json_start, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to write to NVS, Error: %d", ret);
    } else {
        nvs_commit(handle);
    }
    free(ts_request);
    nvs_close(handle);
}

void config_nodes_load_kconfig()
{
    strncpy(general_config.wifi_ssid, CONFIG_WIFI_SSID, sizeof(general_config.wifi_ssid));
    strncpy(general_config.wifi_password, CONFIG_WIFI_PASSWORD, sizeof(general_config.wifi_password));
    strncpy(general_config.mdns_hostname, CONFIG_DEVICE_HOSTNAME, sizeof(general_config.mdns_hostname));
    #ifdef CONFIG_THINGSET_CAN
    general_config.ts_can_active = CONFIG_THINGSET_CAN;
    #else
    general_config.ts_can_active = false;
    #endif
    #ifdef CONFIG_THINGSET_SERIAL
    general_config.ts_serial_active = CONFIG_THINGSET_SERIAL;
    #else
    general_config.ts_serial_active = false;
    #endif

    #ifdef CONFIG_THINGSET_MQTT
    mqtt_config.active = CONFIG_THINGSET_MQTT;
    #else
    mqtt_config.active = false;
    #endif
    strncpy(mqtt_config.broker_hostname, CONFIG_THINGSET_MQTT_BROKER_URI, sizeof(mqtt_config.broker_hostname));
    #ifdef CONFIG_THINGSET_MQTT_TLS
    mqtt_config.use_ssl = CONFIG_THINGSET_MQTT_TLS;
    #else
    mqtt_config.use_ssl = false;
    #endif
    #ifdef CONFIG_THINGSET_MQTT_AUTHENTICATION
    mqtt_config.use_broker_auth = CONFIG_THINGSET_MQTT_AUTHENTICATION;
    #else
    mqtt_config.use_broker_auth = false;
    #endif
    strncpy(mqtt_config.username, CONFIG_THINGSET_MQTT_USER, sizeof(mqtt_config.username));
    strncpy(mqtt_config.password, CONFIG_THINGSET_MQTT_PASS, sizeof(mqtt_config.password));
    mqtt_config.pub_interval = CONFIG_THINGSET_MQTT_PUBLISH_INTERVAL;

    #ifdef CONFIG_EMONCMS
    emon_config.active = CONFIG_EMONCMS;
    #else
    emon_config.active = false;
    #endif
    strncpy(emon_config.emoncms_hostname, CONFIG_EMONCMS_HOST, sizeof(emon_config.emoncms_hostname));
    strncpy(emon_config.port, CONFIG_EMONCMS_PORT, sizeof(emon_config.port));
    strncpy(emon_config.url, CONFIG_EMONCMS_URL, sizeof(emon_config.url));
    strncpy(emon_config.api_key, CONFIG_EMONCMS_APIKEY, sizeof(emon_config.api_key));
    strncpy(emon_config.serial_node, CONFIG_EMONCMS_NODE_SERIAL, sizeof(emon_config.serial_node));
    strncpy(emon_config.mppt, CONFIG_EMONCMS_NODE_MPPT, sizeof(emon_config.mppt));
    strncpy(emon_config.bms, CONFIG_EMONCMS_NODE_BMS, sizeof(emon_config.bms));
}

char *build_query(uint8_t method, char *node, char *payload)
{
    TSUriElems params;
    char base_node[32] = {"conf/"};
    params.ts_payload = payload;
    params.ts_target_node = strcat(base_node, node);
    char *ts_request = (char *) ts_build_query_serial(method, &params, 0);
    //eliminate \n termination used for serial requests
    ts_request[strlen(ts_request) -1] = '\0';
    ESP_LOGD(TAG, "Build query to patch node values: %s", ts_request);
    return ts_request;
}

void uint64_to_base32(uint64_t in, char *out, size_t size)
{
    const char alphabet[] = "0123456789abcdefghjkmnpqrstvwxyz";
    // 13 is the maximum number of characters needed to encode 64-bit variable to base32
    int len = (size > 13) ? 13 : size;

    // find out actual length of output string
    for (int i = 0; i < len; i++) {
        if ((in >> (i * 5)) == 0) {
            len = i;
            break;
        }
    }

    for (int i = 0; i < len; i++) {
        out[len-i-1] = alphabet[(in >> (i * 5)) % 32];
    }
    out[len] = '\0';
}

#endif