#pragma once
#ifndef UNORDERED_SET_TYPE
#define UNORDERED_SET_TYPE 0
#endif // UNORDERED_SET_TYPE

#if UNORDERED_SET_TYPE == 1
#include "Utility/flat_hash_map.hpp"
#else
#include <unordered_set>
#endif

namespace pf
{
	template<typename T>
#if UNORDERED_SET_TYPE == 1
	using unordered_set = ska::flat_hash_set<T>;
#else
	using unordered_set = std::unordered_set<T>;
#endif // WI_UNORDERED_SET_TYPE
}
