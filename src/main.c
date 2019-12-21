
#include <stdio.h>
#include <stdlib.h>
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

static void can_receive_task(void *arg)
{
    can_message_t message;
    unsigned int msg_priority;
    unsigned int node_id;
    unsigned int data_object_id;

    while (1) {
        if (can_receive(&message, pdMS_TO_TICKS(10000)) == ESP_OK) {
            gpio_set_level(GPIO_LED, 0);

            // ThingSet publication message format: https://thingset.github.io/spec/can
            msg_priority = message.identifier >> 26;
            node_id = message.identifier & 0x000000FF;
            data_object_id = (message.identifier >> 8) & 0x000000FF;

            printf("Message from node %u received with priority %u. Data object 0x%.2x = 0x",
                node_id, msg_priority, data_object_id);

            if (!(message.flags & CAN_MSG_FLAG_RTR)) {
                for (int i = 0; i < message.data_length_code; i++) {
                    printf("%.2x", message.data[i]);
                }
            }
            printf("\n");

            gpio_set_level(GPIO_LED, 1);
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
}
