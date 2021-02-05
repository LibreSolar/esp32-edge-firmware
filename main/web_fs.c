/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"

static const char* TAG = "web_fs";

static esp_err_t check_response(esp_err_t ret)
{
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t www_conf = {
        .base_path = "/www",
        .partition_label = "website",
        .max_files = 5,
        .format_if_mount_failed = false
    };
    if(check_response(esp_vfs_spiffs_register(&www_conf)) != ESP_OK) {
        return ESP_FAIL;
    };

    esp_vfs_spiffs_conf_t ota_conf = {
        .base_path = "/stm_ota",
        .partition_label = "stm_ota",
        .max_files = 1,
        .format_if_mount_failed = true
    };
    if(check_response(esp_vfs_spiffs_register(&ota_conf)) != ESP_OK) {
        return ESP_FAIL;
    };

    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info("website", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information for website (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size for website: total: %d, used: %d", total, used);
    }

    ret = esp_spiffs_info("stm_ota", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information for ota (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size ota: total: %d, used: %d", total, used);
    }

    return ESP_OK;
}
