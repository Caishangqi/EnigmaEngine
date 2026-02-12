// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

using System.Text.RegularExpressions;

/// <summary>
/// Validates names for modules, plugins, and projects against C++ naming rules,
/// reserved keywords, and existing name conflicts.
/// </summary>
public static partial class NameValidator
{
    /// <summary>Valid C++ identifier pattern: starts with letter or underscore, followed by alphanumerics or underscores.</summary>
    [GeneratedRegex(@"^[A-Za-z_][A-Za-z0-9_]*$", RegexOptions.Compiled)]
    private static partial Regex CppIdentifierRegex();

    /// <summary>C++ reserved keywords that cannot be used as names.</summary>
    private static readonly HashSet<string> CppReservedKeywords = new(StringComparer.OrdinalIgnoreCase)
    {
        "alignas", "alignof", "and", "and_eq", "asm", "auto",
        "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl",
        "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue",
        "co_await", "co_return", "co_yield",
        "decltype", "default", "delete", "do", "double", "dynamic_cast",
        "else", "enum", "explicit", "export", "extern",
        "false", "float", "for", "friend",
        "goto",
        "if", "inline", "int",
        "long",
        "mutable",
        "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
        "operator", "or", "or_eq",
        "private", "protected", "public",
        "register", "reinterpret_cast", "requires", "return",
        "short", "signed", "sizeof", "static", "static_assert", "static_cast",
        "struct", "switch",
        "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using",
        "virtual", "void", "volatile",
        "wchar_t", "while",
        "xor", "xor_eq",
    };

    /// <summary>
    /// Validate a name against C++ naming rules, reserved keywords, and existing name conflicts.
    /// </summary>
    /// <param name="name">The name to validate.</param>
    /// <param name="context">Contextual data for conflict checking.</param>
    /// <returns>A <see cref="ValidationResult"/> indicating success or failure with a message.</returns>
    public static ValidationResult Validate(string name, ValidationContext context)
    {
        // Rule 1: Non-empty / non-whitespace
        if (string.IsNullOrWhiteSpace(name))
        {
            return ValidationResult.Fail("Name must not be empty or whitespace.");
        }

        // Rule 2: Valid C++ identifier
        if (!CppIdentifierRegex().IsMatch(name))
        {
            return ValidationResult.Fail(
                $"'{name}' is not a valid C++ identifier. " +
                "Must start with a letter or underscore and contain only letters, digits, or underscores.");
        }

        // Rule 3: Not a C++ reserved keyword
        if (CppReservedKeywords.Contains(name))
        {
            return ValidationResult.Fail($"'{name}' is a C++ reserved keyword and cannot be used as a name.");
        }

        // Rule 4: Not an existing engine module name
        if (context.EngineModuleNames.Contains(name))
        {
            return ValidationResult.Fail($"'{name}' conflicts with an existing engine module name.");
        }

        // Rule 5: Not an existing project-level name
        if (context.ExistingNames.Contains(name))
        {
            return ValidationResult.Fail($"'{name}' already exists in the current project.");
        }

        return ValidationResult.Ok();
    }
}
