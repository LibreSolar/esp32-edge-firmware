/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tests.h"
#include <ts_client.h>


void parse_proper_uri(void)
{
    const char *uri = "/someID/info";
    TSUriElems params;
    params.ts_payload = "";
    ts_parse_uri(uri, &params);
    TEST_ASSERT_EQUAL_STRING(0, 1);
}

void ts_client_tests()
{
    UNITY_BEGIN();
    RUN_TEST(parse_proper_uri);
    UNITY_END();
}
