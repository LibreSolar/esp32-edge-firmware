/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UNIT_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ts_serial.h"
#include "ts_client.h"
#include "ts_mqtt.h"
#include "can.h"
#include "emoncms.h"
#include "wifi.h"
#include "web_fs.h"
#include "web_server.h"
#include "provisioning.h"
#include "data_nodes.h"

#define RX_TASK_PRIO    9       // receiving task priority
extern EmoncmsConfig emon_config;
extern MqttConfig mqtt_config;
extern GeneralConfig general_config;

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    data_nodes_init();

    // configure the LED pad as GPIO and set direction
    gpio_pad_select_gpio(CONFIG_GPIO_LED);
    gpio_set_direction(CONFIG_GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_GPIO_LED, 1);
    // wait 3s to open serial terminal after flashing finished
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("Booting Libre Solar Data Manager...\n");

    init_fs();

    if (general_config.ts_can_active) {
        can_setup();
        xTaskCreatePinnedToCore(can_receive_task, "CAN_rx", 4096,
        NULL, RX_TASK_PRIO, NULL, 1);
    }

    if (general_config.ts_serial_active) {
        ts_serial_setup();

        xTaskCreatePinnedToCore(ts_serial_rx_task, "ts_serial_rx", 4096,
            NULL, RX_TASK_PRIO, NULL, 1);
        ts_scan_devices();
    }

    if (strlen(general_config.wifi_ssid) > 0) {
        wifi_connect();
    }
    else {
        // no hard-coded WiFi credentials --> start provisioning via BLE
        provision();
    }

    start_web_server("/www");

    if (emon_config.active) {
        xTaskCreate(&emoncms_post_task, "emoncms_post_task", 4096, NULL, 5, NULL);
    }

    if (mqtt_config.active) {
        xTaskCreate(&ts_mqtt_pub_task, "mqtt_pub", 4096, NULL, 5, NULL);
    }
}

#endif //unit tests