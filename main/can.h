/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

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
