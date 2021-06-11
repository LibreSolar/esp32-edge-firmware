/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "ts_cbor.h"
#include "../lib/tinycbor/src/cbor.h"
#include "../lib/tinycbor/src/cborjson.h"
#include "esp_err.h"
#include <stdlib.h>
#include "cJSON.h"

#ifndef UNIT_TEST

#include "esp_log.h"

#endif

static const char *TAG = "ts_cbor";

uint32_t buffersize = 0;

int getItemCount(cJSON *json)
{
    int count = 0;
    for (cJSON *elem = json->child; elem; elem = elem->next) {
        count ++;
    }
    return count;
}

CborError json2cbor(cJSON *json, CborEncoder *encoder, uint8_t *ts_query) {
    CborEncoder sub_encoder; //for the items of the array/object
    CborError error = 0;
    cJSON *item;

    switch (json->type) {
        case cJSON_True:
        case cJSON_False:
            return cbor_encode_boolean(encoder, json->type == cJSON_True);

        case cJSON_String:
            return cbor_encode_text_stringz(encoder, json->valuestring);

        case cJSON_NULL:
            return cbor_encode_null(encoder);

        case cJSON_Number:
            if ((double)json->valueint == json->valuedouble) {
                return cbor_encode_int(encoder, json->valueint);
            }

encode_double:
        // the only exception that JSON is larger: floating point numbers
            sub_encoder = *encoder;   // save the state
            error = cbor_encode_double(encoder, json->valuedouble);
            if (error == CborErrorOutOfMemory) {
                buffersize += 128;
                uint8_t *newbuffer = realloc(ts_query, buffersize);
                if (newbuffer == NULL) {
                    return error;
                }
                *encoder = sub_encoder;   // restore state
                encoder->data.ptr = newbuffer + (sub_encoder.data.ptr - ts_query);
                encoder->end = newbuffer + buffersize;
                ts_query = newbuffer;
                goto encode_double;
            }
            return error;

        case cJSON_Array:
            // this will init the sub encoder and create an array in the base cbor stream
            error = cbor_encoder_create_array(encoder, &sub_encoder, getItemCount(json));
            if (error) {
                return error;
            }
            for (item = json->child; item != NULL; item = item->next) {
                // recursive call with the sub encoder
                error = json2cbor(item, &sub_encoder, ts_query);
                if (error) {
                    return error;
                }
            }
            return cbor_encoder_close_container(encoder, &sub_encoder);

        case cJSON_Object:
            error = cbor_encoder_create_map(encoder, &sub_encoder, getItemCount(json));
            if (error)
                return error;

            for (item = json->child ; item; item = item->next) {
                error = cbor_encode_text_stringz(&sub_encoder, item->string);
                if (error) {
                    return error;
                }
                error = json2cbor(item, &sub_encoder, ts_query);
                }
                if (error){
                    return error;
                }
            return cbor_encoder_close_container_checked(encoder, &sub_encoder);
        default: return error;
    }
    return error;
}

void *ts_build_query_bin(uint8_t ts_method, TSUriElems *params, uint32_t *query_length)
{
    if (params == NULL) {
        return NULL;
    }

    buffersize = 1;  // 1 byte method
    if (strlen_null(params->ts_target_node) == 0 && params->ts_list_subnodes == 0) {
        buffersize += 2;    // cbor-string '/'
    }
    buffersize += strlen_null(params->ts_target_node) + 1; // strlen + cbor string flag
    buffersize += strlen_null(params->ts_payload);      //assumption that cbor is always equal or smaller than json string

    uint8_t *ts_query = (uint8_t *) calloc(buffersize, sizeof(uint8_t));

    if (ts_query == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for ts_query");
        return NULL;
    }

    int pos = 0;
    switch (ts_method) {
        case TS_GET:
            ts_query[pos] = TS_GET;
            break;
        case TS_POST:
            ts_query[pos] = TS_POST;
            break;
        case TS_PATCH:
            ts_query[pos] = TS_PATCH;
            break;
        case TS_DELETE:
            ts_query[pos] = TS_DELETE;
            break;
        default:
            ts_query[pos] = 0;
            return ts_query;    // nothing else to do here
    }
    pos++;

    CborEncoder encoder;
    cbor_encoder_init(&encoder, ts_query + pos, buffersize - pos, 0);
    CborError err = cbor_encode_text_stringz(&encoder, params->ts_target_node);
    if (err) {
        free(ts_query);
        return NULL;
    }
    if (params->ts_payload != NULL) {
        cJSON *payload = cJSON_ParseWithOpts(params->ts_payload, NULL, true);
        if (payload == NULL){
            free(ts_query);
            return NULL;
        }
        //char *string = cJSON_Print(payload);
        //printf("%s\n", string);
        //free(string);
        err = json2cbor(payload, &encoder, ts_query);
        cJSON_free(payload);
    }
    *query_length = encoder.data.ptr - ts_query;
    return (void *) ts_query;
}

char *cbor2json(uint8_t *cbor, size_t len)
{
    CborParser parser;
    CborValue value;
    CborError error = cbor_parser_init(cbor, len, 0, &parser, &value);
    if (error) {
        return NULL;
    }
    char *json = NULL;
    size_t size;
    FILE *stream = open_memstream(&json, &size);
    error = cbor_value_to_json_advance(stream, &value, 0);
    fclose(stream);
    if (error) {
        return NULL;
    }
    return json;
}

// decode cbor and replace binary data with json
// free binary data from receive task and replace pointer
// with new allocated json, will be free'd in web_server.c
char *ts_cbor_resp_data(TSResponse *res)
{
    char *json = cbor2json((uint8_t *) res->block + 1, res->block_len);
    free(res->block);
    res->block = json;
    return(json);
}

uint8_t ts_cbor_resp_status(TSResponse *res)
{
    return res->block[0];
}
