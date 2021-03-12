/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tests.h"
#include <ts_client.h>
#include <ts_cbor.h>
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>


void parse_uri_no_subnodes(void)
{
    const char *uri = "someID/info";
    TSUriElems params;
    params.ts_payload = "";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_INT(1, params.ts_list_subnodes);
    TEST_ASSERT_EQUAL_STRING("info", params.ts_target_node);
    TEST_ASSERT_EQUAL_STRING("someID", params.ts_device_id);
    TEST_ASSERT_EQUAL_STRING("", params.ts_payload);

}

void parse_uri_with_subnodes(void)
{
    const char *uri = "someID/info/";
    TSUriElems params;
    params.ts_payload = "";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_INT(0, params.ts_list_subnodes);
    TEST_ASSERT_EQUAL_STRING("info/", params.ts_target_node);
    TEST_ASSERT_EQUAL_STRING("someID", params.ts_device_id);
    TEST_ASSERT_EQUAL_STRING("", params.ts_payload);
}

void parse_uri_id_only(void)
{
    const char *uri = "someID";
    TSUriElems params;
    params.ts_payload = "";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_INT(1, params.ts_list_subnodes);
    TEST_ASSERT_EQUAL_STRING("", params.ts_target_node);
    TEST_ASSERT_EQUAL_STRING("someID", params.ts_device_id);
    TEST_ASSERT_EQUAL_STRING("", params.ts_payload);
}

void parse_uri_empty(void)
{
    const char *uri = "";
    TSUriElems params;
    params.ts_payload = "";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_INT(-1, params.ts_list_subnodes);
    TEST_ASSERT_EQUAL_STRING(NULL, params.ts_target_node);
    TEST_ASSERT_EQUAL_STRING(NULL, params.ts_device_id);
    TEST_ASSERT_EQUAL_STRING("", params.ts_payload);
}

void parse_uri_null(void)
{
    const char *uri = NULL;
    TSUriElems params;
    params.ts_payload = "";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_INT(-1, params.ts_list_subnodes);
    TEST_ASSERT_EQUAL_STRING(NULL, params.ts_target_node);
    TEST_ASSERT_EQUAL_STRING(NULL, params.ts_device_id);
    TEST_ASSERT_EQUAL_STRING("", params.ts_payload);
}

void parse_uri_with_payload(void)
{
    const char *uri = "someID/output";
    TSUriElems params;
    params.ts_payload = "{\"Bat_V\"}";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_INT(1, params.ts_list_subnodes);
    TEST_ASSERT_EQUAL_STRING("output", params.ts_target_node);
    TEST_ASSERT_EQUAL_STRING("someID", params.ts_device_id);
    TEST_ASSERT_EQUAL_STRING("{\"Bat_V\"}", params.ts_payload);
}

void ts_build_query_get(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = NULL;
    params.ts_target_node = "output";
    params.ts_list_subnodes = 1;
    query = ts_build_query_serial(TS_GET, &params, &length);
    TEST_ASSERT_EQUAL_STRING("?output\n", query);
    free(query);
}

void ts_build_query_get_subnodes(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = NULL;
    params.ts_target_node = "output/";
    params.ts_list_subnodes = 0;
    query = ts_build_query_serial(TS_GET, &params, &length);
    TEST_ASSERT_EQUAL_STRING("?output/\n", query);
    free(query);
}

void ts_build_query_get_root(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = NULL;
    params.ts_target_node = "";
    params.ts_list_subnodes = 0;
    query = ts_build_query_serial(TS_GET, &params, &length);
    TEST_ASSERT_EQUAL_STRING("?/\n", query);
    free(query);
}

void ts_build_query_exec(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = NULL;
    params.ts_target_node = "auth";
    params.ts_list_subnodes = 1;
    query = ts_build_query_serial(TS_POST, &params, &length);
    TEST_ASSERT_EQUAL_STRING("!auth\n", query);
    free(query);
}

void ts_build_query_post(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = "\"Bat_V\"";
    params.ts_target_node = "pub/serial/IDs";
    params.ts_list_subnodes = 1;
    query = ts_build_query_serial(TS_POST, &params, &length);
    TEST_ASSERT_EQUAL_STRING("+pub/serial/IDs \"Bat_V\"\n", query);
    free(query);
}

void ts_build_query_null(void)
{
    TSUriElems *params = NULL;
    char *query;
    uint32_t length;
    query = ts_build_query_serial(TS_GET, params, &length);
    TEST_ASSERT_EQUAL_STRING(NULL, query);
}

void ts_build_query_false_method(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = NULL;
    params.ts_target_node = "output";
    params.ts_list_subnodes = 1;
    query = ts_build_query_serial(0, &params, &length);
    TEST_ASSERT_EQUAL_STRING("", query);
    free(query);
}

