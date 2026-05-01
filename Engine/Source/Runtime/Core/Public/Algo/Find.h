// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <cstdint>
#include <iterator>
#include <memory>

namespace Enigma::Algo
{

template <typename RangeType, typename ValueType>
[[nodiscard]] auto Find(RangeType&& Range, const ValueType& Value)
{
	using std::begin;
	using std::end;
	using PointerType = decltype(std::addressof(*begin(Range)));

	for (auto It = begin(Range); It != end(Range); ++It)
	{
		if (*It == Value)
		{
			return std::addressof(*It);
		}
	}

	return static_cast<PointerType>(nullptr);
}

template <typename RangeType, typename PredicateType>
[[nodiscard]] auto FindByPredicate(RangeType&& Range, PredicateType Predicate)
{
	using std::begin;
	using std::end;
	using PointerType = decltype(std::addressof(*begin(Range)));

	for (auto It = begin(Range); It != end(Range); ++It)
	{
		if (Predicate(*It))
		{
			return std::addressof(*It);
		}
	}

	return static_cast<PointerType>(nullptr);
}

template <typename RangeType, typename ValueType>
[[nodiscard]] bool Contains(RangeType&& Range, const ValueType& Value)
{
	return Find(Range, Value) != nullptr;
}

template <typename RangeType, typename PredicateType>
[[nodiscard]] bool ContainsByPredicate(RangeType&& Range, PredicateType Predicate)
{
	return FindByPredicate(Range, Predicate) != nullptr;
}

template <typename RangeType, typename ValueType>
[[nodiscard]] int32_t IndexOf(RangeType&& Range, const ValueType& Value)
{
	using std::begin;
	using std::end;

	int32_t Index = 0;
	for (auto It = begin(Range); It != end(Range); ++It, ++Index)
	{
		if (*It == Value)
		{
			return Index;
		}
	}

	return -1;
}

template <typename RangeType, typename PredicateType>
[[nodiscard]] int32_t IndexOfByPredicate(RangeType&& Range, PredicateType Predicate)
{
	using std::begin;
	using std::end;

	int32_t Index = 0;
	for (auto It = begin(Range); It != end(Range); ++It, ++Index)
	{
		if (Predicate(*It))
		{
			return Index;
		}
	}

	return -1;
}

} // namespace Enigma::Algo
