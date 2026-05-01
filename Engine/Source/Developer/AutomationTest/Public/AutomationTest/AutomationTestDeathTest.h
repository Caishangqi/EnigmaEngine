// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Misc/AssertionMacros.h"

#if ENIGMA_WITH_AUTOMATION_TESTS
    #include <gtest/gtest.h>
#endif

#if ENIGMA_WITH_AUTOMATION_TESTS && DO_CHECK
    /// Expect Statement to terminate through a fatal assertion and match stderr.
    #define ENIGMA_EXPECT_FATAL_ASSERT(Statement, ExpectedRegex) \
        EXPECT_DEATH(                                            \
            {                                                     \
                Statement;                                        \
            },                                                    \
            ExpectedRegex)

    /// Assert Statement terminates through a fatal assertion and match stderr.
    #define ENIGMA_ASSERT_FATAL_ASSERT(Statement, ExpectedRegex) \
        ASSERT_DEATH(                                            \
            {                                                    \
                Statement;                                       \
            },                                                   \
            ExpectedRegex)
#elif ENIGMA_WITH_AUTOMATION_TESTS
    #define ENIGMA_EXPECT_FATAL_ASSERT(Statement, ExpectedRegex) \
        GTEST_SKIP() << "Fatal assertion tests are disabled because DO_CHECK is 0"

    #define ENIGMA_ASSERT_FATAL_ASSERT(Statement, ExpectedRegex) \
        GTEST_SKIP() << "Fatal assertion tests are disabled because DO_CHECK is 0"
#else
    #define ENIGMA_EXPECT_FATAL_ASSERT(Statement, ExpectedRegex) \
        do                                                       \
        {                                                        \
            (void)sizeof(ExpectedRegex);                         \
        } while (0)

    #define ENIGMA_ASSERT_FATAL_ASSERT(Statement, ExpectedRegex) \
        do                                                       \
        {                                                        \
            (void)sizeof(ExpectedRegex);                         \
        } while (0)
#endif
