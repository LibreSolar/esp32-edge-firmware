/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include "ts_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/uart.h"

static const char *TAG = "ts_ser";

#if CONFIG_THINGSET_SERIAL

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
        while (uart_read_bytes(uart_num, &byte, 1, portMAX_DELAY) == 0) {;}

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
                ESP_LOGI("serial", "Received response with %d bytes: %s\n", pos, resp_buf);
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

#endif
