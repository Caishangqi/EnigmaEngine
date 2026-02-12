using BuildTool.Scaffolding;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for <see cref="TemplateEngine"/>.
/// </summary>
public static class TemplateEngineTest
{
    public static void Run()
    {
        Console.WriteLine("=== TemplateEngine Tests ===");
        Console.WriteLine();

        TestReplacesPlaceholdersInContent();
        TestReplacesPlaceholdersInFileName();
        TestReplacesPlaceholdersInDirectoryName();
        TestRemovesTemplateSuffix();
        TestPreservesNonPlaceholderContent();
        TestLongestPlaceholderFirst();
        TestMissingTemplateDirectory();
        TestEmptyTemplateDirectory();
        TestNestedDirectoryStructure();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestReplacesPlaceholdersInContent()
    {
        Console.WriteLine("[Test 1] Replaces placeholders in content");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            File.WriteAllText(Path.Combine(templateDir, "Module.Build.cs"),
                "public class MODULE_NAME { // MODULE_NAME module }");

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string> { ["MODULE_NAME"] = "MyModule" },
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            var content = File.ReadAllText(Path.Combine(outputDir, "Module.Build.cs"));
            Assert(content == "public class MyModule { // MyModule module }",
                $"Unexpected content: {content}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestReplacesPlaceholdersInFileName()
    {
        Console.WriteLine("[Test 2] Replaces placeholders in file name");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            File.WriteAllText(Path.Combine(templateDir, "MODULE_NAME.Build.cs.template"), "// build file");

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string> { ["MODULE_NAME"] = "MyModule" },
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            var outputFile = Path.Combine(outputDir, "MyModule.Build.cs");
            Assert(File.Exists(outputFile), $"Expected file at {outputFile}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestReplacesPlaceholdersInDirectoryName()
    {
        Console.WriteLine("[Test 3] Replaces placeholders in directory name");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            var subDir = Path.Combine(templateDir, "PLUGIN_NAME");
            Directory.CreateDirectory(subDir);
            File.WriteAllText(Path.Combine(subDir, "readme.txt"), "plugin readme");

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string> { ["PLUGIN_NAME"] = "MyPlugin" },
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            var outputFile = Path.Combine(outputDir, "MyPlugin", "readme.txt");
            Assert(File.Exists(outputFile), $"Expected file at {outputFile}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestRemovesTemplateSuffix()
    {
        Console.WriteLine("[Test 4] Removes .template suffix");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            File.WriteAllText(Path.Combine(templateDir, "Config.h.template"), "#pragma once");

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string>(),
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            Assert(File.Exists(Path.Combine(outputDir, "Config.h")), "Expected .template suffix removed");
            Assert(!File.Exists(Path.Combine(outputDir, "Config.h.template")), "Original .template name should not exist");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestPreservesNonPlaceholderContent()
    {
        Console.WriteLine("[Test 5] Preserves non-placeholder content");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            const string original = "#include <iostream>\nint main() { return 0; }\n";
            File.WriteAllText(Path.Combine(templateDir, "main.cpp"), original);

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string> { ["MODULE_NAME"] = "MyModule" },
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            var content = File.ReadAllText(Path.Combine(outputDir, "main.cpp"));
            Assert(content == original, $"Content should be unchanged, got: {content}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestLongestPlaceholderFirst()
    {
        Console.WriteLine("[Test 6] Longest placeholder replaced first (no partial matches)");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            File.WriteAllText(Path.Combine(templateDir, "header.h.template"),
                "// MODULE_NAME_UPPER guard\n// MODULE_NAME impl\n");

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string>
                {
                    ["MODULE_NAME"] = "MyModule",
                    ["MODULE_NAME_UPPER"] = "MYMODULE",
                },
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            var content = File.ReadAllText(Path.Combine(outputDir, "header.h"));
            Assert(content.Contains("MYMODULE guard"), $"MODULE_NAME_UPPER not replaced correctly: {content}");
            Assert(content.Contains("MyModule impl"), $"MODULE_NAME not replaced correctly: {content}");
            Assert(!content.Contains("MODULE_NAME"), $"Unreplaced placeholder found: {content}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestMissingTemplateDirectory()
    {
        Console.WriteLine("[Test 7] Missing template directory returns Fail");

        var result = new TemplateEngine().Process(new TemplateContext
        {
            TemplateDir = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString()),
            OutputDir = Path.GetTempPath(),
            Replacements = new Dictionary<string, string>(),
        });

        Assert(!result.Success, "Should fail for missing template directory");
        Assert(result.Error!.Contains("does not exist", StringComparison.OrdinalIgnoreCase),
            $"Error should mention 'does not exist', got: {result.Error}");

        Console.WriteLine("  PASSED");
    }

    private static void TestEmptyTemplateDirectory()
    {
        Console.WriteLine("[Test 8] Empty template directory returns Ok with 0 files");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string>(),
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            Assert(result.CreatedFiles.Count == 0, $"Expected 0 files, got {result.CreatedFiles.Count}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static void TestNestedDirectoryStructure()
    {
        Console.WriteLine("[Test 9] Nested directory structure processed correctly");

        var (templateDir, outputDir) = CreateTempDirs();
        try
        {
            var deepDir = Path.Combine(templateDir, "MODULE_NAME", "Source", "MODULE_NAME", "Private");
            Directory.CreateDirectory(deepDir);
            File.WriteAllText(Path.Combine(deepDir, "MODULE_NAME.cpp.template"),
                "#include \"MODULE_NAME.h\"\n");

            var topDir = Path.Combine(templateDir, "MODULE_NAME");
            File.WriteAllText(Path.Combine(topDir, "MODULE_NAME.Build.cs.template"),
                "// MODULE_NAME build\n");

            var result = new TemplateEngine().Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = outputDir,
                Replacements = new Dictionary<string, string> { ["MODULE_NAME"] = "GameCore" },
            });

            Assert(result.Success, $"Expected success but got: {result.Error}");
            Assert(result.CreatedFiles.Count == 2, $"Expected 2 files, got {result.CreatedFiles.Count}");

            var deepFile = Path.Combine(outputDir, "GameCore", "Source", "GameCore", "Private", "GameCore.cpp");
            Assert(File.Exists(deepFile), $"Expected nested file at {deepFile}");
            var deepContent = File.ReadAllText(deepFile);
            Assert(deepContent == "#include \"GameCore.h\"\n",
                $"Nested content not replaced correctly: {deepContent}");

            var topFile = Path.Combine(outputDir, "GameCore", "GameCore.Build.cs");
            Assert(File.Exists(topFile), $"Expected top-level file at {topFile}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(templateDir, outputDir); }
    }

    private static (string templateDir, string outputDir) CreateTempDirs()
    {
        var root = Path.Combine(Path.GetTempPath(), "EnigmaTest_" + Guid.NewGuid().ToString("N")[..8]);
        var templateDir = Path.Combine(root, "templates");
        var outputDir = Path.Combine(root, "output");
        Directory.CreateDirectory(templateDir);
        Directory.CreateDirectory(outputDir);
        return (templateDir, outputDir);
    }

    private static void Cleanup(string templateDir, string outputDir)
    {
        var root = Path.GetDirectoryName(templateDir)!;
        try { if (Directory.Exists(root)) Directory.Delete(root, recursive: true); }
        catch { /* Best effort cleanup */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}