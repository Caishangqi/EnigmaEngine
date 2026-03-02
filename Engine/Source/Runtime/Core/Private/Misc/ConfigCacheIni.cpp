// Copyright EnigmaEngine. All Rights Reserved.

#include "Misc/ConfigCacheIni.h"
#include "Misc/ConfigDelegates.h"
#include "Logging/LogMacros.h"
#include "Logging/LogCategory.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace Enigma
{

// =============================================================
// FConfigSection
// =============================================================

const FConfigValue* FConfigSection::GetValue(const std::string& key) const
{
    auto it = m_data.find(key);
    if (it != m_data.end())
    {
        return &it->second;
    }
    return nullptr;
}

std::vector<const FConfigValue*> FConfigSection::GetValues(const std::string& key) const
{
    std::vector<const FConfigValue*> result;
    auto [first, last] = m_data.equal_range(key);
    for (auto it = first; it != last; ++it)
    {
        result.push_back(&it->second);
    }
    return result;
}

const std::string* FConfigSection::GetValueString(const std::string& key) const
{
    const FConfigValue* val = GetValue(key);
    if (val != nullptr)
    {
        return &val->Value;
    }
    return nullptr;
}

std::vector<std::string> FConfigSection::GetValueStrings(const std::string& key) const
{
    std::vector<std::string> result;
    auto [first, last] = m_data.equal_range(key);
    for (auto it = first; it != last; ++it)
    {
        result.push_back(it->second.Value);
    }
    return result;
}

void FConfigSection::SetValue(const std::string& key, const FConfigValue& value)
{
    m_data.erase(key);
    m_data.emplace(key, value);
}

void FConfigSection::SetValue(const std::string& key, const std::string& value)
{
    SetValue(key, FConfigValue(value, EConfigValueType::Set));
}

void FConfigSection::AddValue(const std::string& key, const FConfigValue& value)
{
    m_data.emplace(key, value);
}

void FConfigSection::AddUniqueValue(const std::string& key, const FConfigValue& value)
{
    auto [first, last] = m_data.equal_range(key);
    for (auto it = first; it != last; ++it)
    {
        if (it->second.Value == value.Value)
        {
            return; // Already exists
        }
    }
    m_data.emplace(key, value);
}

int32_t FConfigSection::RemoveValue(const std::string& key, const std::string& value)
{
    int32_t removed = 0;
    auto [first, last] = m_data.equal_range(key);
    for (auto it = first; it != last; )
    {
        if (it->second.Value == value)
        {
            it = m_data.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }
    return removed;
}

void FConfigSection::ClearKey(const std::string& key)
{
    m_data.erase(key);
}

void FConfigSection::Clear()
{
    m_data.clear();
}

bool FConfigSection::Contains(const std::string& key) const
{
    return m_data.contains(key);
}

int32_t FConfigSection::Num() const
{
    return static_cast<int32_t>(m_data.size());
}

bool FConfigSection::IsEmpty() const
{
    return m_data.empty();
}

bool FConfigSection::operator==(const FConfigSection& other) const
{
    return m_data == other.m_data;
}

bool FConfigSection::operator!=(const FConfigSection& other) const
{
    return !(*this == other);
}

// =============================================================
// FConfigFile
// =============================================================

DEFINE_LOG_CATEGORY_STATIC(LogConfig, Info, All);

// ----- Helpers -----

namespace
{
    /// Trim leading and trailing whitespace from a string.
    std::string TrimWhitespace(const std::string& str)
    {
        const char* ws = " \t\r\n";
        size_t start = str.find_first_not_of(ws);
        if (start == std::string::npos)
        {
            return {};
        }
        size_t end = str.find_last_not_of(ws);
        return str.substr(start, end - start + 1);
    }
} // anonymous namespace

// ----- Parsing -----

bool FConfigFile::Read(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    ParseLines(ss.str());
    return true;
}

bool FConfigFile::ReadFromString(const std::string& content)
{
    m_sections.clear();
    ParseLines(content);
    return true;
}

void FConfigFile::ParseLines(const std::string& content)
{
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;

    while (std::getline(stream, line))
    {
        ParseLine(line, currentSection);
    }
}

void FConfigFile::ParseLine(const std::string& rawLine, std::string& currentSection)
{
    std::string line = TrimWhitespace(rawLine);

    // Skip empty lines
    if (line.empty())
    {
        return;
    }

    // Skip comments (; or #)
    if (line[0] == ';' || line[0] == '#')
    {
        return;
    }

    // Section header: [SectionName]
    if (line[0] == '[')
    {
        size_t close = line.find(']');
        if (close != std::string::npos)
        {
            currentSection = TrimWhitespace(line.substr(1, close - 1));
        }
        else
        {
            ENIGMA_LOG(LogConfig, Warning, "Malformed section header: {}", rawLine);
        }
        return;
    }

    // Must have a current section to assign key-value pairs
    if (currentSection.empty())
    {
        ENIGMA_LOG(LogConfig, Warning, "Key-value pair before any section: {}", rawLine);
        return;
    }

    // Detect array operator prefix
    EConfigValueType valueType = EConfigValueType::Set;
    std::string keyValueStr = line;

    if (!line.empty())
    {
        char prefix = line[0];
        if (prefix == '+')
        {
            valueType = EConfigValueType::ArrayAddUnique;
            keyValueStr = line.substr(1);
        }
        else if (prefix == '.')
        {
            valueType = EConfigValueType::ArrayAdd;
            keyValueStr = line.substr(1);
        }
        else if (prefix == '-')
        {
            valueType = EConfigValueType::Remove;
            keyValueStr = line.substr(1);
        }
        else if (prefix == '!')
        {
            valueType = EConfigValueType::Clear;
            keyValueStr = line.substr(1);
        }
    }

    // Split on first '='
    size_t eqPos = keyValueStr.find('=');
    if (eqPos == std::string::npos)
    {
        ENIGMA_LOG(LogConfig, Warning, "Malformed line (no '='): {}", rawLine);
        return;
    }

    std::string key = TrimWhitespace(keyValueStr.substr(0, eqPos));
    std::string value = TrimWhitespace(keyValueStr.substr(eqPos + 1));

    if (key.empty())
    {
        ENIGMA_LOG(LogConfig, Warning, "Empty key in line: {}", rawLine);
        return;
    }

    // Add to the current section
    FConfigSection* section = FindOrAddSection(currentSection);
    section->AddValue(key, FConfigValue(std::move(value), valueType));
}

// ----- Serialization -----

bool FConfigFile::Write(const std::string& filePath) const
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        return false;
    }

    file << WriteToString();
    return file.good();
}

