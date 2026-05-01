// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Misc/AssertionMacros.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>

namespace Enigma
{

/// Non-owning contiguous array view. It never extends the referenced data lifetime.
template <typename InElementType>
class TArrayView
{
public:
	using ElementType = InElementType;
	using SizeType = int32_t;
	using Iterator = ElementType*;
	using ConstIterator = const ElementType*;
	using ReverseIterator = std::reverse_iterator<Iterator>;
	using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

	TArrayView() noexcept = default;

	TArrayView(ElementType* InData, SizeType InCount)
		: Data(InData)
		, ArrayNum(InCount)
	{
		checkf(InCount >= 0, "Array view size must be non-negative: {}", InCount);
		checkf(InData != nullptr || InCount == 0, "Array view data must not be null for {} elements", InCount);
	}

	template <std::size_t Count>
	TArrayView(ElementType (&InData)[Count])
		: Data(InData)
		, ArrayNum(CheckedSize(Count))
	{
	}

	template <typename AllocatorType>
		requires(!std::is_const_v<ElementType>)
	TArrayView(TArray<ElementType, AllocatorType>& Array)
		: Data(Array.GetData())
		, ArrayNum(Array.Num())
	{
	}

	template <typename AllocatorType>
		requires(std::is_const_v<ElementType>)
	TArrayView(const TArray<std::remove_const_t<ElementType>, AllocatorType>& Array)
		: Data(Array.GetData())
		, ArrayNum(Array.Num())
	{
	}

	[[nodiscard]] SizeType Num() const noexcept
	{
		return ArrayNum;
	}

	[[nodiscard]] bool IsEmpty() const noexcept
	{
		return ArrayNum == 0;
	}

	[[nodiscard]] bool IsValidIndex(SizeType Index) const noexcept
	{
		return Index >= 0 && Index < ArrayNum;
	}

	[[nodiscard]] ElementType* GetData() const noexcept
	{
		return Data;
	}

	[[nodiscard]] ElementType& operator[](SizeType Index) const
	{
		RangeCheck(Index);
		return Data[Index];
	}

	[[nodiscard]] Iterator begin() const noexcept
	{
		return Data;
	}

	[[nodiscard]] Iterator end() const noexcept
	{
		return EndPointer();
	}

	[[nodiscard]] ReverseIterator rbegin() const noexcept
	{
		return ReverseIterator(end());
	}

	[[nodiscard]] ReverseIterator rend() const noexcept
	{
		return ReverseIterator(begin());
	}

private:
	ElementType* Data = nullptr;
	SizeType ArrayNum = 0;

	static SizeType CheckedSize(std::size_t Value)
	{
		checkf(
			Value <= static_cast<std::size_t>(std::numeric_limits<SizeType>::max()),
			"Array view size out of range: {}",
			Value);
		return static_cast<SizeType>(Value);
	}

	void RangeCheck(SizeType Index) const
	{
		checkf(
			IsValidIndex(Index),
			"Array index out of bounds: {} from an array of size {}",
			Index,
			ArrayNum);
	}

	[[nodiscard]] ElementType* EndPointer() const noexcept
	{
		return Data != nullptr ? Data + ArrayNum : nullptr;
	}
};

template <typename ElementType>
using TConstArrayView = TArrayView<const ElementType>;

} // namespace Enigma
