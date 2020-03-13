/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include <stdbool.h>

extern bool update_serial_received;

/**
 * Initiate the UART interface.
 */
void uart_setup(void);

/**
 * Thread listening to UART interface, needs to be spawned from main.
 */
void uart_rx_task(void *arg);

/**
 * Get data received from UART interface in JSON format
 *
 * Caution: This function is currently not thread-safe and could be changed while reading it.
 *
 * \returns pointer to the buffer
 */
char *get_serial_json_data();
