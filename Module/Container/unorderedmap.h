#pragma once
#ifndef UNORDERED_SET_TYPE
#define UNORDERED_SET_TYPE 0
#endif // UNORDERED_SET_TYPE

#if UNORDERED_MAP_TYPE == 1
#include "Utility/flat_hash_map.hpp"
#elif UNORDERED_MAP_TYPE == 2
#include "Utility/robin_hood.h"
#else
#include <unordered_map>
#endif // UNORDERED_MAP_TYPE

namespace pf
{
	template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<const K, V> > >
#if UNORDERED_MAP_TYPE == 1
	using unordered_map = ska::flat_hash_map<K, V, H, E, A>;
#elif UNORDERED_MAP_TYPE == 2
	using unordered_map = robin_hood::unordered_flat_map<K, V, H, E>;
#else
	using unordered_map = std::unordered_map<K, V, H, E, A>;
#endif // UNORDERED_MAP_TYPE
}
