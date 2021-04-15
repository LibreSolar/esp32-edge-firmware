/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

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

static const uint8_t stm_reset_code[] = {
    0x01, 0x49,		        // ldr     r1, [pc, #4] ; (<AIRCR_OFFSET>)
    0x02, 0x4A,		        // ldr     r2, [pc, #8] ; (<AIRCR_RESET_VALUE>)
    0x0A, 0x60,		        // str     r2, [r1, #0]
    0xfe, 0xe7,		        // endless: b endless
    0x0c, 0xed, 0x00, 0xe0,	// .word 0xe000ed0c <AIRCR_OFFSET> = NVIC AIRCR register address
    0x04, 0x00, 0xfa, 0x05	// .word 0x05fa0004 <AIRCR_RESET_VALUE> = VECTKEY | SYSRESETREQ
};

static const stm32_device_t devices[] = {
    // STM32G431XX
    {0x468, 0x20004000, 0x08000000, 0x1FFF7800},
    // STM32L07XX
    {0x447, 0x20002000, 0x08000000, 0x1FF80000}
};

static const stm32_device_t *get_device_by_id(uint16_t id)
{
    for (int i = 0; i < sizeof(devices); i++) {
        if (devices[i].id == id) {
            return &(devices[i]);
        }
    }
    return NULL;
}

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

static inline void send_cmd(uint8_t cmd)
{
    uint8_t buf[] = { cmd, ~cmd };
    uart_write_bytes(uart, (char *)buf, 2);
}

static void send_address(uint32_t addr)
{
    if (addr % 4 != 0) {
        ESP_LOGE(TAG, "Error: address must be 4 byte aligned");
        return;
    }
    uint8_t buf[] = { addr >> 24, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
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
    for (int i = 0; i < max_pages; i++) {
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

    ESP_LOGD(TAG, "Erasing successfully finished");
    return ESP_OK;
}

int stm32bl_go(uint32_t addr)
{
    send_cmd(STM32BL_GO);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG,"Go command rejected");
        return ESP_FAIL;
    }

    send_address(addr);
    return wait_resp();
}

static int stm32bl_run_raw_code(uint32_t target_address, const uint8_t *code, uint32_t code_size)
{
    uint32_t stack_addr = STM32_RAM_START_ADDR;
    uint32_t code_address = target_address + 8 + 1;
    uint32_t length = code_size + 8;
    uint8_t *mem;

    if (target_address % 4 != 0) {
        ESP_LOGE(TAG, "Code address must be 4 byte aligned");
        return ESP_FAIL;
    }

    mem = malloc(length);
    if (!mem) {
        return ESP_FAIL;
    }

    memcpy(mem, &stack_addr, sizeof(uint32_t));
    memcpy(mem + 4, &code_address, sizeof(uint32_t));
    memcpy(mem + 8, code, code_size);

    if (stm32bl_write(mem, length, target_address) != STM32BL_ACK) {
        ESP_LOGE(TAG, "Could not write raw code!");
    }

    free(mem);
    return stm32bl_go(target_address);
}

int stm32bl_reset_device(uint16_t id)
{
    const stm32_device_t *device = get_device_by_id(id);
    if (device == NULL) {
        ESP_LOGE(TAG, "Cannot reset unkown device");
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "Found device: 0x%x", device->id);

    int ret;
    switch (device->id) {
        case STM32L07XX_ID:
            ret = stm32bl_run_raw_code(device->sram_start, stm_reset_code, sizeof(stm_reset_code));
            break;
        case STM32G431XX_ID:
            ret = stm32bl_reset_optr(device->opt_start);
            break;
        default:
            ESP_LOGE(TAG, "Can't reset unknown device!");
            ret = ESP_FAIL;
    }

    // wait for reboot
    vTaskDelay(pdMS_TO_TICKS(500));
    if (ret == ESP_FAIL) {
        ESP_LOGE(TAG, "Unable reset device!");
        return ESP_FAIL;
    };

    return ESP_OK;
}

