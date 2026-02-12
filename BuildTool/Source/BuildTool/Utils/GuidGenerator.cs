// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Utils;

using System.Security.Cryptography;
using System.Text;

/// <summary>
/// Generates deterministic GUIDs (UUID v5, SHA-1 based) from name strings.
/// Same name always produces the same GUID across runs.
/// </summary>
public static class GuidGenerator
{
    /// <summary>EnigmaEngine namespace UUID for deterministic GUID generation.</summary>
    private static readonly Guid EngineNamespace =
        new("A1B2C3D4-E5F6-7890-ABCD-EF1234567890");

    /// <summary>
    /// Generate a deterministic GUID from a name string using UUID v5 (SHA-1).
    /// </summary>
    /// <param name="name">Input name (must not be null).</param>
    /// <returns>A deterministic GUID derived from the name.</returns>
    public static Guid GenerateFromName(string name)
    {
        ArgumentNullException.ThrowIfNull(name);

        // 1. Convert namespace GUID to big-endian byte array
        byte[] namespaceBytes = GuidToNetworkOrder(EngineNamespace);

        // 2. Append UTF-8 encoded name bytes
        byte[] nameBytes = Encoding.UTF8.GetBytes(name);

        // 3. Compute SHA-1 hash
        byte[] hashInput = new byte[namespaceBytes.Length + nameBytes.Length];
        Buffer.BlockCopy(namespaceBytes, 0, hashInput, 0, namespaceBytes.Length);
        Buffer.BlockCopy(nameBytes, 0, hashInput, namespaceBytes.Length, nameBytes.Length);

        byte[] hash = SHA1.HashData(hashInput);

        // 4. Set version = 5 (bits 4-7 of byte 6)
        hash[6] = (byte)((hash[6] & 0x0F) | 0x50);

        // 5. Set variant = RFC 4122 (bits 6-7 of byte 8)
        hash[8] = (byte)((hash[8] & 0x3F) | 0x80);

        // 6. Construct GUID from first 16 bytes (network order → .NET GUID order)
        return GuidFromNetworkOrder(hash);
    }

    /// <summary>
    /// Generate a deterministic GUID for a solution folder path.
    /// </summary>
    public static Guid GenerateForFolder(string folderPath)
        => GenerateFromName($"folder:{folderPath}");

    /// <summary>
    /// Generate a deterministic GUID for a module project.
    /// </summary>
    public static Guid GenerateForProject(string moduleName)
        => GenerateFromName($"project:{moduleName}");

    /// <summary>
    /// Generate a deterministic GUID for a .vcxproj.filters entry.
    /// </summary>
    public static Guid GenerateForFilter(string moduleName, string filterPath)
        => GenerateFromName($"filter:{moduleName}:{filterPath}");

    /// <summary>
    /// Convert a .NET Guid to big-endian (network order) byte array.
    /// .NET stores the first 3 components in little-endian; UUID requires big-endian.
    /// </summary>
    private static byte[] GuidToNetworkOrder(Guid guid)
    {
        byte[] bytes = guid.ToByteArray();

        // Swap first 4 bytes (Data1: uint32)
        (bytes[0], bytes[3]) = (bytes[3], bytes[0]);
        (bytes[1], bytes[2]) = (bytes[2], bytes[1]);

        // Swap bytes 4-5 (Data2: uint16)
        (bytes[4], bytes[5]) = (bytes[5], bytes[4]);

        // Swap bytes 6-7 (Data3: uint16)
        (bytes[6], bytes[7]) = (bytes[7], bytes[6]);

        // Bytes 8-15 are already in correct order
        return bytes;
    }

    /// <summary>
    /// Convert big-endian (network order) bytes back to a .NET Guid.
    /// </summary>
    private static Guid GuidFromNetworkOrder(byte[] bytes)
    {
        // Swap first 4 bytes (Data1: uint32)
        (bytes[0], bytes[3]) = (bytes[3], bytes[0]);
        (bytes[1], bytes[2]) = (bytes[2], bytes[1]);

        // Swap bytes 4-5 (Data2: uint16)
        (bytes[4], bytes[5]) = (bytes[5], bytes[4]);

        // Swap bytes 6-7 (Data3: uint16)
        (bytes[6], bytes[7]) = (bytes[7], bytes[6]);

        return new Guid(bytes.AsSpan(0, 16));
    }
}
