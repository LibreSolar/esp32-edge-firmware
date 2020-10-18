/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include "ts_client.h"

#include <stdio.h>
#include <stdlib.h>

#include "esp_http_server.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "ts_client";

char *ts_resp_data(char *resp)
{
    if (resp[0] == ':') {
        char *pos = strstr(resp, ". ");
        if (pos != NULL) {
            return pos + 2;
        }
    }
    return NULL;
}

int ts_resp_status(char *resp)
{
    int status_code = 0;
    int ret = sscanf(resp, ":%X ", &status_code);
    if (ret > 0) {
        return status_code;
    }
    else {
        return -1;
    }
}

int ts_req_hdr_from_http(char *buf, size_t buf_size, int method, const char *uri)
{
    ESP_LOGI(TAG, "URI: %s, method: %d", uri, method);

    switch (method) {
        case HTTP_GET:
            buf[0] = '?';
            break;
        case HTTP_POST:
            buf[0] = '!';
            break;
        case HTTP_PATCH:
            buf[0] = '=';
            break;
        default:
            buf[0] = '\0';   // empty string
    }

    int pos_function = strlen("/ts/serial/");
    int len_function = strlen(uri) - pos_function;
    if (len_function > 0) {
        int ret = snprintf(buf + 1, buf_size - 2, "%s", uri + pos_function);
        if (ret > 0) {
            return ret + 1;
        }
        else {
            buf[1] = '\0';   // terminate properly
        }
    }
    return 0;
}
