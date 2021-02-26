/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <ts_client.h>
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define OTA_UART_LOCK_TIMEOUT 500

/**
 * Initiate the UART interface, event groups and semaphores.
 *
 * Must be called before starting the task.
 */
void ts_serial_setup(void);

/**
 * Thread listening to UART interface, needs to be spawned from main.
 */
void ts_serial_rx_task(void *arg);

/**
 * Returns latest pub message received on the interface and waits until timeout if receiving is
 * currently in progress
 *
 * \param timeout_ms Timeout in milliseconds
 *
 * \returns Pointer to received message (terminated string) or NULL if timed out
 */
char *ts_serial_pubmsg(int timeout_ms);

/**
 * Release buffer after processing to receive new messages
 *
 * This must be called after processing of a successfully received pubmsg is finished.
 */
void ts_serial_pubmsg_clear(void);

/**
 * Send request and lock response buffer
 *
 * \param req Request buffer (null-terminated)
 * \param timeout_ms Maximum timeout for getting buffer mutex
 *
 * \returns ESP_OK if request was sent, ESP_FAIL otherwise
 */
int ts_serial_request(char *req, int timeout_ms);

/**
 * Receive response from buffer
 *
 * This function should be called after ts_serial_request
 *
 * \param timeout_ms Timeout in milliseconds
 *
 * \returns Pointer to received message (terminated string) or NULL if timed out
 */
char *ts_serial_response(int timeout_ms);

/**
 * Release response buffer for new requests
 *
 * This must be called after processing of a successfully received response is finished.
 */
void ts_serial_response_clear(void);

/**
 * Scan for device on the serial connection
 *
 * \param device Pointer to allocated struct of type ts_device
 */
int ts_serial_scan_device_info(TSDevice *device);

/**
 * Start over-the-air firmware update (OTA)
 *
 * Uses the STM32 bootloader via UART. The binary must be stored in the designated spiffs partition.
 *
 * \param flash_size The flash size for the target MCU in bytes
 *
 * \param page_size The size of each page in bytes
 */
esp_err_t ts_serial_ota(int flash_size, int page_size);

#endif /* SERIAL_H_ */
