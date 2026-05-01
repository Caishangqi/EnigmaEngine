// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Misc/AssertionMacros.h"

#include <cstddef>
#include <new>

namespace Enigma
{

/// Default heap allocator policy for owning Core containers.
struct FDefaultAllocator
{
	static constexpr bool RequireRangeCheck = true;

	[[nodiscard]] static void* Allocate(std::size_t SizeBytes, std::size_t Alignment)
	{
		if (SizeBytes == 0)
		{
			return nullptr;
		}

		checkf(IsValidAlignment(Alignment), "Invalid allocation alignment: {}", Alignment);

		void* Ptr = Alignment > DefaultNewAlignment
			? ::operator new(SizeBytes, std::align_val_t(Alignment), std::nothrow)
			: ::operator new(SizeBytes, std::nothrow);

		checkf(Ptr != nullptr, "Failed to allocate {} bytes with alignment {}", SizeBytes, Alignment);
		return Ptr;
	}

	static void Free(void* Ptr, std::size_t Alignment) noexcept
	{
		if (Ptr == nullptr)
		{
			return;
		}

		if (Alignment > DefaultNewAlignment)
		{
			::operator delete(Ptr, std::align_val_t(Alignment));
			return;
		}

		::operator delete(Ptr);
	}

private:
#if defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
	static constexpr std::size_t DefaultNewAlignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#else
	static constexpr std::size_t DefaultNewAlignment = alignof(std::max_align_t);
#endif

	static constexpr bool IsValidAlignment(std::size_t Alignment) noexcept
	{
		return Alignment != 0 && (Alignment & (Alignment - 1)) == 0;
	}
};

} // namespace Enigma
