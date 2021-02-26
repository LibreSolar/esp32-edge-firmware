/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/can.h"
#include "driver/gpio.h"

#include "../lib/isotp/isotp.h"

/*
 * required, this must send a single CAN message with the given arbitration
 * ID (i.e. the CAN message ID) and data. The size will never be more than 8
 * bytes.
 */
int isotp_user_send_can(const uint32_t arbitration_id,
                        const uint8_t* data, const uint8_t size)
{
    can_message_t msg;
    memcpy(msg.data, data, size);
    msg.data_length_code = size;
    msg.identifier = arbitration_id;
    msg.flags = CAN_MSG_FLAG_EXTD;
    return can_transmit(&msg, 0);
}

/*
 * required, return system tick, unit is millisecond
 */
uint32_t isotp_user_get_ms(void)
{
    return esp_timer_get_time() / 1000;
}

/*
 * optional, provide to receive debugging log messages
 */
void isotp_user_debug(const char* message, ...)
{
	va_list argp;
	va_start(argp, message);
	printf(message, argp);
	va_end(argp);
}
