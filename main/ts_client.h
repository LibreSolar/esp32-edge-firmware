/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Martin Jäger / Libre Solar
 */

#ifndef TS_CLIENT_H_
#define TS_CLIENT_H_

#include <string.h>

char *ts_resp_data(char *resp);

int ts_resp_status(char *resp);

/**
 * Generate a ThingSet request header from HTTP URL and mode
 *
 * \returns Number of characters added to the buffer or 0 in case of error
 */
int ts_req_hdr_from_http(char *buf, size_t buf_size, int method, const char *uri);

#endif /* TS_CLIENT_H_ */
