/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTA_H_
#define OTA_H_

#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

esp_err_t esp_ota_handler(httpd_req_t *req, cJSON* res);
void esp_ota_reset_device();
void esp_ota_check_image();

#endif
