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
#include "esp_event_loop.h"
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

#include "serial.h"
#include "can.h"
#include "custom_conf.h"

// simple watchdog to reset device if WiFi connection was interrupted (see main function)
int watchdog_counter = 0;

#define RX_TASK_PRIO    9       // receiving task priority

static const char* TAG = "main";    // for logging

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one
   event - are we connected to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
            auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_NONE) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static int send_emoncms(struct addrinfo *res, const char *node_name, const char *json_str)
{
    static char buf[500];
    static char http_body[600];
    char recv_buf[64];

    const char *http_header =
        "POST " EMONCMS_URL " HTTP/1.1\r\n"
        "Host: " EMONCMS_HOST "\r\n"
        "Authorization: " EMONCMS_APIKEY "\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Connection: close\r\n";

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
    sprintf(buf, "Content-Length: %d\r\n\r\n", strlen(http_body));
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

    watchdog_counter = 0;   // reset WiFi watchdog
    return 1;
}

static void http_get_task(void *arg)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;

    while (1) {

        // wait until we receive an update
        while (update_bms_received == false &&
               update_mppt_received == false &&
               update_serial_received == false)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // Wait for the callback to set the CONNECTED_BIT in the event group.
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        int err = getaddrinfo(EMONCMS_HOST, EMONCMS_PORT, &hints, &res);

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
            gpio_set_level(GPIO_LED, 0);
            send_emoncms(res, EMONCMS_NODE_BMS, get_bms_json_data());
            update_bms_received = false;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(GPIO_LED, 1);

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (update_mppt_received) {
            gpio_set_level(GPIO_LED, 0);
            send_emoncms(res, EMONCMS_NODE_MPPT, get_mppt_json_data());
            update_mppt_received = false;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(GPIO_LED, 1);

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (update_serial_received) {
            gpio_set_level(GPIO_LED, 0);
            send_emoncms(res, EMONCMS_NODE_SERIAL, get_serial_json_data());
            update_serial_received = false;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(GPIO_LED, 1);

        // sending interval almost 10s
        vTaskDelay(8000 / portTICK_PERIOD_MS);

        freeaddrinfo(res);
    }
}

void app_main(void)
{
    nvs_flash_init();

    // configure the LED pad as GPIO and set direction
    gpio_pad_select_gpio(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_LED, 1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
 	printf("Booting Libre Solar Data Manager...\n");

#if defined(GPIO_CAN_RX) && defined(GPIO_CAN_TX)
    can_setup();
    xTaskCreatePinnedToCore(can_receive_task, "CAN_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
#endif

#if defined(GPIO_UART_RX) && defined(GPIO_UART_TX)
    uart_setup();
    xTaskCreatePinnedToCore(uart_rx_task, "UART_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
#endif

    initialise_wifi();
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);

    while (1) {
        if (watchdog_counter > 60) {
            printf("Restarting ESP because of interrupted WiFi connection for 60s\n");
            esp_restart();
        }
        watchdog_counter++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
