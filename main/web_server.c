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
#include "ts_client.h"

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
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file.\n");
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
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file.\n");
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

static esp_err_t ts_get_handler(httpd_req_t *req)
{
    char ts_req[100];

    httpd_resp_set_type(req, "application/json");

    int pos = ts_req_hdr_from_http(ts_req, sizeof(ts_req), req->method, req->uri);

    if (pos > 0 && pos < sizeof(ts_req) - 2) {
        ts_req[pos] = '\n';
        ts_req[pos + 1] = '\0';
    }
    else {
        return ESP_FAIL;
    }

    if (ts_serial_request(ts_req, 100) != ESP_OK) {
        return ESP_FAIL;
    }

    char *resp = ts_serial_response(200);
    if (resp) {
        ESP_LOGI(TAG, "status code: %d", ts_resp_status(resp));
        if (ts_resp_status(resp) == 0x85) {
            httpd_resp_sendstr(req, ts_resp_data(resp));
        }
        else {
            // ToDo: Answer with correct HTTP status code derived from ThingSet response
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get data.");
        }
    }
    else {
        // ToDo: Answer with correct HTTP status code derived from ThingSet response
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get data.");
    }

    ts_serial_response_clear();

    return ESP_OK;
}

static esp_err_t ts_patch_handler(httpd_req_t *req)
{
    char ts_req[500];

    httpd_resp_set_type(req, "application/json");

    int len_hdr = ts_req_hdr_from_http(ts_req, sizeof(ts_req), req->method, req->uri);

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(ts_req) - len_hdr - 3);

    int len_data = httpd_req_recv(req, &ts_req[len_hdr + 1], recv_size);
    if (len_data > 0) {
        ts_req[len_hdr++] = ' ';
    }
    else {
        len_data = 0;
    }
    ts_req[len_hdr + len_data] = '\n';
    ts_req[len_hdr + len_data + 1] = '\0';

    if (ts_serial_request(ts_req, 100) != ESP_OK) {
        return ESP_FAIL;
    }

    char *resp = ts_serial_response(200);
    int status_code = ts_resp_status(resp != NULL ? resp : "");
    if (status_code == 0x84) {
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_send(req, NULL, 0);
    }
    else {
        // ToDo: Answer with correct HTTP status code derived from ThingSet response
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to patch data.");
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
    httpd_uri_t ts_get_uri = {
        .uri = "/ts/*",
        .method = HTTP_GET,
        .handler = ts_get_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_get_uri);

    httpd_uri_t ts_patch_uri = {
        .uri = "/ts/*",
        .method = HTTP_PATCH,
        .handler = ts_patch_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_patch_uri);

    httpd_uri_t ts_post_uri = {
        .uri = "/ts/*",
        .method = HTTP_POST,
        .handler = ts_get_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_post_uri);

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
