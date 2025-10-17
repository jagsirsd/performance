#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "HeapSort.h"

// ---- Heap utilities ---------------------------------------------------------

template <class T, class Compare>
inline void siftDown(std::vector<T>& a, std::size_t start, std::size_t end, Compare comp) {
	// Iterative siftDown for 0-based indexing, heap is in [0, end)
	std::size_t root = start;
	while (true) {
		std::size_t left = 2 * root + 1;
		if (left >= end) 
			break;                // no children
		
		std::size_t swapIdx = root;
		if (comp(a[swapIdx], a[left])) 
			swapIdx = left; // compare decides heap type
		
		std::size_t right = left + 1;
		if (right < end && comp(a[swapIdx], a[right])) 
			swapIdx = right;
		if (swapIdx == root) 
			break;
		std::swap(a[root], a[swapIdx]);
	
		root = swapIdx;
	}
}

template <class T, class Compare>
inline void buildHeap(std::vector<T>& a, Compare comp) {
	// Heapify in O(n)
	if (a.empty()) return;
	for (std::size_t i = (a.size() - 2) / 2 + 0; i < a.size(); /* wrap-safe */) {
		siftDown(a, i, a.size(), comp);
		if (i == 0) break;
		--i;
	}
}

/**
 * heapsort:
 *  - ascending = true  => build a MAX-heap (comp = std::less<T>)
 *  - ascending = false => build a MIN-heap (comp = std::greater<T>)
 */
template <class T>
inline void heapSort(std::vector<T>& a, bool ascending = true) {
	if (ascending) {
		auto comp = std::less<T>{};   // MAX-heap for ascending result
		buildHeap(a, comp);
		for (std::size_t end = a.size(); end > 1; --end) {
			std::swap(a[0], a[end - 1]);
			siftDown(a, 0, end - 1, comp);
		}
	}
	else {
		auto comp = std::greater<T>{}; // MIN-heap for descending result
		buildHeap(a, comp);
		for (std::size_t end = a.size(); end > 1; --end) {
			std::swap(a[0], a[end - 1]);
			siftDown(a, 0, end - 1, comp);
		}
	}
}

// ---- Helpers ----------------------------------------------------------------

template <class T>
bool isSortedAsc(const std::vector<T>& v) {
	for (std::size_t i = 1; i < v.size(); ++i) 
		if (v[i] < v[i - 1]) 
			return false;
	return true;
}
template <class T>
bool isSortedDesc(const std::vector<T>& v) {
	for (std::size_t i = 1; i < v.size(); ++i)
		if (v[i] > v[i - 1])
			return false;
	return true;
}



template <class F>
auto timeItMs(F&& f) {
	const auto t0 = std::chrono::high_resolution_clock::now();
	f();
	const auto t1 = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// ---- Demo / Benchmarks ------------------------------------------------------

int heap_sort_main() {
	// 1) Sample of 10 elements
	std::vector<int> sample = { 42, -7, 19, 0, 5, 5, 123, -100, 8, 8 };
	std::cout << "Sample (10 elems), original: ";
	for (auto x : sample) std::cout << x << ' ';
	std::cout << "\n";

	auto a1 = sample;
	heapSort(a1, /*ascending=*/true);
	std::cout << "Sorted ascending: ";
	
	for (auto x : a1) 
		std::cout << x << ' ';
	
	std::cout << "(ok=" << std::boolalpha << isSortedAsc(a1) << ")\n";

	auto a2 = sample;
	heapSort(a2, /*ascending=*/false);
	std::cout << "Sorted descending: ";
	for (auto x : a2)
		std::cout << x << ' ';
	std::cout << "(ok=" << std::boolalpha << isSortedDesc(a2) << ")\n\n";

	// 2) Randomized input generator demo
	auto rndSmall = generateRandomVector(20, -1000, 1000, /*seed=*/12345);
	std::cout << "Random 20 elems (seed=12345) before/after ascending sort:\n";
	heapSort(rndSmall, true);
	std::cout << "ok=" << isSortedAsc(rndSmall) << "\n\n";

	// 3) Benchmarks: 100,000 and 10,000,000
	struct Bench { 
		std::size_t n; 
		std::uint64_t seed; 
	};
	
	std::vector<Bench> benches = {
		{100'000,    2025},
		{10'000'000, 2026}
	};

	for (const auto& b : benches) {
		std::cout << "---- n=" << b.n << " ----\n";
		auto vecAsc = generateRandomVector(b.n, -1'000'000, 1'000'000, b.seed);
		auto vecDesc = vecAsc; // reuse same data

		auto tAsc = timeItMs([&] { heapSort(vecAsc, true); });
		std::cout << "Ascending heapsort: " << std::fixed << std::setprecision(2) << tAsc << " ms"
			<< " (ok=" << isSortedAsc(vecAsc) << ")\n";

		auto tDesc = timeItMs([&] { heapSort(vecDesc, false); });
		std::cout << "Descending heapsort: " << std::fixed << std::setprecision(2) << tDesc << " ms"
			<< " (ok=" << isSortedDesc(vecDesc) << ")\n\n";
	}

	return 0;
}
