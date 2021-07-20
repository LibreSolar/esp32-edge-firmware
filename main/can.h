/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "ts_client.h"

typedef struct {
    int id;
    const char *name;
    uint8_t raw_data[8];
    int len;
} DataObject;

#define CAN_TS_T_TRUE 61
#define CAN_TS_T_FALSE 60
#define CAN_TS_T_POS_INT32  6
#define CAN_TS_T_NEG_INT32  7
#define CAN_TS_T_FLOAT32 30
#define CAN_TS_T_DECFRAC 36

extern bool update_bms_received;
extern bool update_mppt_received;

typedef struct {
    uint8_t * data;
    int len;
} RecvMsg;

/**
 * Sends a query to a given address. If a string is used, the termination bit must be substracted
 * from the query length before invoking this method.
 *
 * @param req A pointer to a buffer with either a string or binary request
 * @param query_size The size of the buffer, not including zero termination byte for strings
 * @param can_address The target address on the CAN bus
 * @param block_len A pointer to a variable where the length of the response block will be stored
 *
 * \returns A pointer to the response block, to be freed subsequently
 */
char *ts_can_send(uint8_t *req, uint32_t query_size, uint8_t can_address, uint32_t *block_len);


int ts_can_scan_device_info(TSDevice *device);

/**
 * Initiate the CAN interface.
 */
void can_setup();

/**
 * Thread listening to CAN interface, needs to be spawned from main.
 */
void can_receive_task(void *arg);

/**
 * Thread performing regular requests to other devices using ISO-TP
 */
void isotp_task(void *arg);

/**
 * Get data from MPPT connected via CAN bus and convert it to JSON
 *
 * Caution: This function is currently not thread-safe and could be changed while reading it.
 *
 * \returns pointer to the buffer
 */
char *get_mppt_json_data();

/**
 * Get data from BMS connected via CAN bus and convert it to JSON
 *
 * Caution: This function is currently not thread-safe and could be changed while reading it.
 *
 * \returns pointer to the buffer
 */
char *get_bms_json_data();