// Use with caution, showed incoherent behavior with different MCUs
int stm32bl_unprotect_write()
{
    send_cmd(STM32BL_WUP);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG,"Unprotect write rejected");
        return ESP_FAIL;
    }
    wait_resp();

    // Unprotect write generates a reset, so we have to reinit the UART connection
    vTaskDelay(pdMS_TO_TICKS(500));

    if (stm32bl_init() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Reinit failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Use with caution, showed incoherent behavior with different MCUs
int stm32bl_unprotect_read()
{
    send_cmd(STM32BL_RUP);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Unprotect read rejected");
        return ESP_FAIL;
    }
    wait_resp();

    // Unprotect read generates a reset, so we have to reinit the UART connection
    vTaskDelay(pdMS_TO_TICKS(500));

    if (stm32bl_init() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Reinit failed");
        return ESP_FAIL;
    }
    return ESP_OK;
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
        ESP_LOGE(TAG, "Read command rejected");
        return ESP_FAIL;
    }

    send_address(addr);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Read address rejected");
        return ESP_FAIL;
    }

    // One byte with 0 < N < 256 and its complement is the same as
    // sending a command
    send_cmd(num_bytes);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Number of bytes to read rejected");
        return ESP_FAIL;
    }

    int ret = uart_read_bytes(uart, buf, num_bytes, pdMS_TO_TICKS(UART_TIMEOUT_MS));
    if (ret > 0) {
        return STM32BL_ACK;
    }

    return ESP_FAIL;
}

int stm32bl_write(uint8_t *buf, uint32_t num_bytes, uint32_t start_addr)
{
    if (num_bytes % 4 != 0) {
        ESP_LOGE(TAG, "Data is not aligned");
        return ESP_FAIL;
    }

    send_cmd(STM32BL_WM);
    if (wait_resp() == STM32BL_NACK) {
        ESP_LOGE(TAG, "Write request failed");
        return ESP_FAIL;
    }

    send_address(start_addr);
    if (wait_resp() == STM32BL_NACK) {
        ESP_LOGE(TAG, "Start address rejected: 0x%.8x", start_addr);
        return ESP_FAIL;
    }

    uint8_t *total = (uint8_t *)malloc(num_bytes + 1);
    total[0] = num_bytes - 1;
    xthal_memcpy(total + 1, buf, num_bytes);

    ESP_LOGD(TAG, "Sending out %d bytes", num_bytes);
    send_buf(total, num_bytes + 1);
    free(total);
    return wait_resp();
}

int stm32bl_get_version()
{
    uint8_t buf[14];

    send_cmd(STM32BL_GET);
    if (wait_resp() != STM32BL_ACK) {
        ESP_LOGE(TAG, "Get command rejected");
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

int stm32bl_reset_optr(uint32_t ob_addr)
{
    uint32_t optr[12] = {};
    ESP_LOGD(TAG, "Going to read from 0x%x", ob_addr);
    int ret = stm32bl_read((uint8_t *) &optr, sizeof(optr), ob_addr);
    if (ret != STM32BL_ACK) {
        ESP_LOGE(TAG, "Unable to read option bytes");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "nSWBOOT: %d", (optr[0] & STM32_nSWBOOT0) >> 26);
    ESP_LOGD(TAG, "nBOOT0: %d", (optr[0] & STM32_nBOOT0) >> 27);
    ESP_LOGD(TAG, "nBOOT1: %d", (optr[0] & STM32_nBOOT1) >> 23);

    // set software boot = 0, which disables the selection via pin
    // should already be set anyway, just making sure here
    optr[0] &= ~STM32_nSWBOOT0;
    // set BOOT0 = 1 to boot to main flash memory
    optr[0] |= STM32_nBOOT0;
    // set BOOT1 = 1. If the above values are corrupted, this will make sure
    // that the chip does not boot from SRAM but System Memory and could possibly
    // be recovered
    optr[0] |= STM32_nBOOT1;
    // the "right" half of the double word is the negated left half
    optr[1] = ~optr[0];

    if (stm32bl_write((uint8_t *) &optr, sizeof(optr), ob_addr) != STM32BL_ACK) {
        // this is bad, we can't reset the option bytes and are stuck in the bootloader
        ESP_LOGE(TAG, "Could not write option bytes");
        return ESP_FAIL;
    }

    // A system reset should be triggered now
    return ESP_OK;
}

#endif //UNIT_TEST
