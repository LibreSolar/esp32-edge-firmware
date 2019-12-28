/*
 * Copyright (c) 2019 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include "driver/can.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "custom_conf.h"

#define GPIO_LED        18
#define GPIO_CAN_RX     4
#define GPIO_CAN_TX     5
#define GPIO_CAN_STB    12

#define RX_TASK_PRIO    9       // receiving task priority
static const char* TAG = "main";    // for logging

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one
   event - are we connected to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static bool update_bms_received = false;
static bool update_mppt_received = false;

static const can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS();
static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
static const can_general_config_t g_config =
    CAN_GENERAL_CONFIG_DEFAULT(GPIO_CAN_TX, GPIO_CAN_RX, CAN_MODE_NORMAL);

typedef struct {
    int id;
    const char *name;
    uint8_t raw_data[8];
    int len;
} DataObject;

#define CAN_TS_T_TRUE 61
#define CAN_TS_T_FALSE 60
#define CAN_TS_T_POS_INT32  6
#define CAN_TS_T_NEG_INT32  7
#define CAN_TS_T_FLOAT32 30
#define CAN_TS_T_DECFRAC 36

DataObject data_obj_bms[] = {
    {0x01, "Bat_V",     {0}, 0},
    {0x02, "Bus_V",     {0}, 0},
    {0x03, "Cell1_V",   {0}, 0},
    {0x04, "Cell2_V",   {0}, 0},
    {0x05, "Cell3_V",   {0}, 0},
    {0x06, "Cell4_V",   {0}, 0},
    {0x07, "Cell5_V",   {0}, 0},
    {0x08, "Bat_A",     {0}, 0},
    {0x09, "Bat_degC",  {0}, 0},
    {0x0A, "SOC",       {0}, 0}
};

DataObject data_obj_mppt[] = {
    {0x04, "LoadState",     {0}, 0},
    {0x0F, "SolarMaxDay_W", {0}, 0},
    {0x10, "LoadMaxDay_W",  {0}, 0},
    {0x70, "Bat_V",         {0}, 0},
    {0x71, "Solar_V",       {0}, 0},
    {0x72, "Bat_A",         {0}, 0},
    {0x73, "Load_A",        {0}, 0},
    {0x74, "Bat_degC",      {0}, 0},
    {0x76, "Int_degC",      {0}, 0},
    {0x77, "Mosfet_degC",   {0}, 0},
    {0x78, "ChgState",      {0}, 0},
    {0x79, "DCDCState",     {0}, 0},
    {0x7a, "Solar_A",       {0}, 0},
    {0x7d, "Bat_W",         {0}, 0},
    {0x7e, "Solar_W",       {0}, 0},
    {0x7f, "Load_W",        {0}, 0},
    {0xa0, "SolarInDay_Wh", {0}, 0},
    {0xa1, "LoadOutDay_Wh", {0}, 0},
    {0xa2, "BatChgDay_Wh",  {0}, 0},
    {0xa3, "BatDisDay_Wh",  {0}, 0},
    {0x06, "SOC",           {0}, 0},
    {0xA4, "Dis_Ah",        {0}, 0}
};

static void can_receive_task(void *arg)
{
    can_message_t message;
    //unsigned int msg_priority;        // currently not used
    unsigned int node_id;
    unsigned int data_object_id;

    while (1) {
        if (can_receive(&message, pdMS_TO_TICKS(10000)) == ESP_OK) {

            // ThingSet publication message format: https://thingset.github.io/spec/can
            //msg_priority = message.identifier >> 26;
            node_id = message.identifier & 0x000000FF;
            data_object_id = (message.identifier >> 8) & 0x000000FF;

            if (node_id == 0) {
                for (int i = 0; i < sizeof(data_obj_bms)/sizeof(DataObject); i++) {
                    if (data_obj_bms[i].id == data_object_id) {
                        memcpy(data_obj_bms[i].raw_data, message.data, message.data_length_code);
                        data_obj_bms[i].len = message.data_length_code;
                    }
                }
                update_bms_received = true;
            }
            else if (node_id == 10) {
                for (int i = 0; i < sizeof(data_obj_mppt)/sizeof(DataObject); i++) {
                    if (data_obj_mppt[i].id == data_object_id) {
                        memcpy(data_obj_mppt[i].raw_data, message.data, message.data_length_code);
                        data_obj_mppt[i].len = message.data_length_code;
                    }
                }
                update_mppt_received = true;
            }

            /*
            printf("CAN Message from node %u received with priority %u. Data object 0x%.2x = 0x",
                node_id, msg_priority, data_object_id);
            if (!(message.flags & CAN_MSG_FLAG_RTR)) {
                for (int i = 0; i < message.data_length_code; i++) {
                    printf("%.2x", message.data[i]);
                }
            }
            printf("\n");
            */
        }
    }
}

