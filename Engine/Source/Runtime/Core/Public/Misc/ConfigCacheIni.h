// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------------
// ConfigCacheIni.h
//
// INI configuration system types: FConfigValue, FConfigSection,
// FConfigFile, and FConfigCacheIni.
//
// All config types are defined in this single header, mirroring
// UE's Public/Misc/ConfigCacheIni.h convention.
// -------------------------------------------------------------

namespace Enigma
{

// -------------------------------------------------------------
// EConfigValueType
//
// Describes how a config value was set, used during layer
// merging to apply array semantics.
// Mirrors UE's FConfigValue::EValueType.
// -------------------------------------------------------------

/// Describes the origin/intent of a config value for merge semantics.
enum class EConfigValueType : uint8_t
{
    /// Normal assignment: Key=Value
    Set,

    /// Array add unique: +Key=Value (skip if already present)
    ArrayAdd,

    /// Array add unconditional: .Key=Value (always append)
    ArrayAddUnique,

    /// Array remove: -Key=Value (remove matching entry)
    Remove,

    /// Clear all values for key: !Key=ClearArray
    Clear,
};

// -------------------------------------------------------------
// FConfigValue
//
// A single config value with its type annotation.
// Mirrors UE's FConfigValue (Value + EValueType).
// -------------------------------------------------------------

/// A single config value paired with its merge-semantic type.
struct CORE_API FConfigValue
{
    std::string Value;
    EConfigValueType ValueType = EConfigValueType::Set;

    FConfigValue() = default;

    explicit FConfigValue(std::string InValue,
                          EConfigValueType InType = EConfigValueType::Set)
        : Value(std::move(InValue))
        , ValueType(InType)
    {
    }

    bool operator==(const FConfigValue&) const = default;
    bool operator!=(const FConfigValue&) const = default;
};

// -------------------------------------------------------------
// FConfigSection
//
// An ordered collection of key-value pairs within an INI section.
// Uses std::multimap to support duplicate keys (array semantics).
// Mirrors UE's FConfigSection (inherits TMultiMap<FName, FConfigValue>).
// -------------------------------------------------------------

/// A named section in an INI file, holding ordered key-value pairs.
/// Duplicate keys are allowed (multimap) for array-style config entries.
class CORE_API FConfigSection
{
public:
    using StorageType = std::multimap<std::string, FConfigValue>;
    using Iterator = StorageType::iterator;
    using ConstIterator = StorageType::const_iterator;

    FConfigSection() = default;
    ~FConfigSection() = default;

    FConfigSection(const FConfigSection&) = default;
    FConfigSection& operator=(const FConfigSection&) = default;
    FConfigSection(FConfigSection&&) noexcept = default;
    FConfigSection& operator=(FConfigSection&&) noexcept = default;

    // ----- Value Access -----

    /// Get the first value for a key. Returns nullptr if not found.
    [[nodiscard]] const FConfigValue* GetValue(const std::string& key) const;

    /// Get all values for a key (for array-style entries).
    [[nodiscard]] std::vector<const FConfigValue*> GetValues(const std::string& key) const;

    /// Get the first value string for a key. Returns nullptr if not found.
    [[nodiscard]] const std::string* GetValueString(const std::string& key) const;

    /// Get all value strings for a key.
    [[nodiscard]] std::vector<std::string> GetValueStrings(const std::string& key) const;

    // ----- Modification -----

    /// Set a key to a single value (replaces all existing entries for this key).
    void SetValue(const std::string& key, const FConfigValue& value);

    /// Set a key to a single string value with Set type.
    void SetValue(const std::string& key, const std::string& value);

    /// Add a value for a key (appends, allows duplicates).
    void AddValue(const std::string& key, const FConfigValue& value);

    /// Add a value only if no entry with the same key and value string exists.
    void AddUniqueValue(const std::string& key, const FConfigValue& value);

    /// Remove all entries matching key and value string.
    /// Returns the number of entries removed.
    int32_t RemoveValue(const std::string& key, const std::string& value);

    /// Remove all entries for a key.
    void ClearKey(const std::string& key);

    /// Remove all entries in this section.
    void Clear();

    // ----- Query -----

    /// Check if any entry exists for a key.
    [[nodiscard]] bool Contains(const std::string& key) const;

    /// Get the total number of entries (including duplicate keys).
    [[nodiscard]] int32_t Num() const;

    /// Check if the section is empty.
    [[nodiscard]] bool IsEmpty() const;

    // ----- Iteration -----

    [[nodiscard]] Iterator begin() { return m_data.begin(); }
    [[nodiscard]] Iterator end() { return m_data.end(); }
    [[nodiscard]] ConstIterator begin() const { return m_data.begin(); }
    [[nodiscard]] ConstIterator end() const { return m_data.end(); }
    [[nodiscard]] ConstIterator cbegin() const { return m_data.cbegin(); }
    [[nodiscard]] ConstIterator cend() const { return m_data.cend(); }

    // ----- Comparison -----

    bool operator==(const FConfigSection& other) const;
    bool operator!=(const FConfigSection& other) const;

private:
    StorageType m_data;
};

// -------------------------------------------------------------
// FConfigFile
//
// Represents a single parsed INI file.
// Handles parsing, serialization, section/key/value access.
// Supports UE-style array operators (+/-/./!) and comments.
// Aligns with UE's FConfigFile (TMap<FString, FConfigSection>).
// -------------------------------------------------------------

/// A parsed INI file containing named sections.
class CORE_API FConfigFile
{
public:
    using SectionMap = std::map<std::string, FConfigSection>;