std::string FConfigFile::WriteToString() const
{
    std::ostringstream ss;
    bool firstSection = true;

    for (const auto& [sectionName, section] : m_sections)
    {
        if (!firstSection)
        {
            ss << '\n';
        }
        firstSection = false;

        ss << '[' << sectionName << "]\n";

        for (const auto& [key, configValue] : section)
        {
            // Write array operator prefix
            switch (configValue.ValueType)
            {
            case EConfigValueType::ArrayAddUnique:
                ss << '+';
                break;
            case EConfigValueType::ArrayAdd:
                ss << '.';
                break;
            case EConfigValueType::Remove:
                ss << '-';
                break;
            case EConfigValueType::Clear:
                ss << '!';
                break;
            case EConfigValueType::Set:
            default:
                break;
            }

            ss << key << '=' << configValue.Value << '\n';
        }
    }

    return ss.str();
}

// ----- Section Access -----

const FConfigSection* FConfigFile::FindSection(const std::string& sectionName) const
{
    auto it = m_sections.find(sectionName);
    if (it != m_sections.end())
    {
        return &it->second;
    }
    return nullptr;
}

FConfigSection* FConfigFile::FindOrAddSection(const std::string& sectionName)
{
    return &m_sections[sectionName];
}

std::vector<std::string> FConfigFile::GetSectionNames() const
{
    std::vector<std::string> names;
    names.reserve(m_sections.size());
    for (const auto& [name, section] : m_sections)
    {
        names.push_back(name);
    }
    return names;
}

bool FConfigFile::ContainsSection(const std::string& sectionName) const
{
    return m_sections.contains(sectionName);
}

bool FConfigFile::IsEmpty() const
{
    return m_sections.empty();
}

bool FConfigFile::operator==(const FConfigFile& other) const
{
    return m_sections == other.m_sections;
}

bool FConfigFile::operator!=(const FConfigFile& other) const
{
    return !(*this == other);
}

// =============================================================
// FConfigCacheIni
// =============================================================

namespace
{
    /// Standard config domain names loaded during initialization.
    constexpr const char* StandardDomains[] = {"Engine", "Game", "GameUserSettings", "Input"};

    /// Build a layer file path: {dir}/{prefix}{domainName}.ini
    std::string BuildConfigPath(const std::string& dir, const std::string& prefix,
                                const std::string& domainName)
    {
        if (dir.empty())
        {
            return {};
        }
        std::string path = dir;
        if (!path.empty() && path.back() != '/' && path.back() != '\\')
        {
            path += '/';
        }
        path += prefix;
        path += domainName;
        path += ".ini";
        return path;
    }
} // anonymous namespace

