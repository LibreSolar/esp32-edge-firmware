/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TS_CBOR_H_
#define TS_CBOR_H_

#include "ts_client.h"

void *ts_build_query_bin(uint8_t ts_method, TSUriElems *params, uint32_t *query_length);

char *cbor2json(uint8_t *cbor, size_t len);

uint8_t ts_cbor_resp_status(TSResponse *resp);

char *ts_cbor_resp_data(TSResponse *res);

#endif // TS_CBOR_H__
