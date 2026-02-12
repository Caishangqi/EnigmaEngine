using BuildTool.Scaffolding;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for <see cref="NameValidator"/>.
/// </summary>
public static class NameValidatorTest
{
    public static void Run()
    {
        Console.WriteLine("=== NameValidator Tests ===");
        Console.WriteLine();

        TestValidNames();
        TestInvalidStartsWithDigit();
        TestInvalidSpecialCharacters();
        TestEmptyName();
        TestCppReservedKeywords();
        TestEngineModuleConflict();
        TestExistingModuleConflict();
        TestCaseSensitivity();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static ValidationContext EmptyContext() =>
        ValidationContext.Create([], []);

    /// <summary>Valid C++ identifiers should all pass.</summary>
    private static void TestValidNames()
    {
        Console.WriteLine("[Test 1] Valid names: MyModule, Core2, _Private, A");

        var ctx = EmptyContext();
        foreach (var name in new[] { "MyModule", "Core2", "_Private", "A" })
        {
            var result = NameValidator.Validate(name, ctx);
            Assert(result.Success, $"'{name}' should be valid but got: {result.Message}");
        }

        Console.WriteLine("  PASSED");
    }

    /// <summary>Names starting with a digit are invalid C++ identifiers.</summary>
    private static void TestInvalidStartsWithDigit()
    {
        Console.WriteLine("[Test 2] Invalid: starts with digit (2Fast)");

        var result = NameValidator.Validate("2Fast", EmptyContext());
        Assert(!result.Success, "Name starting with digit should fail");
        Assert(result.Message.Contains("identifier", StringComparison.OrdinalIgnoreCase),
            $"Error should mention 'identifier', got: {result.Message}");

        Console.WriteLine("  PASSED");
    }

    /// <summary>Names with special characters are invalid.</summary>
    private static void TestInvalidSpecialCharacters()
    {
        Console.WriteLine("[Test 3] Invalid: special characters (-, ., space)");

        foreach (var name in new[] { "My-Module", "My.Module", "My Module" })
        {
            var result = NameValidator.Validate(name, EmptyContext());
            Assert(!result.Success, $"'{name}' should fail validation");
        }

        Console.WriteLine("  PASSED");
    }

    /// <summary>Empty and whitespace-only names should fail.</summary>
    private static void TestEmptyName()
    {
        Console.WriteLine("[Test 4] Invalid: empty and whitespace names");

        foreach (var name in new[] { "", " ", "  " })
        {
            var result = NameValidator.Validate(name, EmptyContext());
            Assert(!result.Success, $"Empty/whitespace name should fail");
            Assert(result.Message.Contains("empty", StringComparison.OrdinalIgnoreCase),
                $"Error should mention 'empty', got: {result.Message}");
        }

        Console.WriteLine("  PASSED");
    }

    /// <summary>C++ reserved keywords should be rejected.</summary>
    private static void TestCppReservedKeywords()
    {
        Console.WriteLine("[Test 5] Invalid: C++ reserved keywords");

        foreach (var keyword in new[] { "class", "struct", "namespace", "int", "void", "template" })
        {
            var result = NameValidator.Validate(keyword, EmptyContext());
            Assert(!result.Success, $"Reserved keyword '{keyword}' should fail");
            Assert(result.Message.Contains("reserved", StringComparison.OrdinalIgnoreCase),
                $"Error should mention 'reserved', got: {result.Message}");
        }

        Console.WriteLine("  PASSED");
    }

    /// <summary>Names conflicting with engine modules should be rejected.</summary>
    private static void TestEngineModuleConflict()
    {
        Console.WriteLine("[Test 6] Invalid: engine module name conflict");

        var ctx = ValidationContext.Create(["Core", "Engine", "Launch"], []);

        foreach (var name in new[] { "Core", "Engine", "Launch" })
        {
            var result = NameValidator.Validate(name, ctx);
            Assert(!result.Success, $"'{name}' should conflict with engine module");
            Assert(result.Message.Contains("engine module", StringComparison.OrdinalIgnoreCase),
                $"Error should mention 'engine module', got: {result.Message}");
        }

        // Non-conflicting name should pass
        var ok = NameValidator.Validate("MyGame", ctx);
        Assert(ok.Success, $"'MyGame' should pass but got: {ok.Message}");

        Console.WriteLine("  PASSED");
    }

    /// <summary>Names already in use in the project should be rejected.</summary>
    private static void TestExistingModuleConflict()
    {
        Console.WriteLine("[Test 7] Invalid: existing project name conflict");

        var ctx = ValidationContext.Create([], ["GameCore", "PlayerModule"]);

        var result = NameValidator.Validate("GameCore", ctx);
        Assert(!result.Success, "'GameCore' should conflict with existing name");
        Assert(result.Message.Contains("already exists", StringComparison.OrdinalIgnoreCase),
            $"Error should mention 'already exists', got: {result.Message}");

        Console.WriteLine("  PASSED");
    }

    /// <summary>Name matching should be case-insensitive.</summary>
    private static void TestCaseSensitivity()
    {
        Console.WriteLine("[Test 8] Case-insensitive matching");

        // Engine module conflict (case-insensitive)
        var ctx1 = ValidationContext.Create(["Core"], []);
        var result1 = NameValidator.Validate("core", ctx1);
        Assert(!result1.Success, "'core' should conflict with 'Core' (case-insensitive)");

        var result2 = NameValidator.Validate("CORE", ctx1);
        Assert(!result2.Success, "'CORE' should conflict with 'Core' (case-insensitive)");

        // Existing name conflict (case-insensitive)
        var ctx2 = ValidationContext.Create([], ["MyModule"]);
        var result3 = NameValidator.Validate("mymodule", ctx2);
        Assert(!result3.Success, "'mymodule' should conflict with 'MyModule' (case-insensitive)");

        Console.WriteLine("  PASSED");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
