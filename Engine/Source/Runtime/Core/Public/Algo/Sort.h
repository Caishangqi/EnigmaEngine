// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <algorithm>
#include <iterator>

namespace Enigma::Algo
{

/// Sorts a random-access range with std::sort. Phase 1 does not implement a custom Enigma sort algorithm.
template <typename RangeType>
void Sort(RangeType&& Range)
{
	using std::begin;
	using std::end;

	std::sort(begin(Range), end(Range));
}

/// Sorts a random-access range with std::sort and a caller-provided comparator.
template <typename RangeType, typename PredicateType>
void Sort(RangeType&& Range, PredicateType Predicate)
{
	using std::begin;
	using std::end;

	std::sort(begin(Range), end(Range), Predicate);
}

} // namespace Enigma::Algo
