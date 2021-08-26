/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UNIT_TEST

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
#include "data_nodes.h"
#include "ota.h"

//just temporary until we implemented a way to select the chip
#include "stm32bl.h"

static const char *TAG = "websrv";

static int url_offset_ts;
static int url_offset_ota;

extern char device_id[9];

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct web_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} web_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Create proper HTTP Response Code */

const char *translate_status_code(uint8_t ts_status_code)
{
    switch (ts_status_code){
        case  TS_STATUS_CREATED:
            return "201";
        case TS_STATUS_VALID:
        case TS_STATUS_CONTENT:
            return "200";
        case TS_STATUS_CHANGED:
        case TS_STATUS_DELETED:
            return "204";
        case TS_STATUS_BAD_REQUEST:
        case TS_STATUS_REQUEST_INCOMPLETE:
            return "400";
        case TS_STATUS_UNAUTHORIZED:
            return "401";
        case TS_STATUS_FORBIDDEN:
            return "403";
        case TS_STATUS_NOT_FOUND:
            return "404";
        case TS_STATUS_METHOD_NOT_ALLOWED:
            return "405";
        case TS_STATUS_CONFLICT:
            return "409";
        case TS_STATUS_REQUEST_TOO_LARGE:
            return "413";
        case TS_STATUS_UNSUPPORTED_FORMAT:
            return "415";
        case TS_STATUS_INTERNAL_SERVER_ERR:
            return "500";
        case TS_STATUS_NOT_IMPLEMENTED:
            return "501";
        default:
            return "500";
    }
}

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
    set_content_type_from_file(req, filepath);

    if (strstr(filepath, ".png") == NULL) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        strlcat(filepath, ".gz", sizeof(filepath));
    }

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file.\n");
        return ESP_FAIL;
    }

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

static char *receive_content(httpd_req_t *req)
{
    // +1 for termination
    char *buf = (char *) malloc(req->content_len+1);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for content");
        return NULL;
    }
    int bytes_received = 0;
    while (req->content_len - bytes_received > 0) {
        bytes_received += httpd_req_recv(req, buf, req->content_len);
    }
    ESP_LOGD(TAG, "received %d bytes: %s", bytes_received, buf);
    buf[req->content_len] = '\0';
    return buf;
}

