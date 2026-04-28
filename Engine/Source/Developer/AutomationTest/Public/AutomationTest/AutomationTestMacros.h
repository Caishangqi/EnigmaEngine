// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "AutomationTest/AutomationTestContext.h"

#if ENIGMA_WITH_AUTOMATION_TESTS
    #include <gtest/gtest.h>
#endif

#define ENIGMA_AUTOMATION_TEST_STRINGIZE_IMPL(Token) #Token
#define ENIGMA_AUTOMATION_TEST_STRINGIZE(Token) ENIGMA_AUTOMATION_TEST_STRINGIZE_IMPL(Token)

#define ENIGMA_AUTOMATION_TEST_REGISTER(TestClass, PrettyName, ModuleToken, TestType, TestFlags) \
    namespace                                                                                    \
    {                                                                                            \
        struct TestClass##AutomationRegistration                                                 \
        {                                                                                        \
            TestClass##AutomationRegistration()                                                  \
            {                                                                                    \
                ::Enigma::FAutomationTestDescriptor Descriptor;                                  \
                Descriptor.Name = PrettyName;                                                    \
                Descriptor.ModuleName = ENIGMA_AUTOMATION_TEST_STRINGIZE(ModuleToken);           \
                Descriptor.Type = TestType;                                                      \
                Descriptor.Flags = TestFlags;                                                    \
                Descriptor.SourceFile = __FILE__;                                                \
                Descriptor.SourceLine = __LINE__;                                                \
                Descriptor.GoogleTestSuiteName =                                                 \
                    ENIGMA_AUTOMATION_TEST_STRINGIZE(ModuleToken);                               \
                Descriptor.GoogleTestName = ENIGMA_AUTOMATION_TEST_STRINGIZE(TestClass);         \
                ::Enigma::FAutomationTestRegistry::Get().RegisterTest(Descriptor);               \
            }                                                                                    \
        };                                                                                       \
        static TestClass##AutomationRegistration G##TestClass##AutomationRegistration;           \
    }

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
            if (!bRunResult)                                                            \
            {                                                                           \
                ADD_FAILURE() << "Automation test returned false";                       \
            }                                                                           \
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
    ENIGMA_AUTOMATION_TEST_REGISTER(TestClass, PrettyName, ModuleToken, TestType, TestFlags) \
                                                                                       \
    ENIGMA_AUTOMATION_TEST_GTEST_BODY(TestClass, ModuleToken, PrettyName)

/// Declare and register an automation test whose body returns void.
#define ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                             \
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
                                                                                       \
    private:                                                                           \
        void RunTestBody();                                                            \
    };                                                                                 \
                                                                                       \
    ENIGMA_AUTOMATION_TEST_REGISTER(TestClass, PrettyName, ModuleToken, TestType, TestFlags) \
                                                                                       \
    ENIGMA_AUTOMATION_TEST_GTEST_BODY(TestClass, ModuleToken, PrettyName)              \
                                                                                       \
    bool TestClass::RunTest(const ::Enigma::FAutomationTestContext& Context)           \
    {                                                                                  \
        RunTestBody();                                                                 \
        return !Context.HasAnyFailures();                                              \
    }                                                                                  \
                                                                                       \
    void TestClass::RunTestBody()

/// Declare and register an automation test with a fixture class.
#define ENIGMA_IMPLEMENT_AUTOMATION_TEST_F(                                           \
    FixtureClass, TestClass, PrettyName, ModuleToken, TestType, TestFlags)             \
    class TestClass final : public FixtureClass, public ::Enigma::FAutomationTestBase  \
    {                                                                                  \
    public:                                                                            \
        TestClass()                                                                    \
            : ::Enigma::FAutomationTestBase(PrettyName)                                \
        {                                                                              \
        }                                                                              \
                                                                                       \
        bool RunTest(const ::Enigma::FAutomationTestContext& Context) override;        \
                                                                                       \
    private:                                                                           \
        void RunTestBody();                                                            \
    };                                                                                 \
                                                                                       \
    ENIGMA_AUTOMATION_TEST_REGISTER(TestClass, PrettyName, ModuleToken, TestType, TestFlags) \
                                                                                       \
    ENIGMA_AUTOMATION_TEST_GTEST_BODY(TestClass, ModuleToken, PrettyName)              \
                                                                                       \
    bool TestClass::RunTest(const ::Enigma::FAutomationTestContext& Context)           \
    {                                                                                  \
        this->SetUp();                                                                 \
        if (!Context.HasAnyFailures())                                                 \
        {                                                                              \
            RunTestBody();                                                             \
        }                                                                              \
        this->TearDown();                                                              \
        return !Context.HasAnyFailures();                                              \
    }                                                                                  \
                                                                                       \
    void TestClass::RunTestBody()
