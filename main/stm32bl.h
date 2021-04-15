/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STM32BL_H_
#define STM32BL_H_

#include <stdint.h>

#define STM32BL_INIT    0x7F    /* Bootloader init command */
#define STM32BL_ACK     0x79    /* ACK response */
#define STM32BL_NACK    0x1F    /* NACK response */

#define STM32BL_GET     0x00    /* Get command */
#define STM32BL_GVR     0x01    /* Get Version & Read Protection Status command */
#define STM32BL_GID     0x02    /* Get ID command */
#define STM32BL_RM      0x11    /* Read Memory command */
#define STM32BL_GO      0x21    /* Go command */
#define STM32BL_WM      0x31    /* Write Memory command */
#define STM32BL_EM      0x43    /* Erase Memory command */
#define STM32BL_EEM     0x44    /* Extended Erase Memory command */
#define STM32BL_WP      0x63    /* Write Protect command */
#define STM32BL_WUP     0x73    /* Write Unprotect command */
#define STM32BL_RP      0x82    /* Readout Protect command */
#define STM32BL_RUP     0x92    /* Readout Unprotect command */

/* STM32 code start address (must be changed if custom bootloader is used */
#define STM32_FLASH_START_ADDR      0x08000000
#define STM32_RAM_START_ADDR        0x20002000

#define STM32L07XX_ID           0x447
#define STM32G431XX_ID          0x468

#define STM32_nBOOT0            (1U << 27)
#define STM32_nSWBOOT0          (1U << 26)
#define STM32_nBOOT1            (1U << 23)

typedef struct {
    uint16_t id;
    uint32_t sram_start;
    uint32_t flash_start;
    uint32_t opt_start;
} stm32_device_t;
/**
 * Resets the device after flashing
 *
 * @return ESP_FAIL in case of error
 */
int stm32bl_reset_device(uint16_t id);

/**
 * Get bootloader version
 *
 * @return Version number or ESP_FAIL in case of error
 */
int stm32bl_get_version();

/**
 * Get STM32 MCU product ID
 *
 * @return Product ID or ESP_FAIL in case of error
 */
int stm32bl_get_id();

/**
 * Send unprotect write memory command
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if protection was successfully unset
 *      - STM32BL_NACK if protection was not successfully unset
 */
int stm32bl_unprotect_write();

/**
 * Send unprotect read memory command
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if protection was successfully unset
 *      - STM32BL_NACK if protection was not successfully unset
 */
int stm32bl_unprotect_read();

/**
 * Send protect read memory command
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if protection was successfully set
 *      - STM32BL_NACK if protection was not successfully set
 */
int stm32bl_protect_read();

/**
 * Global erase of flash
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if erase was successful
 *      - STM32BL_NACK if erase was not successful
 */
int stm32bl_erase_all(uint16_t max_pages);

/**
 * Read from flash memory
 *
 * @param buf Buffer to store read data
 * @param num_bytes Number of bytes to read (must fit into buffer)
 * @param start_addr Address to read from
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if reading was successful
 *      - STM32BL_NACK if reading was not successful
 */
int stm32bl_read(uint8_t *buf, uint8_t num_bytes, uint32_t start_addr);

/**
 * Write to flash memory
 *
 * @param buf Buffer containing data to write
 * @param num_bytes Number of bytes to write
 * @param start_addr Start address to write to
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if reading was successful
 *      - STM32BL_NACK if writing was not successful
 */
int stm32bl_write(uint8_t *buf, uint32_t num_bytes, uint32_t start_addr);

/**
 * Go to address (to start program)
 *
 * @param addr Address in MCU RAM
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if reading was successful
 *      - STM32BL_NACK if reading was not successful
 */
int stm32bl_go(uint32_t addr);

/**
 * Initialize communication with the STM32 bootloader
 *
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if reading was successful
 *      - STM32BL_NACK if reading was not successful
 */
int stm32bl_init();

/**
 * Resets the option bytes so that the bootloader is not
 * started over and over again.
 *
 * @param The starting address of the option byte area
 * @return
 *      - ESP_FAIL in case of communication error
 *      - STM32BL_ACK if reading and writing was successful
 *      - STM32BL_NACK if reading or writing was not successful
 */
int stm32bl_reset_optr(uint32_t ob_addr);

#endif /* STM32BL_H_ */
