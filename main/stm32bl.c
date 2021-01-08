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

const char *TAG = "stm32bl";

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
    uart_write_bytes(uart, (char *)buf, 2);
}

static void send_address(uint32_t addr)
{
    uint8_t buf[] = { addr >> 24, addr >> 16, addr >> 8, addr };
    send_buf(buf, sizeof(buf));
}

static int wait_resp()
{
    uint8_t byte;
    int ret = uart_read_bytes(uart, &byte, 1, pdMS_TO_TICKS(1000));
    if (ret > 0) {
        return byte;
    }
    return ESP_FAIL;
}

int stm32bl_erase_all()
{
    uint8_t buf[] = { 0xFF, 0xFF };   // code for mass erase

    send_cmd(STM32BL_EEM);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Erasing request failed");
        return ESP_FAIL;
    }

    send_buf(buf, sizeof(buf));
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

int stm32bl_unprotect_write()
{
    send_cmd(STM32BL_WUP);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }
    return wait_resp();
}

int stm32bl_unprotect_read()
{
    send_cmd(STM32BL_RUP);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }
    return wait_resp();
}

int stm32bl_protect_read()
{
    send_cmd(STM32BL_RP);
    if (wait_resp() != STM32BL_ACK) {
        return ESP_FAIL;
    }
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

int stm32bl_write(uint8_t *buf, uint32_t num_bytes, uint32_t start_addr)
{
    send_cmd(STM32BL_WM);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Write request failed");
        return ESP_FAIL;
    }

    send_address(start_addr);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Start address rejected: 0x%.8x", start_addr);
        return ESP_FAIL;
    }

    // The bootloader expects to receive (num_bytes - 1) as the first byte, followed by up to 256
    // bytes of data
    uint8_t *total = (uint8_t *)malloc(num_bytes + 1);
    total[0] = num_bytes - 1;
    xthal_memcpy(total + 1, buf, num_bytes);

    send_buf(total, num_bytes + 1);
    free(total);
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

uint16_t stm32bl_get_page_count(int chip)
{
    switch(chip) {
        case STM32L0XX:
            return STM32L0XX_FLASH_PAGE_COUNT;
        case STM32G4XX:
            return STM32G4XX_FLASH_PAGE_COUNT;
        default:
            return 0;
    }
}

uint16_t stm32bl_get_page_size(int chip)
{
    switch(chip) {
        case STM32L0XX:
            return STM32L0XX_FLASH_PAGE_SIZE;
        case STM32G4XX:
            return STM32G4XX_FLASH_PAGE_SIZE;
        default:
            return 0;
    }
}
