/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include "ts_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "ts_client.h"
#include "cJSON.h"
#include "driver/uart.h"

#include "stm32bl.h"

static const char *TAG = "ts_ser";


#define PUBMSG_BUF_SIZE     (1024)
#define RESP_BUF_SIZE       (1024)
#define UART_RX_BUF_SIZE    (256)

EventGroupHandle_t events = NULL;
#define FLAG_AWAITING_RESPONSE  (1U << 0)
#define FLAG_RESPONSE_RECEIVED  (1U << 1)
#define FLAG_PUBMSG_RECEIVED    (1U << 2)

/* stores incoming publication messages */
static uint8_t pubmsg_buf[PUBMSG_BUF_SIZE];
SemaphoreHandle_t pubmsg_buf_lock = NULL;

/* stores incoming response messages */
static uint8_t resp_buf[RESP_BUF_SIZE];
SemaphoreHandle_t resp_buf_lock = NULL;

SemaphoreHandle_t uart_lock = NULL;

/* used UART interface */
static const int uart_num = UART_NUM_2;

void ts_serial_setup(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(
        uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(
        uart_set_pin(uart_num, CONFIG_GPIO_UART_TX, CONFIG_GPIO_UART_RX,
            UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(
        uart_driver_install(uart_num, UART_RX_BUF_SIZE, 0, 0, NULL, 0));

    /* mutex: expected to be taken and given from same task */
    resp_buf_lock = xSemaphoreCreateMutex();

    /* binary semaphore: uart task for new messages, other tasks for processing */
    pubmsg_buf_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(pubmsg_buf_lock);

    uart_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(uart_lock);

    events = xEventGroupCreate();
}

static inline void terminate_buffer(uint8_t *buf, int pos)
{
    if (pos > 0 && buf[pos-1] == '\r') {
        buf[pos-1] = '\0';
    }
    else {
        buf[pos] = '\0';
    }
}

void ts_serial_rx_task(void *arg)
{
    // following two flags indicate in which buffer new characters should be stored
    static bool receiving_pubmsg = false;
    static bool receiving_resp = false;

    int pos = 0;        // stores next free position in currently used buffer

    while (true) {
        uint8_t byte;
        // wait for incoming characters
        while (uart_read_bytes(uart_num, &byte, 1, pdMS_TO_TICKS(50)) == 0) {
            // this allows other threads to block UART read access in this thread (e.g. for
            // firmware upgrade)
            xSemaphoreTake(uart_lock, portMAX_DELAY);
            xSemaphoreGive(uart_lock);
        }

        if (pos == 0 && byte == '#') {
            // only store pub msg if nobody is processing previous message
            if (xSemaphoreTake(pubmsg_buf_lock, 0) == pdTRUE) {
                xEventGroupClearBits(events, FLAG_PUBMSG_RECEIVED);
                receiving_pubmsg = true;
            }
        }
        else if (pos == 0 && byte == ':') {
            // only store response if someone is actually waiting for it
            if (xEventGroupGetBits(events) & FLAG_AWAITING_RESPONSE) {
                xEventGroupClearBits(events, FLAG_AWAITING_RESPONSE);
                receiving_resp = true;
            }
        }

        // \r\n and \n are markers for line end, i.e. command end
        // we accept this at any time, even if the buffer is 'full', since
        // there is always one last character left for the \0
        if (byte == '\n') {
            if (receiving_pubmsg) {
                terminate_buffer(pubmsg_buf, pos);
                xEventGroupSetBits(events, FLAG_PUBMSG_RECEIVED);
                xSemaphoreGive(pubmsg_buf_lock);
                receiving_pubmsg = false;
                //ESP_LOGI("serial", "Received pub message with %d bytes: %s\n", pos, pubmsg_buf);
            }
            else if (receiving_resp) {
                terminate_buffer(resp_buf, pos);
                xEventGroupSetBits(events, FLAG_RESPONSE_RECEIVED);
                receiving_resp = false;
            }
            pos = 0;
        }
        // Fill the buffer up to all but 1 character (the last character is reserved for '\0')
        // Characters beyond the size of the buffer are dropped.
        else if (receiving_pubmsg && pos < (sizeof(pubmsg_buf) - 1)) {
            pubmsg_buf[pos++] = byte;
        }
        else if (receiving_resp && pos < (sizeof(resp_buf) - 1)) {
            resp_buf[pos++] = byte;
        }
        else {
            // increase to make sure that pos != 0 for : and # that are not at beginning of line
            //printf("%c", (char)byte);
            pos++;
        }
    }
}

char *ts_serial_pubmsg(int timeout_ms)
{
    if (xSemaphoreTake(pubmsg_buf_lock, pdMS_TO_TICKS(timeout_ms)) == pdFALSE) {
        ESP_LOGE(TAG, "Could not take semaphore resp_buf_lock");
        return NULL;
    }

    if (xEventGroupGetBits(events) & FLAG_PUBMSG_RECEIVED) {
        return (char *)pubmsg_buf;
    }
    else {
        xSemaphoreGive(pubmsg_buf_lock);
        return NULL;
    }
}

void ts_serial_pubmsg_clear()
{
    xEventGroupClearBits(events, FLAG_PUBMSG_RECEIVED);
    xSemaphoreGive(pubmsg_buf_lock);
}

int ts_serial_request(char *req, int timeout_ms)
{
    if (xSemaphoreTake(resp_buf_lock, pdMS_TO_TICKS(timeout_ms)) == pdFALSE) {
        ESP_LOGE(TAG, "Could not take semaphore resp_buf_lock");
        return ESP_FAIL;
    }

    xEventGroupSetBits(events, FLAG_AWAITING_RESPONSE);
    uart_write_bytes(uart_num, req, strlen(req));

    return ESP_OK;
}

char *ts_serial_response(int timeout_ms)
{
    EventBits_t ret = xEventGroupWaitBits(events, FLAG_RESPONSE_RECEIVED, pdTRUE, pdTRUE,
        pdMS_TO_TICKS(timeout_ms));

    if (ret & FLAG_RESPONSE_RECEIVED) {
        return (char *)resp_buf;
    }
    else {
        return NULL;
    }
}

void ts_serial_response_clear()
{
    xEventGroupClearBits(events, FLAG_RESPONSE_RECEIVED);
    xSemaphoreGive(resp_buf_lock);
}

// CAN_Address is not needed here, but we need the same signature
// as CAN send
char *ts_serial_send(char *req, uint8_t CAN_Address)
{
    if (req == NULL) {
        ESP_LOGE(TAG, "Got invalid parameter");
        return NULL;
    }

    if (ts_serial_request(req, 200) != ESP_OK) {
        ESP_LOGE(TAG, "Request failed: %s", req);
        return NULL;
    }

    char *buf = ts_serial_response(200);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Response failed");
        ts_serial_response_clear();
        return NULL;
    }

    char *resp = (char *) heap_caps_malloc(strlen(buf)+1, MALLOC_CAP_8BIT);
    strcpy(resp, buf);
    ts_serial_response_clear();
    return resp;
}

int ts_serial_scan_device_info(TSDevice *device)
{
    char req[7]= "?info\n\0";
    // First request mostly fails, so we request it twice
    if (ts_serial_request(req, 500) == ESP_FAIL) {
        ESP_LOGE(TAG, "Could not scan for devices on serial adapter");
    }

    char *resp = ts_serial_response(500);
    int status = ts_resp_status(resp != NULL ? resp : "");
    if (status != TS_STATUS_CONTENT) {
        ESP_LOGE(TAG, "Could not retrieve device information: Code %d", status);
        ts_serial_response_clear();
    }

    ts_serial_response_clear();

    if (ts_serial_request(req, 500) == ESP_FAIL) {
        ESP_LOGE(TAG, "Could not scan for devices on serial adapter");
        return -1;

    }

    resp = ts_serial_response(500);
    status = ts_resp_status(resp != NULL ? resp : "");
    if (status != TS_STATUS_CONTENT) {
        ESP_LOGE(TAG, "Could not retrieve device information: Code %d", status);
        ts_serial_response_clear();
        return -1;
    }

    // shift pointer to data
    resp = ts_resp_data(resp);

    cJSON *json_data = cJSON_Parse(resp);
    ts_serial_response_clear();
    size_t ts_string_len = strlen(cJSON_GetStringValue(cJSON_GetObjectItem(json_data, "DeviceType")));
    device->ts_name = (char *) heap_caps_malloc(ts_string_len+1, MALLOC_CAP_8BIT);
    strcpy(device->ts_name, cJSON_GetStringValue(cJSON_GetObjectItem(json_data, "DeviceType")));
    ts_string_len = strlen(cJSON_GetStringValue(cJSON_GetObjectItem(json_data, "DeviceID")));
    device->ts_device_id = (char *) heap_caps_malloc(ts_string_len+1, MALLOC_CAP_8BIT);
    strcpy(device->ts_device_id , cJSON_GetStringValue(cJSON_GetObjectItem(json_data, "DeviceID")));
    // link send function
    device->send = ts_serial_send;
    device->CAN_Address = UINT8_MAX;
    ESP_LOGI(TAG, "Found device with ID: %s!", device->ts_device_id);
    cJSON_Delete(json_data);
    return 0;
}

esp_err_t ts_serial_ota(int flash_size, int page_size)
{
    int ret = ESP_FAIL;
    uint16_t pages = flash_size * 1024 / page_size;

    ts_serial_request("!dfu/bootloader-stm\n", 100);
    ts_serial_response_clear();

    // prevent further UART access in RX thread
    if (xSemaphoreTake(uart_lock, pdMS_TO_TICKS(OTA_UART_LOCK_TIMEOUT)) != pdTRUE ) {
        // this is bad and should not happen, as we already started the bootloader now
        ESP_LOGE(TAG, "Could not take semaphore uart_lock");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    uart_flush(uart_num);
    uart_set_parity(uart_num, UART_PARITY_EVEN);
    uint32_t bytes_read = 0;
    uint8_t buf[page_size];
    FILE * f = NULL;
    int id = 0;

    if (stm32bl_init() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Init failed");
        goto out;
    }
    id = stm32bl_get_id();
    ESP_LOGD(TAG, "STM32BL version: 0x%x", stm32bl_get_version());
    ESP_LOGD(TAG, "STM32BL pid: 0x%x", id);

    f = fopen("/stm_ota/firmware.bin", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open STM image");
        goto out;
    }

    ESP_LOGD(TAG, "Going to erase %u pages", pages);
    if (stm32bl_erase_all(pages) != ESP_OK) {
        ESP_LOGE(TAG, "Mass erase failed");
        goto out;
    }
    uint32_t address = STM32_FLASH_START_ADDR;

    // The maximum block the stm bootloader can handle is 256 bytes
    uint32_t block_size = 128;
    while (true) {
        bytes_read = fread(buf, 1, block_size, f);
        if (bytes_read == EOF || bytes_read == 0) {
            ESP_LOGD(TAG, "Reading and sending of firmware file finished. Wrote %d bytes", address - STM32_FLASH_START_ADDR);
            ret = ESP_OK;
            goto out;
        }
        if (stm32bl_write(buf, bytes_read, address) != STM32BL_ACK) {
            ESP_LOGE(TAG, "Writing failed");
            goto out;
        }
        address += bytes_read;
    };

out:
    if (f != NULL) {
        fclose(f);
    }

    // wait a bit to let the stm32 finish writing
    vTaskDelay(pdMS_TO_TICKS(1000));
    ret = stm32bl_reset_device(id);

    uart_set_parity(uart_num, UART_PARITY_DISABLE);
    xSemaphoreGive(uart_lock);
    return ret;
}

#endif //UINIT_TEST
