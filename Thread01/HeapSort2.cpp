#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "HeapSort2.h"

struct Profiler {
    // record one entry per profiled siftDown call (sampled)
    std::vector<long long> sift_ns; // nanoseconds per call
    std::uint64_t calls_total = 0;
    std::uint32_t sample_rate = 1;  // 1 = record all, 10 = record every 10th call, etc.

    explicit Profiler(std::uint32_t rate = 1) : sample_rate(rate ? rate : 1) {
        sift_ns.reserve(1'000'000); // heuristic; avoids some reallocs for mid-size runs
    }

    inline void record(long long ns) {
        ++calls_total;
        if (calls_total % sample_rate == 0) sift_ns.push_back(ns);
    }
};

template <class T, class Compare>
inline void siftDown(std::vector<T>& a, std::size_t start, std::size_t end, Compare comp, Profiler* prof = nullptr) {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point t0;
    if (prof) t0 = clock::now();

    std::size_t root = start;
    while (true) {
        std::size_t left = 2 * root + 1;
        if (left >= end) break;                // no children
        std::size_t swapIdx = root;
        if (comp(a[swapIdx], a[left])) swapIdx = left;
        std::size_t right = left + 1;
        if (right < end&& comp(a[swapIdx], a[right])) swapIdx = right;
        if (swapIdx == root) break;
        std::swap(a[root], a[swapIdx]);
        root = swapIdx;
    }

    if (prof) {
        auto t1 = clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        prof->record(ns);
    }
}

template <class T, class Compare>
inline void buildHeap(std::vector<T>& a, Compare comp, Profiler* prof = nullptr) {
    if (a.empty()) return;
    for (std::size_t i = (a.size() - 2) / 2 + 1; i-- > 0; ) {
        siftDown(a, i, a.size(), comp, prof);
    }
}

/**
 * heapsort:
 *  - ascending = true  => build a MAX-heap (comp = std::less<T>)
 *  - ascending = false => build a MIN-heap (comp = std::greater<T>)
 *  - If profiler != nullptr, each siftDown call is timed (sampled).
 */
template <class T>
inline void heapSort(std::vector<T>& a, bool ascending = true, Profiler* profiler = nullptr) {
    if (ascending) {
        auto comp = std::less<T>{};   // MAX-heap for ascending result
        buildHeap(a, comp, profiler);
        for (std::size_t end = a.size(); end > 1; --end) {
            std::swap(a[0], a[end - 1]);
            siftDown(a, 0, end - 1, comp, profiler);
        }
    }
    else {
        auto comp = std::greater<T>{}; // MIN-heap for descending result
        buildHeap(a, comp, profiler);
        for (std::size_t end = a.size(); end > 1; --end) {
            std::swap(a[0], a[end - 1]);
            siftDown(a, 0, end - 1, comp, profiler);
        }
    }
}

// ---- Helpers ----------------------------------------------------------------

template <class T>
bool isSortedAsc(const std::vector<T>& v) {
    for (std::size_t i = 1; i < v.size(); ++i) if (v[i] < v[i - 1]) return false;
    return true;
}
template <class T>
bool isSortedDesc(const std::vector<T>& v) {
    for (std::size_t i = 1; i < v.size(); ++i) if (v[i] > v[i - 1]) return false;
    return true;
}

template <class F>
auto timeItMs(F&& f) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    f();
    const auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// Compute percentile from a copy via nth_element (does not fully sort).
double percentile_ns(const std::vector<long long>& data, double p) {
    if (data.empty()) return 0.0;
    if (p <= 0) return static_cast<double>(*std::min_element(data.begin(), data.end()));
    if (p >= 1) return static_cast<double>(*std::max_element(data.begin(), data.end()));
    std::vector<long long> copy = data;
    std::size_t idx = static_cast<std::size_t>(p * (copy.size() - 1));
    std::nth_element(copy.begin(), copy.begin() + idx, copy.end());
    return static_cast<double>(copy[idx]);
}

