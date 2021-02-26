/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Configure Wi-Fi, connect, wait for IP
 *
 * Simple function to connect to WiFi based on ESP-IDF examples
 *
 * @return ESP_OK on successful connection
 */
esp_err_t wifi_connect(void);

/**
 * Counterpart to example_connect, de-initializes Wi-Fi
 */
esp_err_t wifi_disconnect(void);

#ifdef __cplusplus
}
#endif
