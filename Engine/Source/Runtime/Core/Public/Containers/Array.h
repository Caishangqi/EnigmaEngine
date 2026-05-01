// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Containers/ContainerAllocationPolicies.h"
#include "Misc/AssertionMacros.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace Enigma
{

/// Owning dynamic contiguous array with UE-style capacity semantics.
///
/// Phase 1 owns raw storage in [Data, Data + ArrayMax) and constructed elements in
/// [Data, Data + ArrayNum). Reserve is O(N) when capacity grows and O(1) otherwise.
/// Add and Emplace grow capacity by deterministic doubling when storage is full.
/// Reset destroys constructed elements but keeps storage; Empty destroys elements and
/// releases storage. Element constructors and destructors are not caught by TArray.
template <typename InElementType, typename InAllocatorType = FDefaultAllocator>
class TArray
{
	static_assert(!std::is_const_v<InElementType>, "TArray element type must not be const.");

public:
	using ElementType = InElementType;
	using AllocatorType = InAllocatorType;
	using SizeType = int32_t;
	using Iterator = ElementType*;
	using ConstIterator = const ElementType*;
	using ReverseIterator = std::reverse_iterator<Iterator>;
	using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

	TArray() noexcept = default;

	TArray(std::initializer_list<ElementType> InitialValues)
	{
		Reserve(CheckedSize(InitialValues.size()));

		for (const ElementType& Value : InitialValues)
		{
			std::construct_at(Data + ArrayNum, Value);
			++ArrayNum;
		}
	}

	TArray(const TArray& Other)
	{
		CopyFrom(Other);
	}

	TArray(TArray&& Other) noexcept
		: Data(Other.Data)
		, ArrayNum(Other.ArrayNum)
		, ArrayMax(Other.ArrayMax)
	{
		Other.Data = nullptr;
		Other.ArrayNum = 0;
		Other.ArrayMax = 0;
	}

	~TArray()
	{
		Empty();
	}

	TArray& operator=(const TArray& Other)
	{
		if (this != &Other)
		{
			TArray Copy(Other);
			Swap(Copy);
		}

		return *this;
	}

	TArray& operator=(TArray&& Other) noexcept
	{
		if (this != &Other)
		{
			Empty();

			Data = Other.Data;
			ArrayNum = Other.ArrayNum;
			ArrayMax = Other.ArrayMax;

			Other.Data = nullptr;
			Other.ArrayNum = 0;
			Other.ArrayMax = 0;
		}

		return *this;
	}

	/// Returns the number of constructed elements.
	[[nodiscard]] SizeType Num() const noexcept
	{
		return ArrayNum;
	}

	/// Returns the current element capacity.
	[[nodiscard]] SizeType Max() const noexcept
	{
		return ArrayMax;
	}

	[[nodiscard]] bool IsEmpty() const noexcept
	{
		return ArrayNum == 0;
	}

	[[nodiscard]] bool IsValidIndex(SizeType Index) const noexcept
	{
		return Index >= 0 && Index < ArrayNum;
	}

	/// Returns contiguous element storage, or nullptr when no storage is allocated.
	[[nodiscard]] ElementType* GetData() noexcept
	{
		return Data;
	}

	/// Returns contiguous element storage, or nullptr when no storage is allocated.
	[[nodiscard]] const ElementType* GetData() const noexcept
	{
		return Data;
	}

	[[nodiscard]] ElementType& operator[](SizeType Index)
	{
		RangeCheck(Index);
		return Data[Index];
	}

	[[nodiscard]] const ElementType& operator[](SizeType Index) const
	{
		RangeCheck(Index);
		return Data[Index];
	}

	[[nodiscard]] ElementType& Last()
	{
		const SizeType Index = ArrayNum - 1;
		RangeCheck(Index);
		return Data[Index];
	}

	[[nodiscard]] const ElementType& Last() const
	{
		const SizeType Index = ArrayNum - 1;
		RangeCheck(Index);
		return Data[Index];
	}

	/// Ensures capacity for at least NewCapacity elements. Existing elements are moved.
	void Reserve(SizeType NewCapacity)
	{
		checkf(NewCapacity >= 0, "Array capacity must be non-negative: {}", NewCapacity);

		if (NewCapacity <= ArrayMax)
		{
			return;
		}

		Reallocate(NewCapacity);
	}

	SizeType Add(const ElementType& Item)
	{
		return Emplace(Item);
	}

	SizeType Add(ElementType&& Item)
	{
		return Emplace(std::move(Item));
	}

	template <typename... ArgsType>
	SizeType Emplace(ArgsType&&... Args)
	{
		const SizeType NewIndex = ArrayNum;
		EnsureCapacityFor(CheckedAdd(ArrayNum, 1));
		std::construct_at(Data + NewIndex, std::forward<ArgsType>(Args)...);
		++ArrayNum;
		return NewIndex;
	}

	void Append(std::initializer_list<ElementType> Items)
	{
		AppendData(Items.begin(), CheckedSize(Items.size()));
	}

	void Append(const TArray& Other)
	{
		if (this == &Other)
		{
			TArray Copy(Other);
			Append(Copy);
			return;
		}

		AppendData(Other.Data, Other.ArrayNum);
	}

	ElementType Pop()
	{
		const SizeType Index = ArrayNum - 1;
		RangeCheck(Index);

		ElementType Result(std::move(Data[Index]));
		std::destroy_at(Data + Index);
		--ArrayNum;
		return Result;
	}

	void RemoveAt(SizeType Index, SizeType Count = 1)
	{
		RangeCheck(Index, Count);
		if (Count == 0)
		{
			return;
		}

		const SizeType MoveCount = ArrayNum - Index - Count;
		DestroyElements(Data + Index, Count);

		for (SizeType MoveIndex = 0; MoveIndex < MoveCount; ++MoveIndex)
		{
			ElementType* Destination = Data + Index + MoveIndex;
			ElementType* Source = Data + Index + Count + MoveIndex;
			std::construct_at(Destination, std::move(*Source));
			std::destroy_at(Source);
		}

		ArrayNum -= Count;
	}

	void RemoveAtSwap(SizeType Index, SizeType Count = 1)
	{
		RangeCheck(Index, Count);
		if (Count == 0)
		{
			return;
		}

		const SizeType TailCount = ArrayNum - Index - Count;
		const SizeType MoveCount = Count < TailCount ? Count : TailCount;
		DestroyElements(Data + Index, Count);

		for (SizeType MoveIndex = 0; MoveIndex < MoveCount; ++MoveIndex)
		{
			const SizeType SourceIndex = ArrayNum - MoveCount + MoveIndex;
			ElementType* Destination = Data + Index + MoveIndex;
			ElementType* Source = Data + SourceIndex;
			std::construct_at(Destination, std::move(*Source));
			std::destroy_at(Source);
		}

		ArrayNum -= Count;
	}

	[[nodiscard]] Iterator begin() noexcept
	{
		return Data;
	}

	[[nodiscard]] ConstIterator begin() const noexcept
	{
		return Data;
	}

	[[nodiscard]] ConstIterator cbegin() const noexcept
	{
		return Data;
	}

	[[nodiscard]] Iterator end() noexcept
	{
		return EndPointer();
	}

	[[nodiscard]] ConstIterator end() const noexcept
	{
		return EndPointer();
	}

	[[nodiscard]] ConstIterator cend() const noexcept
	{
		return EndPointer();
	}

	[[nodiscard]] ReverseIterator rbegin() noexcept
	{
		return ReverseIterator(end());
	}

	[[nodiscard]] ConstReverseIterator rbegin() const noexcept
	{
		return ConstReverseIterator(end());
	}

	[[nodiscard]] ConstReverseIterator crbegin() const noexcept
	{
		return ConstReverseIterator(cend());
	}

	[[nodiscard]] ReverseIterator rend() noexcept
	{
		return ReverseIterator(begin());
	}

	[[nodiscard]] ConstReverseIterator rend() const noexcept
	{
		return ConstReverseIterator(begin());
	}

	[[nodiscard]] ConstReverseIterator crend() const noexcept
	{
		return ConstReverseIterator(cbegin());
	}

	/// Destroys constructed elements while preserving allocated storage.
	void Reset()
	{
		DestroyElements(Data, ArrayNum);
		ArrayNum = 0;
	}

	/// Destroys constructed elements and releases all owned storage.
	void Empty()
	{
		Reset();
		AllocatorType::Free(Data, alignof(ElementType));
		Data = nullptr;
		ArrayMax = 0;
	}

private:
	ElementType* Data = nullptr;
	SizeType ArrayNum = 0;
	SizeType ArrayMax = 0;

	[[nodiscard]] bool IsValidRange(SizeType Index, SizeType Count) const noexcept
	{
		return Index >= 0
			&& Count >= 0
			&& Index <= ArrayNum
			&& Count <= ArrayNum - Index;
	}

	void RangeCheck(SizeType Index) const
	{
		checkf(
			IsValidIndex(Index),
			"Array index out of bounds: {} into an array of size {}",
			Index,
			ArrayNum);
	}

	void RangeCheck(SizeType Index, SizeType Count) const
	{
		checkf(
			IsValidRange(Index, Count),
			"Array range out of bounds: index {} and length {} into an array of size {}",
			Index,
			Count,
			ArrayNum);
	}

	static SizeType CheckedSize(std::size_t Value)
	{
		checkf(
			Value <= static_cast<std::size_t>(std::numeric_limits<SizeType>::max()),
			"Array size out of range: {}",
			Value);
		return static_cast<SizeType>(Value);
	}

	static SizeType CheckedAdd(SizeType Left, SizeType Right)
	{
		checkf(Right >= 0, "Array count must be non-negative: {}", Right);
		checkf(
			Left <= std::numeric_limits<SizeType>::max() - Right,
			"Array size out of range: {} + {}",
			Left,
			Right);
		return Left + Right;
	}

	static std::size_t AllocationSize(SizeType Capacity)
	{
		checkf(Capacity >= 0, "Array capacity must be non-negative: {}", Capacity);

		const std::size_t ElementCount = static_cast<std::size_t>(Capacity);
		checkf(
			ElementCount <= std::numeric_limits<std::size_t>::max() / sizeof(ElementType),
			"Array allocation size overflow: {} elements of {} bytes",
			ElementCount,
			sizeof(ElementType));
		return ElementCount * sizeof(ElementType);
	}

	static void DestroyElements(ElementType* Elements, SizeType Count)
	{
		for (SizeType Index = 0; Index < Count; ++Index)
		{
			std::destroy_at(Elements + Index);
		}
	}

	static void CopyConstructElements(ElementType* Destination, const ElementType* Source, SizeType Count)
	{
		for (SizeType Index = 0; Index < Count; ++Index)
		{
			std::construct_at(Destination + Index, Source[Index]);
		}
	}

	static void MoveConstructElements(ElementType* Destination, ElementType* Source, SizeType Count)
	{
		for (SizeType Index = 0; Index < Count; ++Index)
		{
			std::construct_at(Destination + Index, std::move(Source[Index]));
		}
	}

	void EnsureCapacityFor(SizeType DesiredCapacity)
	{
		if (DesiredCapacity <= ArrayMax)
		{
			return;
		}

		Reallocate(CalculateGrowCapacity(DesiredCapacity));
	}

	SizeType CalculateGrowCapacity(SizeType DesiredCapacity) const
	{
		SizeType NewCapacity = ArrayMax > 0 ? ArrayMax : 1;

		while (NewCapacity < DesiredCapacity)
		{
			checkf(
				NewCapacity <= std::numeric_limits<SizeType>::max() / 2,
				"Array capacity out of range: {}",
				DesiredCapacity);
			NewCapacity *= 2;
		}

		return NewCapacity;
	}

	void AppendData(const ElementType* Items, SizeType Count)
	{
		if (Count == 0)
		{
			return;
		}

		EnsureCapacityFor(CheckedAdd(ArrayNum, Count));

		for (SizeType Index = 0; Index < Count; ++Index)
		{
			std::construct_at(Data + ArrayNum, Items[Index]);
			++ArrayNum;
		}
	}

	void CopyFrom(const TArray& Other)
	{
		Reserve(Other.ArrayNum);
		CopyConstructElements(Data, Other.Data, Other.ArrayNum);
		ArrayNum = Other.ArrayNum;
	}

	void Reallocate(SizeType NewCapacity)
	{
		ElementType* NewData = static_cast<ElementType*>(
			AllocatorType::Allocate(AllocationSize(NewCapacity), alignof(ElementType)));

		MoveConstructElements(NewData, Data, ArrayNum);
		DestroyElements(Data, ArrayNum);
		AllocatorType::Free(Data, alignof(ElementType));

		Data = NewData;
		ArrayMax = NewCapacity;
	}

	void Swap(TArray& Other) noexcept
	{
		using std::swap;
		swap(Data, Other.Data);
		swap(ArrayNum, Other.ArrayNum);
		swap(ArrayMax, Other.ArrayMax);
	}

	[[nodiscard]] ElementType* EndPointer() noexcept
	{
		return Data != nullptr ? Data + ArrayNum : nullptr;
	}

	[[nodiscard]] const ElementType* EndPointer() const noexcept
	{
		return Data != nullptr ? Data + ArrayNum : nullptr;
	}
};

} // namespace Enigma
