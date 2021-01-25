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

// temporary function to test successful SPIFFS file upload
static void read_hello_txt()
{
    ESP_LOGI(TAG, "Reading index.html");

    FILE* f = fopen("/www/index.html", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open index.html");
        return;
    }

    char buf[512];
    memset(buf, 0, sizeof(buf));
    fread(buf, 1, sizeof(buf), f);
    fclose(f);

    ESP_LOGI(TAG, "File content:\n%s", buf);
}

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
        .format_if_mount_failed = false
    };
    if(check_response(esp_vfs_spiffs_register(&ota_conf)) != ESP_OK) {
        return ESP_FAIL;
    };

    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info("website", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    read_hello_txt();

    return ESP_OK;
}
