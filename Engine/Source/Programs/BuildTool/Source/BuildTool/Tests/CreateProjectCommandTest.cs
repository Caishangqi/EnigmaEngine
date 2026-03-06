// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json.Nodes;
using BuildTool.Commands;
using BuildTool.Models;

/// <summary>
/// Unit tests for <see cref="CreateProjectCommand"/>.
/// </summary>
public static class CreateProjectCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== CreateProjectCommand Tests ===");
        Console.WriteLine();

        TestCreatesFullProjectStructure();
        TestEprojectContent();
        TestTargetCsContent();
        TestGenerateProjectFilesBat();
        TestGameInstanceFiles();
        TestRejectsExistingDirectory();
        TestRejectsInvalidName();
        TestEngineAssociationRelativePath();
        TestWithoutTemplateUsesDefault();
        TestWithDX12TemplateUsesCorrectTemplate();
        TestWithAsciiTemplateUsesCorrectTemplate();
        TestWithInvalidTemplateReturnsError();
        TestWithListTemplatesPrintsDiscoveredTemplates();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestCreatesFullProjectStructure()
    {
        Console.WriteLine("[Test 1] Creates all project files and directories");

        var location = CreateTempLocation();
        try
        {
            var result = Execute("MyGame", location);
            Assert(result.Success, $"Expected success: {result.Message}");

            var proj = Path.Combine(location, "MyGame");
            Assert(File.Exists(Path.Combine(proj, "MyGame.eproject")), ".eproject missing");
            Assert(File.Exists(Path.Combine(proj, "GenerateProjectFiles.bat")), ".bat missing");
            Assert(File.Exists(Path.Combine(proj, "Source", "MyGame.Target.cs")), ".Target.cs missing");
            Assert(File.Exists(Path.Combine(proj, "Source", "MyGame", "MyGame.Build.cs")), ".Build.cs missing");
            Assert(File.Exists(Path.Combine(proj, "Source", "MyGame", "Public", "MyGameGameInstance.h")), "GameInstance.h missing");
            Assert(File.Exists(Path.Combine(proj, "Source", "MyGame", "Private", "MyGameModule.cpp")), "Module.cpp missing");
            Assert(File.Exists(Path.Combine(proj, "Source", "MyGame", "Private", "MyGameGameInstance.cpp")), "GameInstance.cpp missing");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestEprojectContent()
    {
        Console.WriteLine("[Test 2] .eproject has valid JSON with correct fields");

        var location = CreateTempLocation();
        try
        {
            Execute("TestProj", location);

            var path = Path.Combine(location, "TestProj", "TestProj.eproject");
            var json = JsonNode.Parse(File.ReadAllText(path))!.AsObject();

            Assert(json["FileVersion"]!.GetValue<int>() == 1, "FileVersion should be 1");
            Assert(json["EngineAssociation"] is not null, "EngineAssociation should exist");

            var modules = json["Modules"]!.AsArray();
            Assert(modules.Count == 1, $"Expected 1 module, got {modules.Count}");
            Assert(modules[0]!["Name"]!.GetValue<string>() == "TestProj", "Module name mismatch");

            var plugins = json["Plugins"]!.AsArray();
            Assert(plugins.Count == 0, "Plugins should be empty");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestTargetCsContent()
    {
        Console.WriteLine("[Test 3] .Target.cs has correct target class");

        var location = CreateTempLocation();
        try
        {
            Execute("FooGame", location);

            var content = File.ReadAllText(
                Path.Combine(location, "FooGame", "Source", "FooGame.Target.cs"));
            Assert(content.Contains("class FooGameTarget : TargetRules"), "Target class declaration missing");
            Assert(content.Contains("Type = TargetType.Game"), "TargetType.Game missing");
            Assert(content.Contains("ExtraModuleNames.Add(\"FooGame\")"), "ExtraModuleNames missing");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestGenerateProjectFilesBat()
    {
        Console.WriteLine("[Test 4] GenerateProjectFiles.bat has correct content");

        var location = CreateTempLocation();
        try
        {
            Execute("BarGame", location);

            var content = File.ReadAllText(
                Path.Combine(location, "BarGame", "GenerateProjectFiles.bat"));
            Assert(content.Contains("BarGame.eproject"), "Should reference project .eproject");
            Assert(content.Contains("generate-project-files"), "Should invoke generate-project-files");
            Assert(content.Contains("ENGINE_ROOT="), "Should set ENGINE_ROOT");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestGameInstanceFiles()
    {
        Console.WriteLine("[Test 5] GameInstance header and source have correct class name");

        var location = CreateTempLocation();
        try
        {
            Execute("ArcGame", location);

            var header = File.ReadAllText(
                Path.Combine(location, "ArcGame", "Source", "ArcGame", "Public", "ArcGameGameInstance.h"));
            Assert(header.Contains("class") && header.Contains("FArcGameGameInstance"),
                "Header should declare FArcGameGameInstance");
            Assert(header.Contains("Enigma::FGameInstance") || header.Contains("FGameInstance"),
                "Should inherit from FGameInstance");

            var source = File.ReadAllText(
                Path.Combine(location, "ArcGame", "Source", "ArcGame", "Private", "ArcGameGameInstance.cpp"));
            Assert(source.Contains("FArcGameGameInstance"),
                "Source should reference FArcGameGameInstance");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestRejectsExistingDirectory()
    {
        Console.WriteLine("[Test 6] Rejects existing directory");

        var location = CreateTempLocation();
        try
        {
            // Pre-create the project directory
            Directory.CreateDirectory(Path.Combine(location, "Existing"));

            var result = Execute("Existing", location);
            Assert(!result.Success, "Should fail for existing directory");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestRejectsInvalidName()
    {
        Console.WriteLine("[Test 7] Rejects invalid name '2Bad'");

        var location = CreateTempLocation();
        try
        {
            var result = Execute("2Bad", location);
            Assert(!result.Success, "Should fail for name starting with digit");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestEngineAssociationRelativePath()
    {
        Console.WriteLine("[Test 8] EngineAssociation contains relative path to engine root");

        var location = CreateTempLocation();
        try
        {
            Execute("RelTest", location);

            var path = Path.Combine(location, "RelTest", "RelTest.eproject");
            var json = JsonNode.Parse(File.ReadAllText(path))!.AsObject();
            var assoc = json["EngineAssociation"]!.GetValue<string>();

            // Should be a relative path using forward slashes, ending with /
            Assert(assoc.Contains("/"), "Should use forward slashes");
            Assert(assoc.EndsWith("/"), "Should end with /");
            Assert(!assoc.Contains("\\"), "Should not contain backslashes");

            // Verify the path actually resolves to the engine root
            var projectDir = Path.Combine(location, "RelTest");
            var resolved = Path.GetFullPath(Path.Combine(projectDir, assoc)).TrimEnd('\\', '/');
            var engineRoot = Path.GetFullPath(CreateProjectCommand.FindEngineRoot()).TrimEnd('\\', '/');
            Assert(string.Equals(resolved, engineRoot, StringComparison.OrdinalIgnoreCase),
                $"Resolved '{resolved}' should match engine root '{engineRoot}'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestWithoutTemplateUsesDefault()
    {
        Console.WriteLine("[Test 9] Without --template uses Default template");

        var location = CreateTempLocation();
        try
        {
            var result = Execute("DefProj", location);
            Assert(result.Success, $"Expected success: {result.Message}");

            // Default template does not include ApplicationCore in .Build.cs
            var buildCs = File.ReadAllText(
                Path.Combine(location, "DefProj", "Source", "DefProj", "DefProj.Build.cs"));
            Assert(!buildCs.Contains("ApplicationCore"), "Default template should not reference ApplicationCore");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestWithDX12TemplateUsesCorrectTemplate()
    {
        Console.WriteLine("[Test 10] --template BlankDX12 uses DX12 template");

        var location = CreateTempLocation();
        try
        {
            var result = ExecuteWithTemplate("DX12Proj", location, "BlankDX12");
            Assert(result.Success, $"Expected success: {result.Message}");

            var buildCs = File.ReadAllText(
                Path.Combine(location, "DX12Proj", "Source", "DX12Proj", "DX12Proj.Build.cs"));
            Assert(buildCs.Contains("ApplicationCore"), "BlankDX12 template should reference ApplicationCore");

            var header = File.ReadAllText(
                Path.Combine(location, "DX12Proj", "Source", "DX12Proj", "Public", "DX12ProjGameInstance.h"));
            Assert(header.Contains("FGameInstance"), "BlankDX12 GameInstance should inherit FGameInstance");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestWithAsciiTemplateUsesCorrectTemplate()
    {
        Console.WriteLine("[Test 11] --template BlankAscii uses ASCII template");

        var location = CreateTempLocation();
        try
        {
            var result = ExecuteWithTemplate("AsciiProj", location, "BlankAscii");
            Assert(result.Success, $"Expected success: {result.Message}");

            var source = File.ReadAllText(
                Path.Combine(location, "AsciiProj", "Source", "AsciiProj", "Private", "AsciiProjGameInstance.cpp"));
            Assert(source.Contains("IAsciiRendererModule"), "BlankAscii GameInstance should use IAsciiRendererModule");

            var eproject = File.ReadAllText(
                Path.Combine(location, "AsciiProj", "AsciiProj.eproject"));
            Assert(eproject.Contains("BlankAscii"), ".eproject should contain BlankAscii template reference");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestWithInvalidTemplateReturnsError()
    {
        Console.WriteLine("[Test 12] --template NonExistent returns error with available list");

        var location = CreateTempLocation();
        try
        {
            var result = ExecuteWithTemplate("BadProj", location, "NonExistent");
            Assert(!result.Success, "Should fail for non-existent template");
            Assert(result.Message.Contains("NonExistent"), "Error should mention the requested template");
            Assert(result.Message.Contains("Default"), "Error should list Default as available");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    private static void TestWithListTemplatesPrintsDiscoveredTemplates()
    {
        Console.WriteLine("[Test 13] --list-templates returns success");

        var options = new BuildOptions
        {
            ProjectPath = ".",
            ExtraArguments = new Dictionary<string, string>
            {
                ["list-templates"] = "",
            },
        };
        var result = new CreateProjectCommand().Execute(options);
        Assert(result.Success, $"--list-templates should succeed: {result.Message}");

        Console.WriteLine("  PASSED");
    }

    // --- Helpers ---

    private static BuildResult Execute(string projectName, string location)
    {
        var options = new BuildOptions
        {
            ProjectPath = ".",
            ExtraArguments = new Dictionary<string, string>
            {
                ["name"] = projectName,
                ["location"] = location,
            },
        };
        return new CreateProjectCommand().Execute(options);
    }

    private static BuildResult ExecuteWithTemplate(string projectName, string location, string template)
    {
        var options = new BuildOptions
        {
            ProjectPath = ".",
            ExtraArguments = new Dictionary<string, string>
            {
                ["name"] = projectName,
                ["location"] = location,
                ["template"] = template,
            },
        };
        return new CreateProjectCommand().Execute(options);
    }

    private static string CreateTempLocation()
    {
        var path = Path.Combine(Path.GetTempPath(), $"enigma_proj_test_{Guid.NewGuid():N}");
        Directory.CreateDirectory(path);
        return path;
    }

    private static void CleanupDir(string path)
    {
        try
        {
            if (Directory.Exists(path))
                Directory.Delete(path, recursive: true);
        }
        catch { /* Best effort */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
