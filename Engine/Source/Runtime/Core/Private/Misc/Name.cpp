// Copyright EnigmaEngine. All Rights Reserved.

#include "Misc/Name.h"
#include "Misc/AssertionMacros.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogName, Info, All);

namespace Enigma
{

// -------------------------------------------------------------
// FNameTable
// -------------------------------------------------------------

FNameTable::FNameTable()
{
	// Index 0 = NAME_None (empty string)
	m_entries.emplace_back("");
	m_lookupMap.emplace(std::string_view(m_entries[0]), 0);
}

FNameTable& FNameTable::Get()
{
	static FNameTable instance;
	return instance;
}

uint32_t FNameTable::FindOrInsert(std::string_view str)
{
	// Empty string always maps to NAME_None
	if (str.empty())
	{
		return 0;
	}

	// Look up existing entry
	auto it = m_lookupMap.find(str);
	if (it != m_lookupMap.end())
	{
		return it->second;
	}

	// Insert new entry
	uint32_t index = static_cast<uint32_t>(m_entries.size());
	m_entries.emplace_back(str);

	// string_view must point into the stable storage (m_entries owns the string)
	std::string_view stableView(m_entries.back());
	m_lookupMap.emplace(stableView, index);

	ENIGMA_LOG(LogName, Verbose, "FName registered: '{}' -> index {}", m_entries.back(), index);

	return index;
}

std::string_view FNameTable::Resolve(uint32_t index) const
{
	checkf(index < m_entries.size(),
		"FNameTable::Resolve: index {} out of range (table size {})",
		index, m_entries.size());
	return std::string_view(m_entries[index]);
}

// -------------------------------------------------------------
// FName
// -------------------------------------------------------------

FName::FName(const char* str)
	: m_index(FNameTable::Get().FindOrInsert(str ? std::string_view(str) : std::string_view()))
{
}

FName::FName(const std::string& str)
	: m_index(FNameTable::Get().FindOrInsert(str))
{
}

FName::FName(std::string_view str)
	: m_index(FNameTable::Get().FindOrInsert(str))
{
}

std::string_view FName::ToString() const
{
	return FNameTable::Get().Resolve(m_index);
}

const char* FName::operator*() const
{
	// Resolve returns a view into std::string in the table, which is null-terminated
	return FNameTable::Get().Resolve(m_index).data();
}

} // namespace Enigma