void FConfigCacheIni::Initialize(const std::string& engineConfigDir,
                                  const std::string& projectConfigDir)
{
    m_engineConfigDir = engineConfigDir;
    m_projectConfigDir = projectConfigDir;

    // Load standard config domains.
    for (const char* domain : StandardDomains)
    {
        LoadConfigDomain(domain);
    }
}

void FConfigCacheIni::RegisterPlugin(const std::string& pluginName,
                                      const std::string& pluginConfigDir)
{
    m_pluginConfigDirs.emplace_back(pluginName, pluginConfigDir);

    // Re-merge existing domains to pick up plugin modification layers.
    for (auto& [name, domain] : m_domains)
    {
        LoadConfigDomain(name);
    }

    // Check for plugin self-owned domain: Default{PluginName}.ini
    std::string selfOwnedPath = BuildConfigPath(pluginConfigDir, "Default", pluginName);
    FConfigFile selfOwned;
    if (selfOwned.Read(selfOwnedPath))
    {
        // Create or update the plugin's own domain.
        LoadConfigDomain(pluginName);
    }
}

void FConfigCacheIni::LoadConfigDomain(const std::string& domainName)
{
    FConfigFile merged;

    // Layer 1: Engine base -- {engineConfigDir}/Base{Type}.ini
    {
        std::string path = BuildConfigPath(m_engineConfigDir, "Base", domainName);
        FConfigFile layer;
        if (layer.Read(path))
        {
            MergeConfig(merged, layer);
        }
    }

    // Layer 2: Plugin modification layers -- {pluginConfigDir}/{Type}.ini
    for (const auto& [pluginName, pluginDir] : m_pluginConfigDirs)
    {
        std::string path = BuildConfigPath(pluginDir, "", domainName);
        FConfigFile layer;
        if (layer.Read(path))
        {
            MergeConfig(merged, layer);
        }
    }

    // Layer 3: Project default -- {projectConfigDir}/Default{Type}.ini
    {
        std::string path = BuildConfigPath(m_projectConfigDir, "Default", domainName);
        FConfigFile layer;
        if (layer.Read(path))
        {
            MergeConfig(merged, layer);
        }
    }

    // Layer 4: User overrides -- {projectConfigDir}/User{Type}.ini (optional)
    {
        std::string path = BuildConfigPath(m_projectConfigDir, "User", domainName);
        FConfigFile layer;
        if (layer.Read(path))
        {
            MergeConfig(merged, layer);
        }
    }

    // Store the merged result.
    FConfigDomain& domain = m_domains[domainName];
    domain.Name = domainName;
    domain.MergedConfig = std::move(merged);
    domain.bDirty = false;

    // Broadcast config loaded event.
    FConfigDelegates::OnConfigLoaded.Broadcast(domainName);
}

// ----- MergeConfig -----

void FConfigCacheIni::MergeConfig(FConfigFile& dest, const FConfigFile& source)
{
    for (const auto& sectionName : source.GetSectionNames())
    {
        const FConfigSection* srcSection = source.FindSection(sectionName);
        if (srcSection == nullptr)
        {
            continue;
        }

        FConfigSection* destSection = dest.FindOrAddSection(sectionName);

        for (const auto& [key, configValue] : *srcSection)
        {
            switch (configValue.ValueType)
            {
            case EConfigValueType::Set:
                // Last-wins: replace all existing values for this key.
                destSection->SetValue(key, configValue);
                break;

            case EConfigValueType::ArrayAdd:
                // Unconditional append.
                destSection->AddValue(key, FConfigValue(configValue.Value, EConfigValueType::Set));
                break;

            case EConfigValueType::ArrayAddUnique:
                // Add only if not already present.
                destSection->AddUniqueValue(key, FConfigValue(configValue.Value, EConfigValueType::Set));
                break;

            case EConfigValueType::Remove:
                // Remove matching entries.
                destSection->RemoveValue(key, configValue.Value);
                break;

            case EConfigValueType::Clear:
                // Remove all entries for this key.
                destSection->ClearKey(key);
                break;
            }
        }
    }
}

// ----- Typed Getters -----

