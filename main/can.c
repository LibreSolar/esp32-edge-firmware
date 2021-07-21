/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "can.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/twai.h"
#include "driver/gpio.h"

#include "../lib/isotp/isotp.h"
#include "../lib/isotp/isotp_defines.h"

#include "ts_client.h"
#include "ts_cbor.h"
#include "cJSON.h"
static const char *TAG = "can";

bool update_bms_received = false;
bool update_mppt_received = false;

xQueueHandle receive_queue;
#define RECV_QUEUE_SIZE 1
#define ISOTP_BUFSIZE 1000

/* Alloc IsoTpLink statically in RAM */
static IsoTpLink isotp_link;

/* Alloc send and receive buffer statically in RAM */
static uint8_t isotp_recv_buf[ISOTP_BUFSIZE];
static uint8_t isotp_send_buf[ISOTP_BUFSIZE];

uint32_t can_addr_client = 0xF1;     // this device
uint32_t can_addr_server = 0x14;     // select MPPT (14) or BMS (0a)

// buffer for JSON string generated from received data objects via CAN
static char json_buf[500];

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(CONFIG_GPIO_CAN_TX, CONFIG_GPIO_CAN_RX, TWAI_MODE_NORMAL);

DataObject data_obj_bms[] = {
    {0x70, "Bat_V",     {0}, 0},
    {0x71, "Bat_A",     {0}, 0},
    {0x72, "Bat_degC",  {0}, 0},
    {0x76, "IC_degC",   {0}, 0},
    {0x77, "MOSFETs_degC",  {0}, 0},
/*
    {0x03, "Cell1_V",   {0}, 0},
    {0x04, "Cell2_V",   {0}, 0},
    {0x05, "Cell3_V",   {0}, 0},
    {0x06, "Cell4_V",   {0}, 0},
    {0x07, "Cell5_V",   {0}, 0},
    {0x0A, "SOC",       {0}, 0}
*/
};

DataObject data_obj_mppt[] = {
    {0x04, "LoadState",     {0}, 0},
    {0x0F, "SolarMaxDay_W", {0}, 0},
    {0x10, "LoadMaxDay_W",  {0}, 0},
    {0x70, "Bat_V",         {0}, 0},
    {0x71, "Solar_V",       {0}, 0},
    {0x72, "Bat_A",         {0}, 0},
    {0x73, "Load_A",        {0}, 0},
    {0x74, "Bat_degC",      {0}, 0},
    {0x76, "Int_degC",      {0}, 0},
    {0x77, "Mosfet_degC",   {0}, 0},
    {0x78, "ChgState",      {0}, 0},
    {0x79, "DCDCState",     {0}, 0},
    {0x7a, "Solar_A",       {0}, 0},
    {0x7d, "Bat_W",         {0}, 0},
    {0x7e, "Solar_W",       {0}, 0},
    {0x7f, "Load_W",        {0}, 0},
    {0xa0, "SolarInDay_Wh", {0}, 0},
    {0xa1, "LoadOutDay_Wh", {0}, 0},
    {0xa2, "BatChgDay_Wh",  {0}, 0},
    {0xa3, "BatDisDay_Wh",  {0}, 0},
    {0x06, "SOC",           {0}, 0},
    {0xA4, "Dis_Ah",        {0}, 0}
};

