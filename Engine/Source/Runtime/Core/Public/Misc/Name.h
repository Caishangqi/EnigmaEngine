// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>

// -------------------------------------------------------------
// FName -- Lightweight Name System
//
// Global name table with uint32 index for O(1) comparison.
// Follows UE's FName pattern (simplified, single-threaded).
//
// Usage:
//   FName name("PlayerObject");
//   const char* str = *name;           // "PlayerObject"
//   std::string_view sv = name.ToString(); // string_view into table
//   FName other("PlayerObject");
//   check(name == other);              // true, index comparison
//
// NAME_None (index 0) is the default-constructed FName.
// Case-sensitive. No thread safety (single-threaded game loop).
// -------------------------------------------------------------

namespace Enigma
{

// Forward declaration
class FNameTable;

/// Lightweight name handle -- holds a uint32 index into the global name table.
/// Comparison is O(1) via index. Default value is NAME_None (index 0).
class CORE_API FName
{
public:
	/// Default constructor: NAME_None (index 0).
	FName() noexcept = default;

	/// Construct from C string. Inserts into global table if not present.
	explicit FName(const char* str);

	/// Construct from std::string. Inserts into global table if not present.
	explicit FName(const std::string& str);

	/// Construct from string_view. Inserts into global table if not present.
	explicit FName(std::string_view str);

	/// Get the raw index into the name table.
	[[nodiscard]] uint32_t GetIndex() const noexcept { return m_index; }

	/// Return a string_view into the global name table (zero-copy).
	[[nodiscard]] std::string_view ToString() const;

	/// Return const char* into the global name table (UE's *FName() pattern).
	[[nodiscard]] const char* operator*() const;

	/// Check if this is NAME_None (index 0).
	[[nodiscard]] bool IsNone() const noexcept { return m_index == 0; }

	// -- Comparison (index-based, O(1)) --

	[[nodiscard]] bool operator==(const FName& other) const noexcept
	{
		return m_index == other.m_index;
	}

	[[nodiscard]] bool operator!=(const FName& other) const noexcept
	{
		return m_index != other.m_index;
	}

	[[nodiscard]] bool operator<(const FName& other) const noexcept
	{
		return m_index < other.m_index;
	}

private:
	uint32_t m_index = 0; // 0 = NAME_None
};

// -------------------------------------------------------------
// FNameTable -- Global name table singleton
//
// Stores deduplicated strings. Each unique string gets a uint32
// index. Index 0 is reserved for NAME_None (empty string "").
//
// Insert-or-find semantics: if the string already exists, return
// its existing index; otherwise insert and return the new index.
// -------------------------------------------------------------

class CORE_API FNameTable
{
public:
	/// Get the global singleton instance.
	static FNameTable& Get();

	/// Insert or find a name. Returns the index for the given string.
	/// Empty string always returns 0 (NAME_None).
	uint32_t FindOrInsert(std::string_view str);

	/// Retrieve the string for a given index.
	/// @pre index must be valid (< entry count).
	[[nodiscard]] std::string_view Resolve(uint32_t index) const;

	/// Get the number of entries in the table (including NAME_None).
	[[nodiscard]] uint32_t GetEntryCount() const noexcept
	{
		return static_cast<uint32_t>(m_entries.size());
	}

	// Non-copyable, non-movable
	FNameTable(const FNameTable&) = delete;
	FNameTable& operator=(const FNameTable&) = delete;
	FNameTable(FNameTable&&) = delete;
	FNameTable& operator=(FNameTable&&) = delete;

private:
	FNameTable();

	/// Stored strings. Index 0 = "" (NAME_None).
	/// Uses deque to guarantee pointer/reference stability on push_back
	/// (vector would invalidate string_view keys in m_lookupMap on reallocation).
	std::deque<std::string> m_entries;

	/// Lookup map: string_view (pointing into m_entries) -> index.
	std::unordered_map<std::string_view, uint32_t> m_lookupMap;
};

/// Global constant for the "None" name (index 0).
inline const FName NAME_None{};

} // namespace Enigma
