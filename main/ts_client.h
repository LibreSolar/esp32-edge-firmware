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
#include "cJSON.h"

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
 * Struct to hold all information passed by the HTTP request
 */
typedef struct {
    char *ts_device_id;
    char *ts_target_node;   //json pointer
    int ts_list_subnodes;   // wether to return values or available nodes/options
    char *ts_payload;
} TSUriElems;

/**
* Struct to hold ts response values for the http handlers
*/
typedef struct {
    uint8_t ts_status_code;
    char *data;
    char *block;
    uint32_t block_len;
} TSResponse;

/**
* Struct to hold device information
* when a new device is connected, the function pointer
* should be set accordingly to either the serial or CAN send function
*/
typedef struct {
    uint8_t can_address;
    char *ts_device_id;
    char *ts_name;
    //function pointer to send requests to device, abstracting underlying connection
    char *(*send)(void *req, uint32_t query_size, uint8_t can_address, uint32_t *block_len);
    void *(*build_query)(uint8_t ts_method, TSUriElems *params, uint32_t* query_size);
    char *(*ts_resp_data)(TSResponse *res);
    uint8_t (*ts_resp_status)(TSResponse *res);
} TSDevice;

/**
 * Get Handler for basic get requests
 *
 * \returns a pointer to a response object containing status code and data string
 */
TSResponse *ts_execute(const char *uri, char *content, int http_method);

/**
 * Parses the response for the beginning of the payload. Does not work on binary data!
 * \returns A pointer to the first character of the payload
 */
char *ts_serial_resp_data(TSResponse *res);

/**
 * Parses the response for status code. Does not work on binary data!
 * \returns The ThingSet status
 */
uint8_t ts_serial_resp_status(TSResponse *res);

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
 * Check if a CAN device is already known
 *
 * \returns pointer to TSDevice or NULL in case the device is not found
 */
TSDevice *ts_get_can_device(uint8_t can_addr);

/**
 * Generate a ThingSet request header from HTTP URL and mode
 *
 * \returns Number of characters added to the buffer or 0 in case of error
 */
int ts_req_hdr_from_http(char *buf, size_t buf_size, int method, const char *uri);

/**
 * Parse a given URI into the elems struct. Necessary to map the HTTP endpoint to the
 * Thingset Serial/CAN implementation
 */
void ts_parse_uri(const char *uri, TSUriElems *params);

/**
 * Builds the ThingSet query in string format.
 * \returns String with the query
 * Caller is responsible to free() string
 */
void *ts_build_query_serial(uint8_t ts_method, TSUriElems *params, uint32_t *query_size);

/**
 * Takes device Information as a json and fills it in a TSDevice struct. Allocates the necessary memory.
 * \returns a nonzero value in case of failure
 */
int ts_parse_device_info(cJSON *json, TSDevice *device);

/**
 * Frees all fields in the TSDevice struct and the struct itself.
 */
void ts_remove_device(TSDevice *device);

/**
 * Wrapper for strlen where NULL is interpreted as a string with length of zero
 */
int strlen_null(char *r);

#ifdef __cplusplus
}
#endif

#endif /* TS_CLIENT_H_ */