bool FConfigCacheIni::GetString(const std::string& section, const std::string& key,
                                 std::string& outValue, const std::string& configName) const
{
    const FConfigDomain* domain = FindDomain(configName);
    if (domain == nullptr)
    {
        return false;
    }

    const FConfigSection* sec = domain->MergedConfig.FindSection(section);
    if (sec == nullptr)
    {
        return false;
    }

    const std::string* val = sec->GetValueString(key);
    if (val == nullptr)
    {
        return false;
    }

    outValue = *val;
    return true;
}

bool FConfigCacheIni::GetInt(const std::string& section, const std::string& key,
                              int32_t& outValue, const std::string& configName) const
{
    std::string strValue;
    if (!GetString(section, key, strValue, configName))
    {
        return false;
    }

    try
    {
        outValue = std::stoi(strValue);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool FConfigCacheIni::GetFloat(const std::string& section, const std::string& key,
                                float& outValue, const std::string& configName) const
{
    std::string strValue;
    if (!GetString(section, key, strValue, configName))
    {
        return false;
    }

    try
    {
        outValue = std::stof(strValue);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool FConfigCacheIni::GetBool(const std::string& section, const std::string& key,
                               bool& outValue, const std::string& configName) const
{
    std::string strValue;
    if (!GetString(section, key, strValue, configName))
    {
        return false;
    }

    // Accept: True/False, true/false, 1/0, Yes/No, yes/no
    if (strValue == "True" || strValue == "true" || strValue == "1" || strValue == "Yes" || strValue == "yes")
    {
        outValue = true;
        return true;
    }
    if (strValue == "False" || strValue == "false" || strValue == "0" || strValue == "No" || strValue == "no")
    {
        outValue = false;
        return true;
    }

    return false;
}

int32_t FConfigCacheIni::GetArray(const std::string& section, const std::string& key,
                                   std::vector<std::string>& outValues,
                                   const std::string& configName) const
{
    const FConfigDomain* domain = FindDomain(configName);
    if (domain == nullptr)
    {
        return 0;
    }

    const FConfigSection* sec = domain->MergedConfig.FindSection(section);
    if (sec == nullptr)
    {
        return 0;
    }

    outValues = sec->GetValueStrings(key);
    return static_cast<int32_t>(outValues.size());
}

// ----- Typed Setters -----

void FConfigCacheIni::SetString(const std::string& section, const std::string& key,
                                 const std::string& value, const std::string& configName)
{
    FConfigDomain& domain = m_domains[configName];
    if (domain.Name.empty())
    {
        domain.Name = configName;
    }

    FConfigSection* sec = domain.MergedConfig.FindOrAddSection(section);
    sec->SetValue(key, value);
    domain.bDirty = true;

    FConfigDelegates::OnConfigSectionChanged.Broadcast(configName, section);
}

void FConfigCacheIni::SetInt(const std::string& section, const std::string& key,
                              int32_t value, const std::string& configName)
{
    SetString(section, key, std::to_string(value), configName);
}

void FConfigCacheIni::SetFloat(const std::string& section, const std::string& key,
                                float value, const std::string& configName)
{
    SetString(section, key, std::to_string(value), configName);
}

void FConfigCacheIni::SetBool(const std::string& section, const std::string& key,
                               bool value, const std::string& configName)
{
    SetString(section, key, value ? "True" : "False", configName);
}

// ----- Section Access -----

const FConfigSection* FConfigCacheIni::GetSection(const std::string& section,
                                                    const std::string& configName) const
{
    const FConfigDomain* domain = FindDomain(configName);
    if (domain == nullptr)
    {
        return nullptr;
    }
    return domain->MergedConfig.FindSection(section);
}

// ----- Persistence -----

bool FConfigCacheIni::SaveConfig(const std::string& configName)
{
    FConfigDomain* domain = FindDomain(configName);
    if (domain == nullptr)
    {
        return false;
    }

    std::string path = BuildConfigPath(m_projectConfigDir, "User", configName);
    if (domain->MergedConfig.Write(path))
    {
        domain->bDirty = false;
        return true;
    }
    return false;
}

void FConfigCacheIni::SaveAllConfigs()
{
    for (auto& [name, domain] : m_domains)
    {
        if (domain.bDirty)
        {
            SaveConfig(name);
        }
    }
}

// ----- Domain Lookup -----

const FConfigDomain* FConfigCacheIni::FindDomain(const std::string& configName) const
{
    auto it = m_domains.find(configName);
    if (it != m_domains.end())
    {
        return &it->second;
    }
    return nullptr;
}

FConfigDomain* FConfigCacheIni::FindDomain(const std::string& configName)
{
    auto it = m_domains.find(configName);
    if (it != m_domains.end())
    {
        return &it->second;
    }
    return nullptr;
}

} // namespace Enigma
