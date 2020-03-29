/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2019 Martin Jäger / Libre Solar
 */

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
#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "can.h"
#include "emoncms.h"
#include "serial.h"
#include "wifi.h"
#include "web_fs.h"
#include "web_server.h"

#define RX_TASK_PRIO    9       // receiving task priority

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // configure the LED pad as GPIO and set direction
    gpio_pad_select_gpio(CONFIG_GPIO_LED);
    gpio_set_direction(CONFIG_GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_GPIO_LED, 1);

    // wait 3s to open serial terminal after flashing finished
    vTaskDelay(3000 / portTICK_PERIOD_MS);
 	printf("Booting Libre Solar Data Manager...\n");

    init_fs();

#if CONFIG_THINGSET_CAN
    can_setup();
    xTaskCreatePinnedToCore(can_receive_task, "CAN_rx", 4096,
        NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
#endif

#if CONFIG_THINGSET_SERIAL
    uart_setup();
    xTaskCreatePinnedToCore(uart_rx_task, "UART_rx", 4096,
        NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
#endif

    wifi_connect();

    start_web_server("/www");

#if CONFIG_EMONCMS
    xTaskCreate(&emoncms_post_task, "emoncms_post_task", 4096,
        NULL, 5, NULL);
#endif
}
