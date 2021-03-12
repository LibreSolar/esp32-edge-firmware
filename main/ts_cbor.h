/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TS_CBOR_H_
#define TS_CBOR_H_

#include "ts_client.h"

uint8_t *ts_build_query_bin(uint8_t ts_method, TSUriElems *params, uint32_t *query_length);
char *cbor2json(uint8_t *cbor, size_t len);

#endif // TS_CBOR_H__