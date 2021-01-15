/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include "stm32bl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "string.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/uart.h"

/* used UART interface */
static int uart = UART_NUM_2;

const char *TAG = "stm32bl";

#define UART_TIMEOUT_MS 100

/* Reset code for ARMv7-M (Cortex-M3) and ARMv6-M (Cortex-M0)
 * see ARMv7-M or ARMv6-M Architecture Reference Manual (table B3-8)
 * or "The definitive guide to the ARM Cortex-M3", section 14.4.
 */
static const uint8_t stm_reset_code[] = {
	0x01, 0x49,		// ldr     r1, [pc, #4] ; (<AIRCR_OFFSET>)
	0x02, 0x4A,		// ldr     r2, [pc, #8] ; (<AIRCR_RESET_VALUE>)
	0x0A, 0x60,		// str     r2, [r1, #0]
	0xfe, 0xe7,		// endless: b endless
	0x0c, 0xed, 0x00, 0xe0,	// .word 0xe000ed0c <AIRCR_OFFSET> = NVIC AIRCR register address
	0x04, 0x00, 0xfa, 0x05	// .word 0x05fa0004 <AIRCR_RESET_VALUE> = VECTKEY | SYSRESETREQ
};


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
    /* must be 32bit aligned */
	if (addr & 0x3) {
		ESP_LOGE(TAG, "Error: address must be 4 byte aligned\n");
        return;
	}
    uint8_t buf[] = { addr >> 24, (addr >> 16) & 0xFF, (addr >> 8) & 0XFF, addr & 0xFF };
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
    uint8_t buf[4] = {};
    buf[0] = 0;
    buf[1] = 0;
    for(int i = 0; i < max_pages; i++) {
        send_cmd(STM32BL_EEM);
        if (wait_resp() != STM32BL_ACK) {
            ESP_LOGE(TAG, "Erasing request failed");
            return ESP_FAIL;
        }
        buf[2] = (i >> 8) & 0xFF;
        buf[3] = i & 0xFF;
        send_buf(buf, 4);
        if (wait_resp() != STM32BL_ACK) {
            ESP_LOGE(TAG, "Could not erase page 0x%.2x", i);
            return ESP_FAIL;
        }
    }
    return STM32BL_ACK;
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

static int stm32bl_run_raw_code(uint32_t target_address, const uint8_t *code, uint32_t code_size)
{
	uint32_t stack_le = 0x20002000;
	uint32_t code_address_le = target_address + 8 + 1; // thumb mode address (!)
	uint32_t length = code_size + 8;
	uint8_t *mem, *pos;
	uint32_t address;

	/* Must be 32-bit aligned */
	if (target_address & 0x3) {
		ESP_LOGE(TAG, "Error: code address must be 4 byte aligned\n");
		return ESP_FAIL;
	}

	mem = malloc(length);
	if (!mem)
		return ESP_FAIL;

	memcpy(mem, &stack_le, sizeof(uint32_t));
	memcpy(mem + 4, &code_address_le, sizeof(uint32_t));
	memcpy(mem + 8, code, code_size);

	pos = mem;
	address = target_address;
    stm32bl_write(pos, length, address);

	free(mem);
	return stm32bl_go(target_address);
}

int stm32bl_reset_device()
{
    uint32_t address = 0x20002000;
    return stm32bl_run_raw_code(address, stm_reset_code, sizeof(stm_reset_code));
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
    if(num_bytes % 4 != 0) {
        ESP_LOGE(TAG, "Data is not aligned");
        return ESP_FAIL;
    }
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
