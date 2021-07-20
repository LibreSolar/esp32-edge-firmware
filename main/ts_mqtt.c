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
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "ts_serial.h"
#include "ts_client.h"
#include "can.h"
#include "wifi.h"
#include "data_nodes.h"

MqttConfig mqtt_config;

static const char* TAG = "ts_mqtt";

#if CONFIG_THINGSET_MQTT_TLS
/* the certificate path is linked in via root CMakeLists.txt */
extern const uint8_t mqtt_root_pem_start[]  asm("_binary_mqtt_root_pem_start");
//extern const uint8_t mqtt_root_pem_end[]    asm("_binary_mqtt_root_pem_end");
#endif

static void send_data(esp_mqtt_client_handle_t client, char *device_id, char *data, int size)
{
    char mqtt_topic[256];
    snprintf(mqtt_topic, sizeof(mqtt_topic), "dat/%s/%s", mqtt_config.username, device_id);
    int msg_id = esp_mqtt_client_publish(client, mqtt_topic, data, size, 0, 0);
    ESP_LOGI(TAG, "message sent to %s with msg_id=%d", mqtt_topic, msg_id);
}

/*
 * Event handler called by the MQTT client event loop. Currently only used for debugging
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id,
    void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            //ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            //if (strncmp(event->data, "send binary please", event->data_len) == 0) {
            //    ESP_LOGI(TAG, "Sending the binary");
            //    send_binary(client);
            //}
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void ts_mqtt_pub_task(void *arg)
{
    TSDevice ts_device;
    ts_device.ts_device_id = NULL;
    bool device_found = false;
    esp_err_t err;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_config.broker_hostname,
#if CONFIG_THINGSET_MQTT_TLS
        .cert_pem = (const char *)mqtt_root_pem_start,
#endif
        };
        if (mqtt_config.use_broker_auth) {
            mqtt_cfg.username = mqtt_config.username;
            mqtt_cfg.password = mqtt_config.password;
        }

    // wait 3s for device to boot
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    // the last argument may be used to pass data to the event handler
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    TickType_t mqtt_pub_ticks = xTaskGetTickCount();

    while (1) {
        if (!device_found) {
            err = ts_serial_scan_device_info(&ts_device);
            if (err) {
                ESP_LOGE(TAG, "No device found, waiting 1 minute");
                vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
                continue;
            }
            else {
                device_found = true;
            }
        }

        // wait until we get a serial publication message
        char *pub_msg = ts_serial_pubmsg(1000);
        while (pub_msg == NULL) {
            pub_msg = ts_serial_pubmsg(1000);
            printf("Waiting for pub msg\n");
        }

        int data_len = strlen(pub_msg) - 2;
        if (data_len > 0) {
            gpio_set_level(CONFIG_GPIO_LED, 0);
            send_data(client, ts_device.ts_device_id, pub_msg + 2, data_len);
            printf("Received: %s\n", pub_msg);
            ts_serial_pubmsg_clear();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(CONFIG_GPIO_LED, 1);

        vTaskDelayUntil(&mqtt_pub_ticks,
            mqtt_config.pub_interval * 1000 / portTICK_PERIOD_MS);
    }
}

#endif /* UNIT_TEST */