void printProfilerSummary(const Profiler& prof, const char* label) {
    const auto& v = prof.sift_ns;
    long long total_ns = 0;
    for (auto ns : v) total_ns += ns;

    std::cout << "[Profiler] " << label << "\n"
        << "  siftDown calls total: " << prof.calls_total
        << " (sampled=" << v.size() << ", sample_rate=" << prof.sample_rate << ")\n"
        << "  total (sampled)   : " << std::fixed << std::setprecision(3)
        << (total_ns / 1e6) << " ms\n";
    if (!v.empty()) {
        double avg = (static_cast<double>(total_ns) / v.size());
        double p50 = percentile_ns(v, 0.50);
        double p90 = percentile_ns(v, 0.90);
        double p99 = percentile_ns(v, 0.99);
        auto [mn_it, mx_it] = std::minmax_element(v.begin(), v.end());
        std::cout << "  avg ns/call       : " << avg << "\n"
            << "  p50 / p90 / p99   : " << p50 << " / " << p90 << " / " << p99 << " ns\n"
            << "  min / max         : " << *mn_it << " / " << *mx_it << " ns\n\n";
    }
    else {
        std::cout << "  (no samples recorded)\n\n";
    }
}

// ---- Demo / Benchmarks ------------------------------------------------------

int heap_sort_2_main() {
    // 1) Sample of 10 elements
    std::vector<int> sample = { 42, -7, 19, 0, 5, 5, 123, -100, 8, 8 };
    std::cout << "Sample (10 elems), original: ";
    for (auto x : sample) std::cout << x << ' ';
    std::cout << "\n";

    // Profile small sample with full capture
    Profiler profSmall(/*sample_rate=*/1);
    auto a1 = sample;
    heapSort(a1, /*ascending=*/true, &profSmall);
    std::cout << "Sorted ascending: ";
    for (auto x : a1) std::cout << x << ' ';
    std::cout << "(ok=" << std::boolalpha << isSortedAsc(a1) << ")\n";
    printProfilerSummary(profSmall, "Sample ascending");

    auto a2 = sample;
    Profiler profSmallDesc(/*sample_rate=*/1);
    heapSort(a2, /*ascending=*/false, &profSmallDesc);
    std::cout << "Sorted descending: ";
    for (auto x : a2) std::cout << x << ' ';
    std::cout << "(ok=" << std::boolalpha << isSortedDesc(a2) << ")\n\n";
    printProfilerSummary(profSmallDesc, "Sample descending");

    // 2) Randomized input generator demo
    auto rndSmall = generateRandomVector(20, -1000, 1000, /*seed=*/12345);
    heapSort(rndSmall, true);
    std::cout << "Random 20 elems (seed=12345) sorted ascending ok? "
        << std::boolalpha << isSortedAsc(rndSmall) << "\n\n";

    // 3) Benchmarks: 100,000 and 10,000,000
    struct Bench { std::size_t n; std::uint64_t seed; std::uint32_t sample_rate; };
    std::vector<Bench> benches = {
        {100'000,    2025, 1},     // record all
        {10'000'000, 2026, 50}     // sample every 50th call to keep memory in check
    };

    for (const auto& b : benches) {
        std::cout << "---- n=" << b.n << " ----\n";
        auto vecAsc = generateRandomVector(b.n, -1'000'000, 1'000'000, b.seed);
        auto vecDesc = vecAsc; // reuse same data

        Profiler profAsc(b.sample_rate);
        auto tAsc = timeItMs([&] { heapSort(vecAsc, true, &profAsc); });
        std::cout << "Ascending heapsort: " << std::fixed << std::setprecision(2) << tAsc << " ms"
            << " (ok=" << isSortedAsc(vecAsc) << ")\n";
        printProfilerSummary(profAsc, "Ascending");

        Profiler profDesc(b.sample_rate);
        auto tDesc = timeItMs([&] { heapSort(vecDesc, false, &profDesc); });
        std::cout << "Descending heapsort: " << std::fixed << std::setprecision(2) << tDesc << " ms"
            << " (ok=" << isSortedDesc(vecDesc) << ")\n";
        printProfilerSummary(profDesc, "Descending");
    }

    return 0;
}
