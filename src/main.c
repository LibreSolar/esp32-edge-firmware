
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/can.h"
#include "sdkconfig.h"

#define GPIO_LED        18
#define GPIO_CAN_RX     4
#define GPIO_CAN_TX     5
#define GPIO_CAN_STB    12

#define RX_TASK_PRIO    9       // receiving task priority

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
            gpio_set_level(GPIO_LED, 0);

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
            }
            else if (node_id == 10) {
                for (int i = 0; i < sizeof(data_obj_mppt)/sizeof(DataObject); i++) {
                    if (data_obj_mppt[i].id == data_object_id) {
                        memcpy(data_obj_mppt[i].raw_data, message.data, message.data_length_code);
                        data_obj_mppt[i].len = message.data_length_code;
                    }
                }
            }
            /*
            printf("Message from node %u received with priority %u. Data object 0x%.2x = 0x",
                node_id, msg_priority, data_object_id);
            if (!(message.flags & CAN_MSG_FLAG_RTR)) {
                for (int i = 0; i < message.data_length_code; i++) {
                    printf("%.2x", message.data[i]);
                }
            }
            printf("\n");
            */

            gpio_set_level(GPIO_LED, 1);
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

static void http_get_task(void *arg)
{
    char buf[500];
    int len;

    while(1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        // BMS data objects
        len = generate_json_string(buf, sizeof(buf), data_obj_bms, sizeof(data_obj_bms)/sizeof(DataObject));
        if (len > 0) {
            printf("BMS data: %s\n", buf);
        }

        // MPPT data objects
        len = generate_json_string(buf, sizeof(buf), data_obj_mppt, sizeof(data_obj_mppt)/sizeof(DataObject));
        if (len > 0) {
            printf("MPPT data: %s\n", buf);
        }
    }
}

void app_main(void)
{
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

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