static int generate_json_string(char *buf, size_t len, DataObject *objs, size_t num_objs)
{
    union float2bytes { float f; char b[4]; } f2b;     // for conversion of float to single bytes
    int pos = 0;

    for (int i = 0; i < num_objs; i++) {

        if (objs[i].raw_data[0] == 0) {
            continue;
        }

        // print data object ID
        if (pos == 0) {
            pos += snprintf(&buf[pos], len - pos, "{\"%s\":", objs[i].name);
        }
        else {
            pos += snprintf(&buf[pos], len - pos, ",\"%s\":", objs[i].name);
        }

        float value = 0.0;
        uint32_t value_abs;

        // print value
        switch (objs[i].raw_data[0]) {
            case CAN_TS_T_TRUE:
            case CAN_TS_T_FALSE:
                pos += snprintf(&buf[pos], len - pos, "%d",
                    (objs[i].raw_data[0] == CAN_TS_T_TRUE) ? 1 : 0);
                break;
            case CAN_TS_T_POS_INT32:
                value_abs =
                    (objs[i].raw_data[1] << 24) +
                    (objs[i].raw_data[2] << 16) +
                    (objs[i].raw_data[3] << 8) +
                    (objs[i].raw_data[4]);
                pos += snprintf(&buf[pos], len - pos, "%u", value_abs);
                break;
            case CAN_TS_T_NEG_INT32:
                value_abs =
                    (objs[i].raw_data[1] << 24) +
                    (objs[i].raw_data[2] << 16) +
                    (objs[i].raw_data[3] << 8) +
                    (objs[i].raw_data[4]);
                pos += snprintf(&buf[pos], len - pos, "%d", -(int32_t)(value_abs + 1));
                break;
            case CAN_TS_T_FLOAT32:
                f2b.b[3] = objs[i].raw_data[1];
                f2b.b[2] = objs[i].raw_data[2];
                f2b.b[1] = objs[i].raw_data[3];
                f2b.b[0] = objs[i].raw_data[4];
                pos += snprintf(&buf[pos], len - pos, "%.3f", f2b.f);
                break;
            case CAN_TS_T_DECFRAC:
                value_abs =
                    (objs[i].raw_data[4] << 24) +
                    (objs[i].raw_data[5] << 16) +
                    (objs[i].raw_data[6] << 8) +
                    (objs[i].raw_data[7]);

                // dirty hack: We know that currently decfrac is only used for mV or mA
                if (objs[i].raw_data[3] == 0x1a &&
                    objs[i].raw_data[2] == 0x22)
                {
                    // positive int32 with exp -3
                    value = (float)value_abs / 1000.0;
                }
                else if (objs[i].raw_data[3] == 0x3a &&
                    objs[i].raw_data[2] == 0x22)
                {
                    // negative int32 with exp -3
                    value = -((float)value_abs + 1.0) / 1000.0;
                }
                else {
                    pos += snprintf(&buf[pos], len - pos, "err");
                }
                pos += snprintf(&buf[pos], len - pos, "%.3f", value);
                break;
        }
    }

    if (pos < len - 1) {
        buf[pos++] = '}';
    }
    return pos;
}

char *get_mppt_json_data()
{
    generate_json_string(json_buf, sizeof(json_buf),
        data_obj_mppt, sizeof(data_obj_mppt)/sizeof(DataObject));
    return json_buf;
}

char *get_bms_json_data()
{
    generate_json_string(json_buf, sizeof(json_buf),
        data_obj_bms, sizeof(data_obj_bms)/sizeof(DataObject));
    return json_buf;
}

void can_setup()
{

#ifdef GPIO_CAN_STB
    // switch CAN transceiver on (STB = low)
    gpio_pad_select_gpio(CONFIG_GPIO_CAN_STB);
    gpio_set_direction(CONFIG_GPIO_CAN_STB, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_GPIO_CAN_STB, 0);
#endif

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install CAN driver");
        return;
    }

    if (twai_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start CAN driver");
        return;
    }

    receive_queue = xQueueCreate(RECV_QUEUE_SIZE, sizeof(RecvMsg));
    if (!receive_queue) {
        ESP_LOGE(TAG, "Failed to create receiving queue");
        return;
    }

    /* Initialize link with the CAN ID we send with */
    isotp_init_link(&isotp_link, can_addr_server << 8 | can_addr_client | 0x1ada << 16,
        isotp_send_buf, sizeof(isotp_send_buf), isotp_recv_buf, sizeof(isotp_recv_buf));
}

