/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include <sys/param.h>

#include "ts_serial.h"

static const char *TAG = "websrv";

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct web_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} web_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    web_server_context_t *server_ctx = (web_server_context_t *)req->user_ctx;
    strlcpy(filepath, server_ctx->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = server_ctx->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        }
        else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Simple handler for light brightness control */
static esp_err_t light_brightness_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((web_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int red = cJSON_GetObjectItem(root, "red")->valueint;
    int green = cJSON_GetObjectItem(root, "green")->valueint;
    int blue = cJSON_GetObjectItem(root, "blue")->valueint;
    ESP_LOGI(TAG, "Light control: red = %d, green = %d, blue = %d", red, green, blue);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t json_data_get_handler(httpd_req_t *req)
{
    char ts_req[300];

    httpd_resp_set_type(req, "application/json");

    ESP_LOGI(TAG, "URI: %s", req->uri);

    switch (req->method) {
        case HTTP_GET:
            ts_req[0] = '?';
            break;
        case HTTP_POST:
            ts_req[0] = '!';
            break;
        case HTTP_PATCH:
            ts_req[0] = '=';
            break;
        case HTTP_DELETE:
            ts_req[0] = '-';
            break;
        default:
            ts_req[0] = '\0';   // empty string
    }

    int pos_function = strlen("/devices/serial/");
    int len_function = strlen(req->uri) - pos_function;
    if (len_function > 0 && len_function < sizeof(ts_req) - 3) {
        strncpy(&ts_req[1], req->uri + pos_function, sizeof(ts_req) - 3);

        /* Truncate if content length larger than the buffer */
        size_t recv_size = MIN(req->content_len, sizeof(ts_req) - len_function - 3);

        int ret =  httpd_req_recv(req, &ts_req[len_function + 2], recv_size);
        if (ret > 0) {
            ts_req[len_function + 1] = ' ';
            ts_req[len_function + 2 + ret] = '\n';
            ts_req[len_function + 3 + ret] = '\0';
        }
        else if (ret == 0) {
            ts_req[len_function + 1] = '\n';
            ts_req[len_function + 2] = '\0';
        }
    }

    if (ts_serial_request(ts_req, 100) != ESP_OK) {
        return ESP_FAIL;
    }

    char *resp = ts_serial_response(200);
    if (resp) {
        if (req->method == HTTP_GET) {
            httpd_resp_sendstr(req, resp);
        }
        else {
            httpd_resp_set_status(req, "204 No Content");
            httpd_resp_sendstr(req, "");
        }
    }
    else {
        httpd_resp_sendstr(req, "ERROR");
    }

    ts_serial_response_clear();

    return ESP_OK;
}

esp_err_t start_web_server(const char *base_path)
{
    if (base_path == NULL) {
        ESP_LOGE(TAG, "Wrong base path");
        return ESP_FAIL;
    }

    web_server_context_t *server_ctx = calloc(1, sizeof(web_server_context_t));
    if (server_ctx == NULL) {
        ESP_LOGE(TAG, "No memory for web server");
        return ESP_FAIL;
    }
    strlcpy(server_ctx->base_path, base_path, sizeof(server_ctx->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Start server failed");
        free(server_ctx);
        return ESP_FAIL;
    }

    /* URI handler for fetching JSON data */
    httpd_uri_t json_data_get_uri = {
        .uri = "/devices/*",
        .method = HTTP_GET,
        .handler = json_data_get_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &json_data_get_uri);

    httpd_uri_t json_data_patch_uri = {
        .uri = "/devices/*",
        .method = HTTP_PATCH,
        .handler = json_data_get_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &json_data_patch_uri);

    httpd_uri_t json_data_post_uri = {
        .uri = "/devices/*",
        .method = HTTP_POST,
        .handler = json_data_get_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &json_data_post_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = common_get_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
}