void ts_build_query_payload(void)
{
    TSUriElems params;
    char *query;
    uint32_t length;
    params.ts_payload = "[\"Bat_V\", \"Bat_A\", \"Bat_W\", \"Bat_degC\", \"BatTempExt\"]";
    params.ts_target_node = "output";
    params.ts_list_subnodes = 1;
    query = ts_build_query_serial(TS_GET, &params, &length);
    TEST_ASSERT_EQUAL_STRING("?output [\"Bat_V\", \"Bat_A\", \"Bat_W\", \"Bat_degC\", \"BatTempExt\"]\n", query);
    free(query);
}

void ts_build_bin_query_post(void)
{
    TSUriElems params;
    uint8_t *query;
    uint32_t length;
    params.ts_payload = "[\"Bat_V\", \"Bat_A\", \"Bat_W\", \"Bat_degC\", \"BatTempExt\"]";
    params.ts_target_node = "pub/serial/IDs";
    params.ts_list_subnodes = 1;
    query = ts_build_query_bin(TS_POST, &params, &length);
    int offset = 1;

    TEST_ASSERT_EQUAL(TS_POST, query[0]);
    uint8_t expected_node[] = {0x6E, 0x70, 0x75, 0x62,
                            0x2F, 0x73, 0x65 , 0x72,
                            0x69, 0x61, 0x6C, 0x2F,
                            0x49, 0x44, 0x73};
    for (int i = 0; i < sizeof(expected_node); i++) {
        offset++;
        TEST_ASSERT_EQUAL(expected_node[i], query[i+1]);
    }
    uint8_t expected_payload[] = {0x85, 0x65, 0x42, 0x61, 0x74, 0x5F, 0x56, 0x65,
                                0x42, 0x61, 0x74, 0x5F, 0x41, 0x65, 0x42, 0x61,
                                0x74, 0x5F, 0x57, 0x68, 0x42, 0x61, 0x74, 0x5F,
                                0x64, 0x65, 0x67, 0x43, 0x6A, 0x42, 0x61, 0x74,
                                0x54, 0x65, 0x6D, 0x70, 0x45, 0x78, 0x74};

    for (int i = 0; i < sizeof(expected_payload); i++) {
        TEST_ASSERT_EQUAL(expected_payload[i], query[i+offset]);
    }
    TEST_ASSERT_EQUAL(offset + sizeof(expected_payload), length);

    free(query);
}

void ts_build_bin_query_with_object(void)
{
    TSUriElems params;
    uint8_t *query;
    uint32_t length;
    params.ts_payload = "{\"loadEn\": true}";
    params.ts_target_node = "config";
    params.ts_list_subnodes = 1;
    query = ts_build_query_bin(TS_POST, &params, &length);
    int offset = 1;
    /*
    for (int i = 0; i < strlen_null((char *) query); i++) {
        printf("%02X", query[i]);
    }
    printf("\n");
    */

    TEST_ASSERT_EQUAL(TS_POST, query[0]);

    uint8_t expected_node[] = {0x66, 0x63, 0x6F, 0x6E, 0x66, 0x69, 0x67};
    for (int i = 0; i < sizeof(expected_node); i++) {
        offset++;
        TEST_ASSERT_EQUAL(expected_node[i], query[i+1]);
    }
    uint8_t expected_payload[] = {0xA1, 0x66, 0x6C, 0x6F, 0x61, 0x64, 0x45, 0x6E, 0xF5};

    for (int i = 0; i < sizeof(expected_payload); i++) {
        TEST_ASSERT_EQUAL(expected_payload[i], query[i+offset]);
    }
    TEST_ASSERT_EQUAL(offset + sizeof(expected_payload), length);

    free(query);
}

void ts_get_json_from_valid_cbor(void)
{
    uint8_t node[] = {0x66, 0x63, 0x6F, 0x6E, 0x66, 0x69, 0x67};
    char * string = cbor2json(node, sizeof(node));
    TEST_ASSERT_EQUAL_STRING("\"config\"", string);
    free(string);

}


void ts_client_tests()
{
    UNITY_BEGIN();
    RUN_TEST(parse_uri_no_subnodes);
    RUN_TEST(parse_uri_with_subnodes);
    RUN_TEST(parse_uri_id_only);
    RUN_TEST(parse_uri_empty);
    RUN_TEST(parse_uri_null);
    RUN_TEST(parse_uri_with_payload);

    RUN_TEST(ts_build_query_get);
    RUN_TEST(ts_build_query_get_subnodes);
    RUN_TEST(ts_build_query_get_root);
    RUN_TEST(ts_build_query_exec);
    RUN_TEST(ts_build_query_post);
    RUN_TEST(ts_build_query_null);
    RUN_TEST(ts_build_query_payload);
    RUN_TEST(ts_build_query_false_method);

    RUN_TEST(ts_build_bin_query_post);
    RUN_TEST(ts_build_bin_query_with_object);
    RUN_TEST(ts_get_json_from_valid_cbor);
    UNITY_END();
}
