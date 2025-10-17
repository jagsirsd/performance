#include "HeapSortUtil.h"

#ifndef File1H
#define File1H

std::vector<int> generateRandomVector(std::size_t n,
	int low = std::numeric_limits<int>::min() / 2,
	int high = std::numeric_limits<int>::max() / 2,
	std::uint64_t seed = 0) {
	std::mt19937_64 rng(seed ? seed : std::random_device{}());
	std::uniform_int_distribution<int> dist(low, high);
	std::vector<int> v;
	v.reserve(n);
	for (std::size_t i = 0; i < n; ++i)
		v.push_back(dist(rng));
	return v;
}

#endif