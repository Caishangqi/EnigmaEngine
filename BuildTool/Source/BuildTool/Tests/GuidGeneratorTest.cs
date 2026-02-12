using BuildTool.Utils;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for GuidGenerator (UUID v5 deterministic GUID generation).
/// </summary>
public static class GuidGeneratorTest
{
    public static void Run()
    {
        Console.WriteLine("=== GuidGenerator Smoke Tests ===");
        Console.WriteLine();

        TestDeterminism();
        TestUniqueness();
        TestUuidV5Format();
        TestEmptyString();
        TestSpecialCharacters();
        TestPrefixIsolation();
        TestFilterPathVariation();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    /// <summary>
    /// Same name must produce the same GUID across multiple calls.
    /// </summary>
    private static void TestDeterminism()
    {
        Console.WriteLine("[Test 1] Determinism: same name -> same GUID");

        var guid1 = GuidGenerator.GenerateFromName("Core");
        var guid2 = GuidGenerator.GenerateFromName("Core");

        Assert(guid1 == guid2, $"Expected identical GUIDs, got {guid1} and {guid2}");
        Assert(guid1 != Guid.Empty, "GUID should not be empty");

        Console.WriteLine($"  PASSED (Core -> {guid1})");
    }

    /// <summary>
    /// Different names must produce different GUIDs.
    /// </summary>
    private static void TestUniqueness()
    {
        Console.WriteLine("[Test 2] Uniqueness: different names -> different GUIDs");

        var guidCore = GuidGenerator.GenerateFromName("Core");
        var guidEngine = GuidGenerator.GenerateFromName("Engine");
        var guidLaunch = GuidGenerator.GenerateFromName("Launch");

        Assert(guidCore != guidEngine, "Core and Engine should have different GUIDs");
        Assert(guidEngine != guidLaunch, "Engine and Launch should have different GUIDs");
        Assert(guidCore != guidLaunch, "Core and Launch should have different GUIDs");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Output must conform to UUID v5 format: version=5, variant=RFC4122.
    /// </summary>
    private static void TestUuidV5Format()
    {
        Console.WriteLine("[Test 3] UUID v5 format: version=5, variant=RFC4122");

        var guid = GuidGenerator.GenerateFromName("TestModule");
        byte[] bytes = guid.ToByteArray();

        // .NET Guid.ToByteArray() returns mixed-endian:
        // bytes[0..3] = Data1 (little-endian)
        // bytes[4..5] = Data2 (little-endian)
        // bytes[6..7] = Data3 (little-endian) — version is in high nibble of Data3
        // bytes[8..15] = Data4 (big-endian) — variant is in high bits of byte[8]

        // Version: high nibble of byte[7] (Data3 high byte in little-endian)
        int version = (bytes[7] >> 4) & 0x0F;
        Assert(version == 5, $"Expected version 5, got {version}");

        // Variant: high 2 bits of byte[8] should be 10 (binary)
        int variant = (bytes[8] >> 6) & 0x03;
        Assert(variant == 2, $"Expected variant 2 (RFC4122), got {variant}");

        Console.WriteLine($"  PASSED (version={version}, variant={variant})");
    }

    /// <summary>
    /// Empty string should not throw and should return a valid GUID.
    /// </summary>
    private static void TestEmptyString()
    {
        Console.WriteLine("[Test 4] Empty string: no exception, valid GUID");

        var guid = GuidGenerator.GenerateFromName("");

        Assert(guid != Guid.Empty, "Empty string should produce a non-empty GUID");

        Console.WriteLine($"  PASSED ({guid})");
    }

    /// <summary>
    /// Names with special characters should produce valid, distinct GUIDs.
    /// </summary>
    private static void TestSpecialCharacters()
    {
        Console.WriteLine("[Test 5] Special characters: spaces, dots, slashes");

        var guid1 = GuidGenerator.GenerateFromName("My Module");
        var guid2 = GuidGenerator.GenerateFromName("path/to/module");
        var guid3 = GuidGenerator.GenerateFromName("module.name.v2");

        Assert(guid1 != Guid.Empty, "Space name should produce valid GUID");
        Assert(guid2 != Guid.Empty, "Slash name should produce valid GUID");
        Assert(guid3 != Guid.Empty, "Dot name should produce valid GUID");
        Assert(guid1 != guid2 && guid2 != guid3 && guid1 != guid3,
            "Different special-char names should produce different GUIDs");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Convenience methods with different prefixes must produce distinct GUIDs
    /// for the same base name.
    /// </summary>
    private static void TestPrefixIsolation()
    {
        Console.WriteLine("[Test 6] Prefix isolation: folder vs project vs filter");

        var folderGuid = GuidGenerator.GenerateForFolder("Core");
        var projectGuid = GuidGenerator.GenerateForProject("Core");
        var filterGuid = GuidGenerator.GenerateForFilter("Core", "Public");

        Assert(folderGuid != projectGuid,
            "Folder and Project GUIDs for same name should differ");
        Assert(projectGuid != filterGuid,
            "Project and Filter GUIDs for same name should differ");
        Assert(folderGuid != filterGuid,
            "Folder and Filter GUIDs for same name should differ");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Different filter paths for the same module must produce different GUIDs.
    /// </summary>
    private static void TestFilterPathVariation()
    {
        Console.WriteLine("[Test 7] Filter path variation: Public vs Private");

        var publicGuid = GuidGenerator.GenerateForFilter("Core", "Public");
        var privateGuid = GuidGenerator.GenerateForFilter("Core", "Private");

        Assert(publicGuid != privateGuid,
            "Public and Private filter GUIDs should differ");

        // Determinism check
        var publicGuid2 = GuidGenerator.GenerateForFilter("Core", "Public");
        Assert(publicGuid == publicGuid2,
            "Same filter path should produce same GUID");

        Console.WriteLine("  PASSED");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
