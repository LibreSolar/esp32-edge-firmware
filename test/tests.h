/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unity.h>
#include <time.h>


void ts_client_tests();

// activate this via build_flags in platformio.ini or custom.ini
#ifdef CUSTOM_TESTS
void custom_tests();
#endif