    FConfigFile() = default;
    ~FConfigFile() = default;

    FConfigFile(const FConfigFile&) = default;
    FConfigFile& operator=(const FConfigFile&) = default;
    FConfigFile(FConfigFile&&) noexcept = default;
    FConfigFile& operator=(FConfigFile&&) noexcept = default;

    // ----- I/O -----

    /// Parse an INI file from disk. Returns true on success.
    bool Read(const std::string& filePath);

    /// Parse INI content from a string (for testing).
    bool ReadFromString(const std::string& content);

    /// Serialize to disk. Returns true on success.
    bool Write(const std::string& filePath) const;

    /// Serialize to string (for testing/round-trip).
    [[nodiscard]] std::string WriteToString() const;

    // ----- Section Access -----

    /// Get a section by name. Returns nullptr if not found.
    [[nodiscard]] const FConfigSection* FindSection(const std::string& sectionName) const;

    /// Get or create a section by name.
    FConfigSection* FindOrAddSection(const std::string& sectionName);

    /// Get all section names.
    [[nodiscard]] std::vector<std::string> GetSectionNames() const;

    /// Check if a section exists.
    [[nodiscard]] bool ContainsSection(const std::string& sectionName) const;

    /// Check if the file has no sections.
    [[nodiscard]] bool IsEmpty() const;

    // ----- Comparison -----

    bool operator==(const FConfigFile& other) const;
    bool operator!=(const FConfigFile& other) const;

private:
    /// Section map: section name -> FConfigSection.
    SectionMap m_sections;

    /// Parse a single line within the current section context.
    void ParseLine(const std::string& line, std::string& currentSection);

    /// Parse all lines from a stream.
    void ParseLines(const std::string& content);
};

// -------------------------------------------------------------
// FConfigDomain
//
// Named config domain (e.g., "Engine", "Game").
// Holds the merged result of multiple layers.
// -------------------------------------------------------------

/// A named config domain holding the merged config result.
struct CORE_API FConfigDomain
{
    std::string Name;
    FConfigFile MergedConfig;
    bool bDirty = false;
};

// -------------------------------------------------------------
// FConfigCacheIni
//
// Global config cache managing all config domains.
// Supports engine base, plugin, project default, and user layers.
// Mirrors UE's FConfigCacheIni (simplified).
// -------------------------------------------------------------

/// Global config cache -- manages all config domains.
/// Supports engine base, plugin, project default, and user layers.
class CORE_API FConfigCacheIni
{
public:
    FConfigCacheIni() = default;
    ~FConfigCacheIni() = default;

    // Non-copyable
    FConfigCacheIni(const FConfigCacheIni&) = delete;
    FConfigCacheIni& operator=(const FConfigCacheIni&) = delete;

    /// Initialize the config cache with engine and project root paths.
    /// Loads standard config domains (Engine, Game, GameUserSettings, Input).
    void Initialize(const std::string& engineConfigDir, const std::string& projectConfigDir);

    /// Register a plugin's config directory for dual-track loading.
    /// UE equivalent: FConfigCacheIni::RegisterPlugin()
    void RegisterPlugin(const std::string& pluginName, const std::string& pluginConfigDir);

    /// Load a single config domain by merging layers.
    void LoadConfigDomain(const std::string& domainName);

    // ----- Typed Getters -----

    bool GetString(const std::string& section, const std::string& key,
                   std::string& outValue, const std::string& configName) const;

    bool GetInt(const std::string& section, const std::string& key,
                int32_t& outValue, const std::string& configName) const;

    bool GetFloat(const std::string& section, const std::string& key,
                  float& outValue, const std::string& configName) const;

    bool GetBool(const std::string& section, const std::string& key,
                 bool& outValue, const std::string& configName) const;

    int32_t GetArray(const std::string& section, const std::string& key,
                     std::vector<std::string>& outValues, const std::string& configName) const;

    // ----- Typed Setters (in-memory only) -----

    void SetString(const std::string& section, const std::string& key,
                   const std::string& value, const std::string& configName);

    void SetInt(const std::string& section, const std::string& key,
                int32_t value, const std::string& configName);

    void SetFloat(const std::string& section, const std::string& key,
                  float value, const std::string& configName);

    void SetBool(const std::string& section, const std::string& key,
                 bool value, const std::string& configName);

    // ----- Section Access -----

    /// Get a section as a const reference. Returns nullptr if not found.
    const FConfigSection* GetSection(const std::string& section,
                                     const std::string& configName) const;

    // ----- Persistence -----

    /// Save a config domain's User layer to disk.
    bool SaveConfig(const std::string& configName);

    /// Save all dirty config domains.
    void SaveAllConfigs();

private:
    std::string m_engineConfigDir;
    std::string m_projectConfigDir;

    /// Registered plugin config directories (pluginName -> configDir).
    std::vector<std::pair<std::string, std::string>> m_pluginConfigDirs;

    /// Config domains indexed by name.
    std::unordered_map<std::string, FConfigDomain> m_domains;

    /// Find a domain by name. Returns nullptr if not loaded.
    const FConfigDomain* FindDomain(const std::string& configName) const;
    FConfigDomain* FindDomain(const std::string& configName);

    /// Merge a source FConfigFile into a destination, applying array operators.
    static void MergeConfig(FConfigFile& dest, const FConfigFile& source);
};

} // namespace Enigma
