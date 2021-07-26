/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ts_client.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"

#ifndef UNIT_TEST

#include "ts_serial.h"
#include "can.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "data_nodes.h"

static const char *TAG = "ts_client";

static TSDevice *devices[10];
extern char device_id[9];
extern GeneralConfig general_config;

void ts_devices_init()
{
    // Add self to devices
    devices[0] = (TSDevice *) malloc(sizeof(TSDevice));
    devices[0]->ts_device_id = device_id;
    devices[0]->ts_name = "self";
    devices[0]->can_address = 0;
    devices[0]->send = &process_ts_request;
    devices[0]->build_query = &ts_build_query_serial;
    devices[0]->ts_resp_data = &ts_serial_resp_data;
    devices[0]->ts_resp_status = &ts_serial_resp_status;
}

void ts_devices_scan_serial()
{
    int num = 1;
    while (devices[num] != NULL && num < sizeof(devices)) {
        num++;
    }

    // scan serial connection
    if (num < sizeof(devices) && general_config.ts_serial_active) {
        devices[num] = (TSDevice *) calloc(1, sizeof(TSDevice));
        int err = ts_serial_scan_device_info(devices[num]);
        if (err) {
            ts_remove_device(devices[num]);
            devices[num] = NULL;
        }
    }
}

void ts_devices_add_can(uint8_t can_addr)
{
    int num = 1;
    while (devices[num] != NULL && num < sizeof(devices)) {
        num++;
    }

    if (num < sizeof(devices) && general_config.ts_can_active) {
        devices[num] = (TSDevice *) calloc(1, sizeof(TSDevice));
        devices[num]->can_address = can_addr;
    }
}

char *ts_get_device_list()
{
    cJSON *obj = cJSON_CreateObject();
    int i = 0;
    while (devices[i] != NULL) {
        if (devices[i]->can_address > 0 && devices[i]->ts_device_id == NULL) {
            // device information not yet obtained
            int err = ts_can_scan_device_info(devices[i]);
            if (!err) {
                cJSON *id = cJSON_CreateString(devices[i]->ts_device_id);
                cJSON_AddItemToObject(obj, devices[i]->ts_name, id);
            }
            else {
                ts_remove_device(devices[i]);
                devices[i] = NULL;
            }
        }
        i++;
    }
    char *names_string = NULL;
    if (i > 0) {
        names_string = cJSON_Print(obj);
        cJSON_Delete(obj);
    } else {
        // hackky solution so that free() can always be called on names_string
        const char msg[] = "";
        names_string = (char *) malloc(sizeof(msg)+1);
        strncpy(names_string, msg, sizeof(msg)+1);
    }
    return names_string;
}

TSDevice *ts_get_device(char *device_id)
{
    for (int i = 0; i < sizeof(devices); i++) {
        if (devices[i] != NULL && devices[i]->ts_device_id != NULL) {
            if (strcmp(devices[i]->ts_device_id, device_id) == 0) {
                return devices[i];
            }
        }
    }
    return NULL;
}

TSDevice *ts_get_can_device(uint8_t can_addr)
{
    for (int i = 0; devices[i] != NULL && i < sizeof(devices); i++) {
        if (devices[i]->can_address == can_addr) {
            return devices[i];
        }
    }
    return NULL;
}

int ts_parse_device_info(cJSON *json, TSDevice *device)
{
    size_t ts_string_len = strlen(cJSON_GetStringValue(cJSON_GetObjectItem(json, "DeviceType")));
    device->ts_name = (char *) malloc(ts_string_len+1);
    if (device->ts_name == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for device name");
        return -1;
    }
    strcpy(device->ts_name, cJSON_GetStringValue(cJSON_GetObjectItem(json, "DeviceType")));
    ts_string_len = strlen(cJSON_GetStringValue(cJSON_GetObjectItem(json, "DeviceID")));
    device->ts_device_id = (char *) malloc(ts_string_len+1);
    if (device->ts_device_id == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for device id");
        return -1;
    }
    strcpy(device->ts_device_id , cJSON_GetStringValue(cJSON_GetObjectItem(json, "DeviceID")));
    ESP_LOGI(TAG, "Got device with ID: %s!", device->ts_device_id);

    return 0;
}

void ts_remove_device(TSDevice *device)
{
    if (device == NULL) {
        return;
    }
    if (device->ts_name != NULL) {
        free(device->ts_name);
    }
    if (device->ts_device_id != NULL) {
        free(device->ts_device_id);
    }
    free(device);
}

#endif

// wrapper for strlen() to check for NULL
int strlen_null(char *r)
{
    if (r != NULL) {
        return(strlen(r));
    } else {
        return 0;
    }
}

char *exec_or_create(char *node)
{
    if (strstr(node, "auth") != NULL ||
        strstr(node, "exec") != NULL ||
        strstr(node, "dfu") != NULL) {
        return "!";
    } else {
        return "+";
    }
}

