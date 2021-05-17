/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TS_CLIENT_H_
#define TS_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>

/*
 * Protocol function codes (same as CoAP)
 */
#define TS_GET      0x01
#define TS_POST     0x02
#define TS_DELETE   0x04
#define TS_FETCH    0x05
#define TS_PATCH    0x07        // it's actually iPATCH

#define TS_PUBMSG   0x1F

/*
 * Status codes (same as CoAP)
 */

// success
#define TS_STATUS_CREATED               0x81
#define TS_STATUS_DELETED               0x82
#define TS_STATUS_VALID                 0x83
#define TS_STATUS_CHANGED               0x84
#define TS_STATUS_CONTENT               0x85

// client errors
#define TS_STATUS_BAD_REQUEST           0xA0
#define TS_STATUS_UNAUTHORIZED          0xA1        // need to authenticate
#define TS_STATUS_FORBIDDEN             0xA3        // trying to write read-only value
#define TS_STATUS_NOT_FOUND             0xA4
#define TS_STATUS_METHOD_NOT_ALLOWED    0xA5
#define TS_STATUS_REQUEST_INCOMPLETE    0xA8
#define TS_STATUS_CONFLICT              0xA9
#define TS_STATUS_REQUEST_TOO_LARGE     0xAD
#define TS_STATUS_UNSUPPORTED_FORMAT    0xAF

// server errors
#define TS_STATUS_INTERNAL_SERVER_ERR   0xC0
#define TS_STATUS_NOT_IMPLEMENTED       0xC1

// ThingSet specific errors
#define TS_STATUS_RESPONSE_TOO_LARGE    0xE1

/**
* Struct to hold device information
* when a new device is connected, the function pointer
* should be set accordingly to either the serial or CAN send function
*/
typedef struct {
    uint8_t CAN_Address;
    char *ts_device_id;
    char *ts_name;
    //function pointer to send requests to device, abstracting underlying connection
    char *(*send)(char * req, uint8_t CAN_Address);
} TSDevice;

/**
* Struct to hold ts response values for the http handlers
*/
typedef struct {
    uint8_t ts_status_code;
    char *data;
    char *block;
} TSResponse;

/**
 * Struct to hold all information passed by the HTTP request
 */
typedef struct {
    char *ts_device_id;
    char *ts_target_node;   //json pointer
    int ts_list_subnodes;   // wether to return values or available nodes/options
    char *ts_payload;
} TSUriElems;

/**
 * Get Handler for basic get requests
 *
 * \returns a pointer to a response object containing status code and data string
 */

TSResponse *ts_execute(const char *uri, char *content, int http_method);

char *ts_resp_data(char *resp);

int ts_resp_status(char *resp);

/**
 * Initializes the internal device list and scans
 * both serial and CAN Bus for devices
 */
void ts_scan_devices();

/**
 * Will return a list with the names and IDs of connected devices
 *
 * \returns A char pointer to a stringified json array
 *
 * The caller is responsible to call free() on the result after usage
 */
char *ts_get_device_list();

/**
 * Generate a ThingSet request header from HTTP URL and mode
 *
 * \returns Number of characters added to the buffer or 0 in case of error
 */
int ts_req_hdr_from_http(char *buf, size_t buf_size, int method, const char *uri);

void ts_parse_uri(const char *uri, TSUriElems *params);
char *ts_build_query(uint8_t ts_method, TSUriElems *params);

#ifdef __cplusplus
}
#endif
#endif /* TS_CLIENT_H_ */
