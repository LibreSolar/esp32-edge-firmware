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
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ts_serial.h"
#include "can.h"
#include "data_nodes.h"

static const char* TAG = "emoncms";

extern EmoncmsConfig emon_config;

char http_header[1024];

void build_header()
{
    snprintf(http_header, sizeof(http_header),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Authorization: %s\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Connection: close\r\n", emon_config.url, emon_config.emoncms_hostname, emon_config.api_key);
        ESP_LOGD(TAG, "Header (%d bytes): \n%s", strlen(http_header), http_header);
}

static int send_emoncms(struct addrinfo *res, const char *node_name, const char *json_str)
{
    static char buf[500];
    static char http_body[600];
    char recv_buf[64];

    snprintf(http_body, sizeof(http_body), "node=%s&json=%s", node_name, json_str);
    printf("HTTP body for %s: %s\n", node_name, http_body);

    int s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0) {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        return -1;
    }
    ESP_LOGI(TAG, "... allocated socket\r\n");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        return -1;
    }
    ESP_LOGI(TAG, "... connected");

    int err = write(s, http_header, strlen(http_header));
    sprintf(buf, "Content-Length: %d\r\n\r\n", (int)strlen(http_body));
    err += write(s, buf, strlen(buf));
    err += write(s, http_body, strlen(http_body));
    err += write(s, "\r\n", 1);

    if (err < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        return -1;
    }
    ESP_LOGI(TAG, "... socket send success");

    // Read HTTP response
    int resp;
    do {
        bzero(recv_buf, sizeof(recv_buf));
        resp = read(s, recv_buf, sizeof(recv_buf)-1);
        for(int i = 0; i < resp; i++) {
            putchar(recv_buf[i]);
        }
    } while (resp > 0);

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", resp, errno);
    close(s);

    return 1;
}

void emoncms_post_task(void *arg)
{
    build_header();
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;

    while (1) {
        esp_err_t err;

        // attempt to get serial publication message
        char *pub_msg = ts_serial_pubmsg(100);

        // wait until we receive an update
        while (update_bms_received == false &&
               update_mppt_received == false &&
               pub_msg == NULL)
        {
            // try again as long as a message from
            pub_msg = ts_serial_pubmsg(100);
            vTaskDelay(100/portTICK_PERIOD_MS);
        }

        //esp_netif_ip_info_t ip_info;
        //err = esp_netif_get_ip_info(wifi_get_netif, &ip_info);

        err = getaddrinfo(emon_config.emoncms_hostname, emon_config.port, &hints, &res);

        if (err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // Code to print the resolved IP.
        // Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        if (update_bms_received) {
            gpio_set_level(CONFIG_GPIO_LED, 0);
            send_emoncms(res, emon_config.bms, get_bms_json_data());
            update_bms_received = false;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(CONFIG_GPIO_LED, 1);

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (update_mppt_received) {
            gpio_set_level(CONFIG_GPIO_LED, 0);
            send_emoncms(res, emon_config.mppt, get_mppt_json_data());
            update_mppt_received = false;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(CONFIG_GPIO_LED, 1);

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (pub_msg != NULL && strlen(pub_msg) > 2) {
            gpio_set_level(CONFIG_GPIO_LED, 0);
            send_emoncms(res, emon_config.serial_node, pub_msg + 2);
            ts_serial_pubmsg_clear();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(CONFIG_GPIO_LED, 1);

        // sending interval almost 10s
        vTaskDelay(8000 / portTICK_PERIOD_MS);

        freeaddrinfo(res);
    }
}

#endif // UNIT_TEST