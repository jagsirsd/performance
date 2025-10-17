#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

// -------------------- Profiler --------------------
struct Profiler {
    std::vector<long long> sift_ns;    // ns per (sampled) siftDown call
    std::uint64_t calls_total = 0;
    std::uint32_t sample_rate = 1;     // 1 = record every call; 10 = every 10th, etc.

    explicit Profiler(std::uint32_t rate = 1) : sample_rate(rate ? rate : 1) {
        sift_ns.reserve(1'000'000);
    }
    inline void record(long long ns) {
        ++calls_total;
        if (calls_total % sample_rate == 0) sift_ns.push_back(ns);
    }
};

// -------------------- Heap internals (inline compare) --------------------
template <bool MaxHeap, class T>
inline bool shouldSwap(const T& parent, const T& child) {
    if constexpr (MaxHeap) return parent < child;  // max-heap: bubble larger up
    else                    return parent > child; // min-heap: bubble smaller up
}

template <bool MaxHeap, class T>
inline void siftDown(std::vector<T>& a, std::size_t start, std::size_t end, Profiler* prof = nullptr) {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point t0;
    if (prof) t0 = clock::now();

    std::size_t root = start;
    while (true) {
        std::size_t left = 2 * root + 1;
        if (left >= end) break; // no children

        std::size_t swapIdx = root;
        if (shouldSwap<MaxHeap>(a[swapIdx], a[left])) swapIdx = left;

        std::size_t right = left + 1;
        if (right < end&& shouldSwap<MaxHeap>(a[swapIdx], a[right])) swapIdx = right;

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

template <bool MaxHeap, class T>
inline void buildHeap(std::vector<T>& a, Profiler* prof = nullptr) {
    if (a.empty()) return;
    // start at last parent and sift down
    for (std::size_t i = (a.size() - 2) / 2 + 1; i-- > 0; ) {
        siftDown<MaxHeap>(a, i, a.size(), prof);
    }
}

// -------------------- Public API --------------------
// Ascending sort uses a MAX-heap
template <class T>
inline void heapSortAscending(std::vector<T>& a, Profiler* profiler = nullptr) {
    buildHeap<true>(a, profiler);
    for (std::size_t end = a.size(); end > 1; --end) {
        std::swap(a[0], a[end - 1]);
        siftDown<true>(a, 0, end - 1, profiler);
    }
}

// Descending sort uses a MIN-heap
template <class T>
inline void heapSortDescending(std::vector<T>& a, Profiler* profiler = nullptr) {
    buildHeap<false>(a, profiler);
    for (std::size_t end = a.size(); end > 1; --end) {
        std::swap(a[0], a[end - 1]);
        siftDown<false>(a, 0, end - 1, profiler);
    }
}

// -------------------- Helpers --------------------
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

std::vector<int> generateRandomVector3(std::size_t n,
    int low = std::numeric_limits<int>::min() / 2,
    int high = std::numeric_limits<int>::max() / 2,
    std::uint64_t seed = 0) {
    std::mt19937_64 rng(seed ? seed : std::random_device{}());
    std::uniform_int_distribution<int> dist(low, high);
    std::vector<int> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) v.push_back(dist(rng));
    return v;
}

template <class F>
auto timeItMs(F&& f) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    f();
    const auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double percentile_ns2(const std::vector<long long>& data, double p) {
    if (data.empty()) return 0.0;
    if (p <= 0) return static_cast<double>(*std::min_element(data.begin(), data.end()));
    if (p >= 1) return static_cast<double>(*std::max_element(data.begin(), data.end()));
    std::vector<long long> copy = data;
    std::size_t idx = static_cast<std::size_t>(p * (copy.size() - 1));
    std::nth_element(copy.begin(), copy.begin() + idx, copy.end());
    return static_cast<double>(copy[idx]);
}

void printProfilerSummary2(const Profiler& prof, const char* label) {
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
        double p50 = percentile_ns2(v, 0.50);
        double p90 = percentile_ns2(v, 0.90);
        double p99 = percentile_ns2(v, 0.99);
        auto [mn_it, mx_it] = std::minmax_element(v.begin(), v.end());
        std::cout << "  avg ns/call       : " << avg << "\n"
            << "  p50 / p90 / p99   : " << p50 << " / " << p90 << " / " << p99 << " ns\n"
            << "  min / max         : " << *mn_it << " / " << *mx_it << " ns\n\n";
    }
    else {
        std::cout << "  (no samples recorded)\n\n";
    }
}

// -------------------- Demo / Benchmarks --------------------
int heap_sort_inline_comparison_main() {
    // 1) Sample of 10 elements
    std::vector<int> sample = { 42, -7, 19, 0, 5, 5, 123, -100, 8, 8 };
    std::cout << "Sample (10 elems), original: ";
    for (auto x : sample) std::cout << x << ' ';
    std::cout << "\n";

    // Profile small sample with full capture
    Profiler profSmall(1);
    auto a1 = sample;
    heapSortAscending(a1, &profSmall);
    std::cout << "Sorted ascending: ";
    for (auto x : a1) std::cout << x << ' ';
    std::cout << "(ok=" << std::boolalpha << isSortedAsc(a1) << ")\n";
    printProfilerSummary2(profSmall, "Sample ascending");

    auto a2 = sample;
    Profiler profSmallDesc(1);
    heapSortDescending(a2, &profSmallDesc);
    std::cout << "Sorted descending: ";
    for (auto x : a2) std::cout << x << ' ';
    std::cout << "(ok=" << std::boolalpha << isSortedDesc(a2) << ")\n\n";
    printProfilerSummary2(profSmallDesc, "Sample descending");

    // 2) Randomized input generator demo
    auto rndSmall = generateRandomVector3(20, -1000, 1000, /*seed=*/12345);
    heapSortAscending(rndSmall);
    std::cout << "Random 20 elems (seed=12345) sorted ascending ok? "
        << std::boolalpha << isSortedAsc(rndSmall) << "\n\n";

    // 3) Benchmarks: 100,000 and 10,000,000
    struct Bench { std::size_t n; std::uint64_t seed; std::uint32_t sample_rate; };
    std::vector<Bench> benches = {
        {100'000,    2025, 1},   // record all
        {10'000'000, 2026, 50}   // sample to keep memory sane
    };

    for (const auto& b : benches) {
        std::cout << "---- n=" << b.n << " ----\n";
        auto vecAsc = generateRandomVector3(b.n, -1'000'000, 1'000'000, b.seed);
        auto vecDesc = vecAsc;

        Profiler profAsc(b.sample_rate);
        auto tAsc = timeItMs([&] { heapSortAscending(vecAsc, &profAsc); });
        std::cout << "Ascending heapsort: " << std::fixed << std::setprecision(2) << tAsc << " ms"
            << " (ok=" << isSortedAsc(vecAsc) << ")\n";
        printProfilerSummary2(profAsc, "Ascending");

        Profiler profDesc(b.sample_rate);
        auto tDesc = timeItMs([&] { heapSortDescending(vecDesc, &profDesc); });
        std::cout << "Descending heapsort: " << std::fixed << std::setprecision(2) << tDesc << " ms"
            << " (ok=" << isSortedDesc(vecDesc) << ")\n";
        printProfilerSummary2(profDesc, "Descending");
    }

    return 0;
}
