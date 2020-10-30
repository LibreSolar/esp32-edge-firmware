/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include "stm32bl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/uart.h"

/* used UART interface */
static int uart = UART_NUM_2;

#define UART_TIMEOUT_MS 100

static uint8_t calc_checksum(uint8_t *data, uint8_t len)
{
    uint8_t ret = data[0];
    for (int i = 1; i < len; i++) {
        ret ^= data[i];
    }
    return ret;
}

static void send_buf(uint8_t *buf, size_t len)
{
    uint8_t checksum = calc_checksum(buf, len);
    uart_write_bytes(uart, (char *)buf, len);
    uart_write_bytes(uart, (char *)&checksum, 1);
}

static inline void send_byte(uint8_t byte)
{
    send_buf(&byte, 1);
}

static inline void send_cmd(uint8_t cmd)
{
    uint8_t buf[] = { cmd, ~cmd };
    uart_write_bytes(uart, (char *)buf, sizeof(buf));
}

static void send_address(uint32_t addr)
{
    uint8_t buf[] = { addr >> 24, addr >> 16, addr >> 8, addr };
    send_buf(buf, sizeof(buf));
}

static int wait_resp()
{
    uint8_t byte;
    int ret = uart_read_bytes(uart, &byte, 1, pdMS_TO_TICKS(UART_TIMEOUT_MS));
    if (ret > 0) {
        return byte;
    }
    return ESP_FAIL;
}

int stm32bl_erase_all()
{
    send_cmd(STM32BL_EM);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    send_byte(0xFF);        /* global erase */
    return wait_resp();
}

int stm32bl_go(uint32_t addr)
{
    send_cmd(STM32BL_GO);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    send_address(addr);
    return wait_resp();
}

int stm32bl_init()
{
    uint8_t init_byte = STM32BL_INIT;
    uart_write_bytes(uart, (char *)&init_byte, 1);
    return wait_resp();
}

int stm32bl_read(uint8_t *buf, uint8_t num_bytes, uint32_t addr)
{
    send_cmd(STM32BL_RM);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    send_address(addr);
    if (wait_resp() == STM32BL_ACK) {
        return ESP_FAIL;
    }

    send_byte(num_bytes);
    int ret = uart_read_bytes(uart, buf, num_bytes, pdMS_TO_TICKS(UART_TIMEOUT_MS));
    if (ret > 0) {
        return STM32BL_ACK;
    }

    return ESP_FAIL;
}

int stm32bl_write(uint8_t *buf, uint8_t num_bytes, uint32_t start_addr)
{
    send_cmd(STM32BL_WM);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    send_address(start_addr);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    send_byte(num_bytes);
    send_buf(buf, num_bytes);
    return wait_resp();
}

int stm32bl_get_version()
{
    uint8_t buf[14];

    send_cmd(STM32BL_GET);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    int ret = uart_read_bytes(uart, buf, 14, pdMS_TO_TICKS(UART_TIMEOUT_MS));
    if (ret > 0) {
        return buf[1];
    }

    return ESP_FAIL;
}

int stm32bl_get_id()
{
    uint8_t buf[4];

    send_cmd(STM32BL_GID);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }

    int ret = uart_read_bytes(uart, buf, 4, pdMS_TO_TICKS(UART_TIMEOUT_MS));
    if (ret > 0) {
        return (buf[1] << 8) + buf[2];
    }

    return ESP_FAIL;
}
