// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "AutomationTest/AutomationTestContext.h"

#if ENIGMA_WITH_AUTOMATION_TESTS
    #include <gtest/gtest.h>
#endif

#define ENIGMA_AUTOMATION_TEST_STRINGIZE_IMPL(Token) #Token
#define ENIGMA_AUTOMATION_TEST_STRINGIZE(Token) ENIGMA_AUTOMATION_TEST_STRINGIZE_IMPL(Token)

#if ENIGMA_WITH_AUTOMATION_TESTS
    #define ENIGMA_AUTOMATION_TEST_GTEST_BODY(TestClass, ModuleToken, PrettyName)       \
        TEST(ModuleToken, TestClass)                                                    \
        {                                                                               \
            ::Enigma::FAutomationTestContext Context(                                   \
                PrettyName,                                                             \
                ENIGMA_AUTOMATION_TEST_STRINGIZE(ModuleToken));                         \
            TestClass TestInstance;                                                     \
            TestInstance.SetContext(&Context);                                          \
            const bool bRunResult = TestInstance.RunTest(Context);                      \
            TestInstance.SetContext(nullptr);                                           \
            for (const auto& Failure : Context.GetFailures())                           \
            {                                                                           \
                ADD_FAILURE_AT(Failure.File.c_str(), Failure.Line) << Failure.Message;   \
            }                                                                           \
            EXPECT_TRUE(bRunResult);                                                    \
        }
#else
    #define ENIGMA_AUTOMATION_TEST_GTEST_BODY(TestClass, ModuleToken, PrettyName)
#endif

/// Declare and register a simple UE-style automation test.
#define ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(                                       \
    TestClass, PrettyName, ModuleToken, TestType, TestFlags)                           \
    class TestClass final : public ::Enigma::FAutomationTestBase                       \
    {                                                                                  \
    public:                                                                            \
        TestClass()                                                                    \
            : ::Enigma::FAutomationTestBase(PrettyName)                                \
        {                                                                              \
        }                                                                              \
                                                                                       \
        bool RunTest(const ::Enigma::FAutomationTestContext& Context) override;        \
    };                                                                                 \
                                                                                       \
    namespace                                                                          \
    {                                                                                  \
        struct TestClass##AutomationRegistration                                       \
        {                                                                              \
            TestClass##AutomationRegistration()                                        \
            {                                                                          \
                ::Enigma::FAutomationTestDescriptor Descriptor;                        \
                Descriptor.Name = PrettyName;                                          \
                Descriptor.ModuleName = ENIGMA_AUTOMATION_TEST_STRINGIZE(ModuleToken); \
                Descriptor.Type = TestType;                                            \
                Descriptor.Flags = TestFlags;                                          \
                Descriptor.SourceFile = __FILE__;                                      \
                Descriptor.SourceLine = __LINE__;                                      \
                Descriptor.GoogleTestSuiteName =                                       \
                    ENIGMA_AUTOMATION_TEST_STRINGIZE(ModuleToken);                     \
                Descriptor.GoogleTestName = ENIGMA_AUTOMATION_TEST_STRINGIZE(TestClass);\
                ::Enigma::FAutomationTestRegistry::Get().RegisterTest(Descriptor);     \
            }                                                                          \
        };                                                                             \
        static TestClass##AutomationRegistration G##TestClass##AutomationRegistration; \
    }                                                                                  \
                                                                                       \
    ENIGMA_AUTOMATION_TEST_GTEST_BODY(TestClass, ModuleToken, PrettyName)
