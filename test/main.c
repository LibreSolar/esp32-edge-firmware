/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tests.h"

#ifdef __WIN32__

// The below lines are added to fix unit test compilation error in PIO.
// The Unit Test Framework uses these functions like a constructor/destructor.
// We can just define two empty functions with these names.
void setUp (void) {}
void tearDown (void) {}

#endif

int app_main()
{
    void ts_client_tests();

#ifdef CUSTOM_TESTS
    custom_tests();
#endif
    return 1;
}