static int generate_json_string(char *buf, size_t len, DataObject *objs, size_t num_objs)
{
    union float2bytes { float f; char b[4]; } f2b;     // for conversion of float to single bytes
    int pos = 0;

    for (int i = 0; i < num_objs; i++) {

        if (objs[i].raw_data[0] == 0) {
            continue;
        }

        // print data object ID
        if (pos == 0) {
            pos += snprintf(&buf[pos], len - pos, "\"%s\":", objs[i].name);
        }
        else {
            pos += snprintf(&buf[pos], len - pos, ",\"%s\":", objs[i].name);
        }

        float value = 0.0;
        uint32_t value_abs;

        // print value
        switch (objs[i].raw_data[0]) {
            case CAN_TS_T_TRUE:
            case CAN_TS_T_FALSE:
                pos += snprintf(&buf[pos], len - pos, "%d", (objs[i].raw_data[0] == CAN_TS_T_TRUE) ? 1 : 0);
                break;
            case CAN_TS_T_POS_INT32:
                value_abs =
                    (objs[i].raw_data[1] << 24) +
                    (objs[i].raw_data[2] << 16) +
                    (objs[i].raw_data[3] << 8) +
                    (objs[i].raw_data[4]);
                pos += snprintf(&buf[pos], len - pos, "%u", value_abs);
                break;
            case CAN_TS_T_NEG_INT32:
                value_abs =
                    (objs[i].raw_data[1] << 24) +
                    (objs[i].raw_data[2] << 16) +
                    (objs[i].raw_data[3] << 8) +
                    (objs[i].raw_data[4]);
                pos += snprintf(&buf[pos], len - pos, "%d", -(int32_t)(value_abs + 1));
                break;
            case CAN_TS_T_FLOAT32:
                f2b.b[3] = objs[i].raw_data[1];
                f2b.b[2] = objs[i].raw_data[2];
                f2b.b[1] = objs[i].raw_data[3];
                f2b.b[0] = objs[i].raw_data[4];
                pos += snprintf(&buf[pos], len - pos, "%.3f", f2b.f);
                break;
            case CAN_TS_T_DECFRAC:
                value_abs =
                    (objs[i].raw_data[4] << 24) +
                    (objs[i].raw_data[5] << 16) +
                    (objs[i].raw_data[6] << 8) +
                    (objs[i].raw_data[7]);

                // dirty hack: We know that currently decfrac is only used for mV or mA
                if (objs[i].raw_data[3] == 0x1a &&
                    objs[i].raw_data[2] == 0x22)
                {
                    // positive int32 with exp -3
                    value = (float)value_abs / 1000.0;
                }
                else if (objs[i].raw_data[3] == 0x3a &&
                    objs[i].raw_data[2] == 0x22)
                {
                    // negative int32 with exp -3
                    value = -((float)value_abs + 1.0) / 1000.0;
                }
                else {
                    pos += snprintf(&buf[pos], len - pos, "err");
                }
                pos += snprintf(&buf[pos], len - pos, "%.3f", value);
                break;
        }
    }
    return pos;
}

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

static int send_emoncms(struct addrinfo *res, const char *node_name, DataObject *objs, size_t num_objs)
{
    static char buf[500];
    static char http_body[500];
    char recv_buf[64];

    const char *http_header =
        "POST " EMONCMS_URL " HTTP/1.1\r\n"
        "Host: " EMONCMS_HOST "\r\n"
        "Authorization: " EMONCMS_APIKEY "\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Connection: close\r\n";

    int pos = snprintf(http_body, sizeof(http_body), "node=%s&json=", node_name);
    pos += generate_json_string(&http_body[pos], sizeof(http_body) - pos, objs, num_objs);
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
        while (update_bms_received == false && update_mppt_received == false) {
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
            update_bms_received = false;
            gpio_set_level(GPIO_LED, 0);
            send_emoncms(res, EMONCMS_NODE_BMS,
                data_obj_bms, sizeof(data_obj_bms)/sizeof(DataObject));
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(GPIO_LED, 1);

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (update_mppt_received) {
            update_mppt_received = false;
            gpio_set_level(GPIO_LED, 0);
            send_emoncms(res, EMONCMS_NODE_MPPT,
                data_obj_mppt, sizeof(data_obj_mppt)/sizeof(DataObject));
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        gpio_set_level(GPIO_LED, 1);

        // sending interval almost 10s
        vTaskDelay(9000 / portTICK_PERIOD_MS);

        freeaddrinfo(res);
    }
}


void app_main(void)
{
    nvs_flash_init();

    // configure the pad to GPIO and set direction
    gpio_pad_select_gpio(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_LED, 1);

    // switch CAN transceiver on (STB = low)
    gpio_pad_select_gpio(GPIO_CAN_STB);
    gpio_set_direction(GPIO_CAN_STB, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_CAN_STB, 0);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
 	printf("Booting Libre Solar Data Manager...\n");

    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("CAN driver installed\n");
    }
    else {
        printf("Failed to install CAN driver\n");
        return;
    }

    if (can_start() == ESP_OK) {
        printf("CAN driver started\n");
    }
    else {
        printf("Failed to start CAN driver\n");
        return;
    }

    xTaskCreatePinnedToCore(can_receive_task, "CAN_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

    initialise_wifi();
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
