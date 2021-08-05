/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_TEST

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <string.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <mdns.h>
#include <wifi_provisioning/manager.h>

#include <wifi_provisioning/scheme_ble.h>
#include "provisioning.h"
#include "data_nodes.h"

extern GeneralConfig general_config;

extern char device_id[9];

static const char *TAG = "prov";

const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

const char prov_service_prefix[] = "PROV_LS_";

uint8_t prov_service_uuid[] = {
    /* LSB ... MSB */
    0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
    0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
};

static bool provisioning = false;

static void prov_ble_start(void)
{
    char service_name[20];
    snprintf(service_name, sizeof(service_name), "%s%s", prov_service_prefix, device_id);

    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

    wifi_prov_scheme_ble_set_service_uuid(prov_service_uuid);

    wifi_prov_mgr_start_provisioning(security, NULL, service_name, NULL);
}

static void event_handler(void *arg, esp_event_base_t event_base, int event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START: {
                ESP_LOGD(TAG, "Starting provisioning...");
                provisioning = true;
                break;
            }
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials for SSID %s",
                         (const char *) wifi_sta_cfg->ssid);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed: Wi-Fi %s\n"
                         "\tPlease reset and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "station authentication failed" : "access-point not found");
                wifi_prov_mgr_reset_sm_state_on_failure();
                provisioning = false;
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                provisioning = false;
                break;
            default:
                break;
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi connected with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
         /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi disconnected (reason %d), trying to reconnect...", event->reason);
        if (!provisioning && event->reason == WIFI_REASON_NO_AP_FOUND) {
            prov_ble_start();
        }
        esp_wifi_connect();
    }
}

static void initialize_mdns(void)
{
    char *hostname = general_config.mdns_hostname;
    // initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    // set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_LOGI(TAG, "mDNS hostname set to: %s", hostname);
    // set default mDNS instance name
    ESP_ERROR_CHECK(mdns_instance_name_set(general_config.mdns_hostname));

    mdns_txt_item_t serviceTxtData[] = {
        { "board", "esp32" }
    };

    // initialize service
    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData, 1));
}

void provision(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager, we don't need BT or BLE after provisioning */
    wifi_prov_mgr_config_t prov_cfg = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_cfg));

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (provisioned) {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    else {
        ESP_LOGI(TAG, "Starting provisioning via BLE");
        prov_ble_start();
    }

    initialize_mdns();

    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
}

#endif // UNIT_TEST