static esp_err_t get_content(httpd_req_t *req, char **content)
{
    if (req->content_len != 0) {
        *content = receive_content(req);
    }
    if (*content == NULL && req->content_len > 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t send_response(httpd_req_t *req, TSResponse *res)
{
    httpd_resp_set_status(req, translate_status_code(res->ts_status_code));
    httpd_resp_set_type(req, "application/json");
    if (res->data != NULL) {
        ESP_LOGD(TAG, "Sending out data: %s", res->data);
        // res->data points to res->block behind the "header" section of ts-response
        httpd_resp_sendstr(req, res->data);
        heap_caps_free(res->block);
    } else {
        httpd_resp_send(req, NULL, 0);
    }
    heap_caps_free(res);
    return ESP_OK;
}

static esp_err_t ts_get_devices_handler(httpd_req_t *req)
{
    char *names = ts_get_device_list();
    if (names != NULL) {
        httpd_resp_set_status(req, "200");
        httpd_resp_sendstr(req, names);
        free(names);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get device list.");
    }
    return ESP_OK;
}

static esp_err_t ts_handler(httpd_req_t *req)
{
    if (req->uri[url_offset_ts] == '\0') {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resource not found");
        return ESP_OK;
    }
    char *content = NULL;
    if (get_content(req, &content) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not receive content");
        return ESP_FAIL;
    }

    TSResponse *res = ts_execute(req->uri + url_offset_ts, content, req->method);
    if (content != NULL) {
        heap_caps_free(content);
    }

    if (res == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Device not connected");
        return ESP_OK;
    }
    return send_response(req, res);
}

esp_err_t esp_ota_start_handler(httpd_req_t *req)
{
    cJSON *res = cJSON_CreateObject();
    esp_err_t err = esp_ota_handler(req, res);
    char *res_string = cJSON_Print(res);
    cJSON_Delete(res);

    if (err == ESP_OK) {
        httpd_resp_set_status(req, "200");
        httpd_resp_sendstr(req, res_string);
        reset_device();
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, res_string);
        err = ESP_FAIL;
    }

    free(res_string);
    return err;
}

static esp_err_t stm_ota_start_handler(httpd_req_t *req)
{
    const char endpoint[] = "/dfu";
    int id_len = strlen(req->uri + url_offset_ota);

    char *uri = (char *) malloc(sizeof(endpoint) + id_len);
    strncpy(uri, req->uri + url_offset_ota, id_len);
    strcpy(uri + id_len, "/dfu");
    ESP_LOGI(TAG, "URL: %s", uri);

    TSResponse *res = ts_execute(uri, NULL, HTTP_GET);
    free(uri);
    if (res == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Device not connected");
        return ESP_OK;
    }

    cJSON *info = cJSON_Parse(res->data);
    if (info == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to parse device information");
        return ESP_OK;
    }
    cJSON *flash_size = cJSON_GetObjectItemCaseSensitive(info, "FlashSize_KiB");
    cJSON *page_size = cJSON_GetObjectItemCaseSensitive(info, "FlashPageSize_B");
    if (flash_size == NULL || page_size == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST , "Device does not support updates yet");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/plain");

    int ret = ts_serial_ota(flash_size->valueint, page_size->valueint);
    /* Give it time to reboot, otherwise subsequent request could fail */
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (ret != ESP_FAIL) {
        httpd_resp_sendstr(req, "OTA successful.");
    }
    else {
        httpd_resp_sendstr(req, "OTA failed.");
    }
    cJSON_Delete(info);
    return ESP_OK;
}

esp_err_t ota_upload_handler(httpd_req_t *req)
{
    /* check if target is self and invoke the proper handler */
    if (strstr(req->uri, device_id)) {
        return esp_ota_start_handler(req);
    }

    if (req->content_len > 0x20000) {
        httpd_resp_set_status(req, "413");
        httpd_resp_sendstr(req, "Maximum size: 128 kByte");
        return ESP_OK;
    }

    unsigned int bytes_received = 0;
    unsigned int bytes_written = 0;
    int buffer_size = 8 * 1024;
    char * buf = (char *) malloc(buffer_size);
    FILE *fd = fopen("/stm_ota/firmware.bin", "w");
    if (fd == NULL) {
        ESP_LOGE(TAG, "Failed to create file for image");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to create file");
        goto out;
    }
    while (req->content_len - bytes_received > 0) {
        int chunk_size = httpd_req_recv(req, buf, buffer_size);
        bytes_received += chunk_size;
        ESP_LOGD(TAG, "Got chunk of %d bytes", chunk_size);
        bytes_written = fwrite(buf, 1, chunk_size, fd);
        ESP_LOGD(TAG, "Wrote chunk of %d bytes", bytes_written);
        if (ferror(fd)) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to write file");
            goto out;
        }
    }
    ESP_LOGD(TAG, "Wrote %d to flash", bytes_received);
    httpd_resp_set_status(req, "200");
    httpd_resp_send(req, NULL, 0);
out:
    free(buf);
    fclose(fd);
    return ESP_OK;
}

esp_err_t start_web_server(const char *base_path)
{
    if (base_path == NULL) {
        ESP_LOGE(TAG, "Wrong base path");
        return ESP_FAIL;
    }

    web_server_context_t *server_ctx = (web_server_context_t *) calloc(1, sizeof(web_server_context_t));
    if (server_ctx == NULL) {
        ESP_LOGE(TAG, "No memory for web server");
        return ESP_FAIL;
    }
    strlcpy(server_ctx->base_path, base_path, sizeof(server_ctx->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.core_id = 1;
    config.max_uri_handlers = 16;
    config.stack_size = 8*1024;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 20;

    ESP_LOGI(TAG, "Starting HTTP Server");
    url_offset_ts = strlen("/ts/");
    url_offset_ota = strlen("/ota/");

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Start server failed");
        free(server_ctx);
        return ESP_FAIL;
    }

    /* URI handler to get connected device list */
    httpd_uri_t ts_get_devices_uri = {
        .uri = "/ts/",
        .method = HTTP_GET,
        .handler = ts_get_devices_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_get_devices_uri);

    /* URI handler for fetching JSON data */
    httpd_uri_t ts_get_uri = {
        .uri = "/ts/*",
        .method = HTTP_GET,
        .handler = ts_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_get_uri);

    httpd_uri_t ts_patch_uri = {
        .uri = "/ts/*",
        .method = HTTP_PATCH,
        .handler = ts_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_patch_uri);

    httpd_uri_t ts_post_uri = {
        .uri = "/ts/*",
        .method = HTTP_POST,
        .handler = ts_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_post_uri);

    httpd_uri_t ts_delete_uri = {
        .uri = "/ts/*",
        .method = HTTP_DELETE,
        .handler = ts_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ts_delete_uri);

    httpd_uri_t stm_ota_start_uri = {
        .uri = "/ota/*",
        .method = HTTP_GET,
        .handler = stm_ota_start_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &stm_ota_start_uri);

    httpd_uri_t ota_upload_uri = {
        .uri = "/ota/*",
        .method = HTTP_POST,
        .handler = ota_upload_handler,
        .user_ctx = server_ctx
    };
    httpd_register_uri_handler(server, &ota_upload_uri);

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

#endif // UNIT_TEST