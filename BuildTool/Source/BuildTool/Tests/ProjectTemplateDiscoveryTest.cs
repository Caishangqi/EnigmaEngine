// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json;
using BuildTool.Scaffolding;

/// <summary>
/// Unit tests for <see cref="ProjectTemplateDiscovery"/>.
/// </summary>
public static class ProjectTemplateDiscoveryTest
{
    public static void Run()
    {
        Console.WriteLine("=== ProjectTemplateDiscovery Tests ===");
        Console.WriteLine();

        TestDiscoverFindsDefaultTemplate();
        TestDiscoverFindsVariantTemplates();
        TestDiscoverParsesTemplateJson();
        TestDiscoverMissingTemplateJsonUsesDefaults();
        TestFindExistingTemplateReturnsInfo();
        TestFindNonExistentReturnsNull();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestDiscoverFindsDefaultTemplate()
    {
        Console.WriteLine("[Test 1] Discover finds Default template");

        var root = CreateFakeEngineRoot("Default");
        try
        {
            var templates = ProjectTemplateDiscovery.Discover(root);
            Assert(templates.Count >= 1, $"Expected at least 1 template, got {templates.Count}");
            Assert(templates[0].Name == "Default", $"First template should be Default, got '{templates[0].Name}'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestDiscoverFindsVariantTemplates()
    {
        Console.WriteLine("[Test 2] Discover finds variant templates alongside Default");

        var root = CreateFakeEngineRoot("Default", "BlankDX12", "BlankAscii");
        try
        {
            var templates = ProjectTemplateDiscovery.Discover(root);
            Assert(templates.Count == 3, $"Expected 3 templates, got {templates.Count}");
            Assert(templates[0].Name == "Default", "Default should be first");

            var names = templates.Select(t => t.Name).ToList();
            Assert(names.Contains("BlankDX12"), "Should contain BlankDX12");
            Assert(names.Contains("BlankAscii"), "Should contain BlankAscii");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestDiscoverParsesTemplateJson()
    {
        Console.WriteLine("[Test 3] Discover parses template.json metadata");

        var root = CreateFakeEngineRoot("Default");
        var templateDir = Path.Combine(root, "Engine", "Templates", "Project", "Default");
        var metadata = new { DisplayName = "My Default", Description = "A test template", Tags = new[] { "game", "test" } };
        File.WriteAllText(
            Path.Combine(templateDir, "template.json"),
            JsonSerializer.Serialize(metadata));
        try
        {
            var templates = ProjectTemplateDiscovery.Discover(root);
            Assert(templates.Count == 1, $"Expected 1 template, got {templates.Count}");
            Assert(templates[0].DisplayName == "My Default", $"DisplayName should be 'My Default', got '{templates[0].DisplayName}'");
            Assert(templates[0].Description == "A test template", $"Description mismatch");
            Assert(templates[0].Tags.Count == 2, $"Expected 2 tags, got {templates[0].Tags.Count}");
            Assert(templates[0].Tags[0] == "game", $"First tag should be 'game'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestDiscoverMissingTemplateJsonUsesDefaults()
    {
        Console.WriteLine("[Test 4] Discover uses directory name when template.json is missing");

        var root = CreateFakeEngineRoot("MyVariant");
        try
        {
            var templates = ProjectTemplateDiscovery.Discover(root);
            Assert(templates.Count == 1, $"Expected 1 template, got {templates.Count}");
            Assert(templates[0].Name == "MyVariant", $"Name should be 'MyVariant', got '{templates[0].Name}'");
            Assert(templates[0].DisplayName == "MyVariant", $"DisplayName should default to 'MyVariant', got '{templates[0].DisplayName}'");
            Assert(string.IsNullOrEmpty(templates[0].Description), "Description should be empty");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestFindExistingTemplateReturnsInfo()
    {
        Console.WriteLine("[Test 5] Find returns info for existing template");

        var root = CreateFakeEngineRoot("Default", "BlankAscii");
        try
        {
            var info = ProjectTemplateDiscovery.Find(root, "BlankAscii");
            Assert(info is not null, "Should find BlankAscii");
            Assert(info!.Name == "BlankAscii", $"Name should be 'BlankAscii', got '{info.Name}'");
            Assert(Directory.Exists(info.DirectoryPath), "DirectoryPath should exist");

            // Case-insensitive
            var infoLower = ProjectTemplateDiscovery.Find(root, "blankascii");
            Assert(infoLower is not null, "Should find blankascii (case-insensitive)");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestFindNonExistentReturnsNull()
    {
        Console.WriteLine("[Test 6] Find returns null for non-existent template");

        var root = CreateFakeEngineRoot("Default");
        try
        {
            var info = ProjectTemplateDiscovery.Find(root, "NonExistent");
            Assert(info is null, "Should return null for non-existent template");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    // --- Helpers ---

    /// <summary>
    /// Creates a fake engine root with the given template subdirectories
    /// under Engine/Templates/Project/.
    /// </summary>
    private static string CreateFakeEngineRoot(params string[] templateNames)
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_discovery_test_{Guid.NewGuid():N}");
        foreach (var name in templateNames)
        {
            Directory.CreateDirectory(Path.Combine(root, "Engine", "Templates", "Project", name));
        }
        return root;
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
