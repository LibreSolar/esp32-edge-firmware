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

int stm32bl_erase_all(uint16_t max_pages)
{
    send_cmd(STM32BL_EEM);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Erasing request failed");
        return ESP_FAIL;
    }
    // 2 bytes for total number of pages (as uint16) to be erased and
    // 2*max_pages bytes as index for each page
    size_t byte_len = sizeof(uint16_t)*max_pages + 2;
    uint8_t *buf = (uint8_t *) malloc(byte_len);
    buf[0] = (max_pages - 1) >> 8;  // MSB of N
    buf[1] = (max_pages - 1);       // LSB of N
    uint16_t page_num = 0;
    for (int i = 2; i < byte_len; i+=2) {
        buf[i] = page_num >> 8;     // MSB of page n
        buf[i+1] = page_num;        // LSB of page n
        page_num++;
    }
    //page_num should be max_pages now
    send_buf(buf, byte_len);
    free(buf);
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
        ESP_LOGE(TAG, "Writing request failed");
        return ESP_FAIL;
    }

    send_address(start_addr);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Adress rejected");
        return ESP_FAIL;
    }
    // The protocol specifies:
    // slave gets a byte, N, which contains the number of data bytes to be received
    // receives the user data ((N + 1) bytes) and the checksum of ALL sent bytes
    // To use send_buf(), we have to prepend the number N
    uint8_t * total = (uint8_t *) malloc(num_bytes + 1);
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

uint16_t stm32bl_get_page_num(int chip)
{
    switch(chip) {
        case STM32L0xx:
            return L0x_FLASH_PAGE_NUM;
        case STM32G4xx:
            return G4x_FLASH_PAGE_NUM;
        default:
            return 0;
    }
}

uint16_t stm32bl_get_page_size(int chip)
{
    switch(chip) {
        case STM32L0xx:
            return L0x_FLASH_PAGE_SIZE;
        case STM32G4xx:
            return G4x_FLASH_PAGE_SIZE;
        default:
            return 0;
    }
}
