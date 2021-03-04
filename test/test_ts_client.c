/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tests.h"
#include <ts_client.h>
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
    params.ts_payload = NULL;
    params.ts_target_node = "output";
    params.ts_list_subnodes = 1;
    query = ts_build_query(TS_GET, &params);
    TEST_ASSERT_EQUAL_STRING("?output\n", query);
    free(query);
}

void ts_build_query_get_subnodes(void)
{
    TSUriElems params;
    char *query;
    params.ts_payload = NULL;
    params.ts_target_node = "output/";
    params.ts_list_subnodes = 0;
    query = ts_build_query(TS_GET, &params);
    TEST_ASSERT_EQUAL_STRING("?output/\n", query);
    free(query);
}

void ts_build_query_get_root(void)
{
    TSUriElems params;
    char *query;
    params.ts_payload = NULL;
    params.ts_target_node = "";
    params.ts_list_subnodes = 0;
    query = ts_build_query(TS_GET, &params);
    TEST_ASSERT_EQUAL_STRING("?/\n", query);
    free(query);
}

void ts_build_query_exec(void)
{
    TSUriElems params;
    char *query;
    params.ts_payload = NULL;
    params.ts_target_node = "auth";
    params.ts_list_subnodes = 1;
    query = ts_build_query(TS_POST, &params);
    TEST_ASSERT_EQUAL_STRING("!auth\n", query);
    free(query);
}

void ts_build_query_post(void)
{
    TSUriElems params;
    char *query;
    params.ts_payload = "\"Bat_V\"";
    params.ts_target_node = "pub/serial/IDs";
    params.ts_list_subnodes = 1;
    query = ts_build_query(TS_POST, &params);
    TEST_ASSERT_EQUAL_STRING("+pub/serial/IDs \"Bat_V\"\n", query);
    free(query);
}

void ts_build_query_null(void)
{
    TSUriElems *params = NULL;
    char *query;
    query = ts_build_query(TS_GET, params);
    TEST_ASSERT_EQUAL_STRING(NULL, query);
}

void ts_build_query_false_method(void)
{
    TSUriElems params;
    char *query;
    params.ts_payload = NULL;
    params.ts_target_node = "output";
    params.ts_list_subnodes = 1;
    query = ts_build_query(0, &params);
    TEST_ASSERT_EQUAL_STRING("", query);
    free(query);
}

void ts_build_query_payload(void)
{
    TSUriElems params;
    char *query;
    params.ts_payload = "[\"Bat_V\", \"Bat_A\", \"Bat_W\", \"Bat_degC\", \"BatTempExt\"]";
    params.ts_target_node = "output";
    params.ts_list_subnodes = 1;
    query = ts_build_query(TS_GET, &params);
    TEST_ASSERT_EQUAL_STRING("?output [\"Bat_V\", \"Bat_A\", \"Bat_W\", \"Bat_degC\", \"BatTempExt\"]\n", query);
    free(query);
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
    UNITY_END();
}