void ts_parse_uri(const char *uri, TSUriElems *params)
{
    params->ts_list_subnodes = -1;
    params->ts_device_id = NULL;
    params->ts_target_node = NULL;

    if (uri == NULL || uri[0] == '\0') {
        ESP_LOGE(TAG, "Got invalid uri");
        return;
    }
    params->ts_list_subnodes = uri[strlen(uri)-1] == '/' ? 0 : 1;

    // copy uri so we can safely modify
    char *temp_uri = (char *) malloc(strlen(uri)+1);
    if (temp_uri == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for temp_uri");
        return;
    }
    strcpy(temp_uri, uri);
    // extract device ID
    int i = 0;
    while (temp_uri[i] != '\0' && temp_uri[i] != '/') {
        i++;
    }
    // replace '/' so we have two null-terminated strings
    temp_uri[i] = '\0';
    params->ts_device_id = temp_uri;
    // this points either to '\0' aka NULL or the rest of the string
    params->ts_target_node = temp_uri + i + 1;
    ESP_LOGD(TAG, "Got URI %s", uri);
    ESP_LOGD(TAG, "Device_id: %s", params->ts_device_id);
    ESP_LOGD(TAG, "Target Node: %s", params->ts_target_node);
    ESP_LOGD(TAG, "List the sub nodes: %s", params->ts_list_subnodes == 0 ? "yes" : "no");
}

void *ts_build_query_serial(uint8_t ts_method, TSUriElems *params, uint32_t *query_size)
{
    if (params == NULL) {
        return NULL;
    }
    // calculate size to allocate buffer
    int nbytes = 3;  // method + termination + zero termination
    if (strlen_null(params->ts_target_node) == 0 && params->ts_list_subnodes == 0) {
        nbytes++;    // '/' at the end of query
    }
    nbytes += strlen_null(params->ts_target_node);
    if (params->ts_payload != NULL) {
        // additional whitespace between uri and array/json
        nbytes += strlen_null(params->ts_payload) +1;
    }
    char *ts_query = (char *) malloc(nbytes);

    // Special case when devices using the CAN Bus with TEXT-Mode,
    // they must not send the termination bytes used with UART
    if (query_size != NULL) {
        *query_size = nbytes - 2;
    }

    if (ts_query == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for ts_query");
        return NULL;
    }
    int pos = 0;
    switch (ts_method){
        case TS_GET:
            ts_query[pos] = '?';
            break;
        case TS_POST:
            ts_query[pos] = *(exec_or_create(params->ts_target_node));
            break;
        case TS_PATCH:
            ts_query[pos] = '=';
            break;
        case TS_DELETE:
            ts_query[pos] = '-';
            break;
        default:
            ts_query[pos] = '\0';
            return ts_query;    // nothing else to do here
    }
    pos++;
    // corner case for getting device categories
    if (strlen(params->ts_target_node) == 0 && params->ts_list_subnodes == 0) {
        ts_query[pos] = '/';
        pos++;
    } else {
        strncpy(ts_query + pos, params->ts_target_node, strlen_null(params->ts_target_node));
        pos += strlen_null(params->ts_target_node);
    }
    if (params->ts_payload != NULL) {
        ts_query[pos] = ' ';
        pos++;
        strncpy(ts_query + pos, params->ts_payload, strlen_null(params->ts_payload));
        pos += strlen_null(params->ts_payload);
    }
    //terminate query properly
    ts_query[pos] = '\n';
    pos++;
    ts_query[pos] = '\0';
    ESP_LOGD(TAG, "Build query String: %s !", ts_query);
    return (void *) ts_query;
}

#ifndef UNIT_TEST

TSResponse *ts_execute(const char *uri, char *content, int http_method)
{
    uint8_t ts_method;
    switch (http_method) {
    case HTTP_DELETE:
        ts_method = TS_DELETE;
        break;
    case HTTP_GET:
        ts_method = TS_GET;
        break;
    case HTTP_POST:
        ts_method = TS_POST;
        break;
    case HTTP_PATCH:
        ts_method = TS_PATCH;
        break;
    default:
        ts_method = TS_GET;
        break;
    }
    TSUriElems params;
    params.ts_payload = content;
    params.ts_list_subnodes = -1;
    ts_parse_uri(uri, &params);
    TSDevice *device = ts_get_device(params.ts_device_id);
    if (device == NULL) {
        ESP_LOGD(TAG, "No Device, freeing query string and device id");
        heap_caps_free(params.ts_device_id);
        return NULL;
    }
    uint32_t query_size;
    char *ts_query_string = device->build_query(ts_method, &params, &query_size);

    TSResponse *res = (TSResponse *) malloc(sizeof(TSResponse));
    if (res == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for ts response");
        heap_caps_free(ts_query_string);
        heap_caps_free(params.ts_device_id);
        return NULL;
    }
    // send is already a pointer to the correct function
    uint32_t block_len = 0;
    res->block = device->send((uint8_t *)ts_query_string, query_size, device->can_address, &block_len);
    res->block_len = block_len;
    if (res->block == NULL) {
        ESP_LOGI(TAG, "No Response, freeing query string and device id");
        heap_caps_free(ts_query_string);
        heap_caps_free(params.ts_device_id);
        heap_caps_free(res);
        return NULL;
    }

    //call status code first, data will be overwritten when device is using CAN binary methods
    res->ts_status_code = device->ts_resp_status(res);
    res->data = device->ts_resp_data(res);

    heap_caps_free(ts_query_string);
    heap_caps_free(params.ts_device_id);

    return res;
}

char *ts_serial_resp_data(TSResponse *res)
{
    if (res->block[0] == ':') {
        char *pos = strstr(res->block, ". ");
        if (pos != NULL) {
            return pos + 2;
        }
    }
    return NULL;
}

uint8_t ts_serial_resp_status(TSResponse *res)
{
    unsigned int status_code = -1;
    sscanf(res->block, ":%X ", &status_code);
    return status_code;
}

#endif  //UNIT_TEST
