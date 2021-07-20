/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DATA_NODES_H_
#define DATA_NODES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "esp_err.h"
#include "ts_client.h"


#define LIBRE_SOLAR_TYPE_ID 9

/*
 * Partition name
 */
#define PARTITION "config"
#define NAMESPACE "main"


/*
 * Categories / first layer node IDs
 */
#define ID_ROOT     0x00
#define ID_INFO     0x18        // read-only device information (e.g. manufacturer, device ID)
#define ID_CONF     0x30        // configurable settings
#define ID_CONF_GENERAL 0x31
#define ID_CONF_EMONCMS 0x37
#define ID_CONF_MQTT    0x40
#define ID_INPUT    0x60        // input data (e.g. set-points)
#define ID_OUTPUT   0x70        // output data (e.g. measurement values)
#define ID_REC      0xA0        // recorded data (history-dependent)
#define ID_CAL      0xD0        // calibration
#define ID_EXEC     0xE0        // function call
#define ID_AUTH     0xEA
#define ID_PUB      0xF0        // publication setup
#define ID_SUB      0xF1        // subscription setup
#define ID_LOG      0x100       // access log data

#define STRING_LEN 128          // size of allocated strings in config
#define DATA_NODE_CONF        "conf"
#define DATA_NODE_GENERAL     "general"
#define DATA_NODE_EMONCMS     "emoncms"
#define DATA_NODE_MQTT        "mqtt"

/*
 * Publish/subscribe channels
 */
#define PUB_SER     (1U << 0)   // UART serial
#define PUB_CAN     (1U << 1)   // CAN bus
#define PUB_NVM     (1U << 2)   // data that should be stored in EEPROM

/**
 * Initializes and reads config nodes from nvs
 */
void data_nodes_init();

/**
 * Process incoming Thingset requests. Has CAN address to match
 * send function from TSDevice struct as it is used only internally,
 * but could also be used by a UART/CAN task to process ThingSet requests in general
 */
char *process_ts_request(uint8_t *req, uint32_t query_size, uint8_t can_address, uint32_t *block_len);

void uint64_to_base32(uint64_t in, char *out, size_t size);

/**
 * sets of a timer to reset the device, lets http request return gracefully before restarting
 */
void reset_device();

/*
 * Wrapper for saving nodes, used as callbacks. Always restarts the device after saving
 */
void save_mqtt();

void save_emon();

void save_general();

/**
 * Reads config from Kconfig values
 */
void config_nodes_load_kconfig();

/**
 * Reads config from NVS values
 */
void config_nodes_load();

/**
 * Writes config to NVS
 */
void config_nodes_save(const char *node);

/**
 * Wrapper for serial query builder. Eliminates newline termination used for serial communication
 */
char *build_query(uint8_t method, char *node, char *payload);

/**
 * Structs to hold config data
 */
typedef struct {
    char wifi_ssid[STRING_LEN];
    char wifi_password[STRING_LEN];
    char mdns_hostname[STRING_LEN];
    bool ts_can_active;
    bool ts_serial_active;
} GeneralConfig;

typedef struct {
    bool active;
    char emoncms_hostname[STRING_LEN];
    char port[STRING_LEN];
    char url[STRING_LEN];
    char api_key[STRING_LEN];
    char serial_node[STRING_LEN];
    char mppt[STRING_LEN];
    char bms[STRING_LEN];
} EmoncmsConfig;

typedef struct {
    bool active;
    char broker_hostname[STRING_LEN];
    bool use_ssl;
    bool use_broker_auth;
    char username[STRING_LEN];
    char password[STRING_LEN];
    uint32_t pub_interval;
} MqttConfig;

#ifdef __cplusplus
}
#endif

#endif