void can_receive_task(void *arg)
{
    twai_message_t message;
    unsigned int device_addr;
    unsigned int data_node_id;

    uint8_t payload[1000];
    int ret;
    while (1) {
        ret = twai_receive(&message, pdMS_TO_TICKS(10000));
        if (ret == ESP_OK) {
            device_addr = message.identifier & 0x000000FF;
            ESP_LOGD(TAG, "Received CAN msg from %.2x", device_addr);

            TSDevice *ts_dev = ts_get_can_device(device_addr);
            if (ts_dev == NULL) {
                ts_devices_add_can(device_addr);
                ESP_LOGI(TAG, "Adding CAN device %x", device_addr);
            }

            /* checking for CAN ID used to receive ISO-TP frames */
            if (message.identifier == (can_addr_client << 8 | can_addr_server | 0x1ada << 16)) {
                ESP_LOGD(TAG, "ISO TP msg part received");
                isotp_on_can_message(&isotp_link, message.data, message.data_length_code);

                /* process multiple frame transmissions and timeouts */
                isotp_poll(&isotp_link);

                /* extract received data */
                uint16_t out_size = 0;
                int ret = isotp_receive(&isotp_link, payload, sizeof(payload) - 1, &out_size);
                if (ret == ISOTP_RET_OK) {
                    RecvMsg msg;
                    uint8_t *data = malloc(out_size + 1);
                    memcpy(data, payload, out_size);
                    msg.data = data;
                    msg.data[out_size] = '\0';
                    msg.len = out_size;
                    ESP_LOGD(TAG, "Received response: %s", (char *)msg.data);
                    if (!xQueueSend(receive_queue, &msg, pdMS_TO_TICKS(10))) {
                        ESP_LOGE(TAG, "Response could not be queued");
                        free(data);
                    }
                    else if (ret == ISOTP_RET_NO_DATA) {
                        ESP_LOGE(TAG, "isotp_receive(): No Data Received");
                    }
                }
            }
            else {
                // ThingSet publication message format: https://libre.solar/thingset/
                data_node_id = (message.identifier & 0x00FFFF00) >> 8;

                if (device_addr == 0) {
                    for (int i = 0; i < sizeof(data_obj_bms) / sizeof(DataObject); i++) {
                        if (data_obj_bms[i].id == data_node_id) {
                            memcpy(data_obj_bms[i].raw_data, message.data,
                                message.data_length_code);
                            data_obj_bms[i].len = message.data_length_code;
                        }
                    }
                    update_bms_received = true;
                }
                else if (device_addr == 10) {
                    for (int i = 0; i < sizeof(data_obj_mppt) / sizeof(DataObject); i++) {
                        if (data_obj_mppt[i].id == data_node_id) {
                            memcpy(data_obj_mppt[i].raw_data, message.data,
                                message.data_length_code);
                            data_obj_mppt[i].len = message.data_length_code;
                        }
                    }
                    update_mppt_received = true;
                }
                ESP_LOGD(TAG, "Received pub-msg on CAN:");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, message.data, message.data_length_code, ESP_LOG_DEBUG);
            }
        }
        else {

            if (ret == ESP_ERR_TIMEOUT) {
                ESP_LOGD(TAG, "Receive timed out");
                // ToDo: Remove devices from device list
            }
            else if (ret == ESP_ERR_INVALID_STATE) {
                ESP_LOGE(TAG, "Driver in invalid state");
            }
        }
    }
}

char *ts_can_send(uint8_t *req, uint32_t query_size, uint8_t can_address, uint32_t *block_len)
{
    RecvMsg msg;
    // empty queue before request, don't block if empty and
    // dismiss data if present
    if (xQueueReceive(receive_queue, &msg, 50)) {
        if (msg.data != NULL) {
            free(msg.data);
        }
    }

    int ret = isotp_send(&isotp_link, req, query_size);
    ESP_LOGI(TAG, "ISOTP Send %s", ret == ESP_OK ? "OK" : "FAILED");
    if (xQueueReceive(receive_queue, &msg, pdMS_TO_TICKS(1500))) {
        *block_len = msg.len;
        return (char *) msg.data;
    }

    return NULL;
}

int ts_can_scan_device_info(TSDevice *device)
{
    TSResponse res;
    char query[] = "?info";

    res.block = ts_can_send((uint8_t *)query, strlen(query), device->can_address, &(res.block_len));
    if (res.block == NULL) {
        res.block = "";
    }

    if (ts_serial_resp_status(&res) == TS_STATUS_CONTENT) {
        // CAN bus is used in TEXT Mode for now so we use the serial methods here
        res.data = ts_serial_resp_data(&res);
        cJSON *json_data = cJSON_Parse(res.data);
        free(res.block);

        if (json_data == NULL) {
            ESP_LOGE(TAG, "Error parsing JSON");
            return ESP_FAIL;
        }

        int ret = ts_parse_device_info(json_data, device);
        free(json_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error parsing device information");
            return ESP_FAIL;
        };

        device->build_query = ts_build_query_serial;
        device->send = ts_can_send;
        device->ts_resp_data = ts_serial_resp_data;
        device->ts_resp_status = ts_serial_resp_status;
        return ESP_OK;
    }
    else {
        ESP_LOGE(TAG, "No valid response");
        return ESP_FAIL;
    }
}

#endif // UNIT_TEST
