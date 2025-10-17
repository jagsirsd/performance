#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <vector>

// -------------------- Profiler (unchanged) --------------------
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

// -------------------- Heapsort with inline comparisons --------------------
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
    for (std::size_t i = (a.size() - 2) / 2 + 1; i-- > 0; ) {
        siftDown<MaxHeap>(a, i, a.size(), prof);
    }
}

// Ascending sort uses a MAX-heap
template <class T>
inline void heapSortAscending(std::vector<T>& a, Profiler* profiler = nullptr) {
    buildHeap<true>(a, profiler);
    for (std::size_t end = a.size(); end > 1; --end) {
        std::swap(a[0], a[end - 1]);
        siftDown<true>(a, 0, end - 1, profiler);
    }
}
struct AVL {
    struct Node {
        int   key;
        int   h;
        Node* l;
        Node* r;
        explicit Node(int k) : key(k), h(1), l(nullptr), r(nullptr) {}
    };

    Node* root = nullptr;
    ~AVL() { destroy(root); }

    // --- utilities ---
    static inline int height(Node* n) noexcept { return n ? n->h : 0; }
    static inline int bf(Node* n) noexcept { return n ? height(n->l) - height(n->r) : 0; }
    static inline void update(Node* n) noexcept {
        n->h = 1 + std::max(height(n->l), height(n->r));
    }

    // Right rotation:        y                x
    //                       / \              / \
    //                      x   T3   ->      T1  y
    //                     / \                  / \
    //                    T1 T2                T2 T3
    static Node* rotateRight(Node* y) noexcept {
        Node* x = y->l;             // must be non-null if called correctly
        Node* T2 = x ? x->r : nullptr;
        // rotate
        if (x) x->r = y;
        y->l = T2;
        // fix heights (child first)
        update(y);
        if (x) update(x);
        return x ? x : y;            // fallback (should never be null)
    }

    // Left rotation:          x                  y
    //                        / \                / \
    //                       T1  y     ->       x  T3
    //                          / \            / \
    //                         T2 T3          T1 T2
    static Node* rotateLeft(Node* x) noexcept {
        Node* y = x->r;             // must be non-null if called correctly
        Node* T2 = y ? y->l : nullptr;
        // rotate
        if (y) y->l = x;
        x->r = T2;
        // fix heights
        update(x);
        if (y) update(y);
        return y ? y : x;
    }

    // Rebalance around n after an insertion in a subtree.
    static Node* rebalance(Node* n) noexcept {
        update(n);
        int b = bf(n);

        // Left heavy
        if (b > 1) {
            // If left-right case, convert to left-left
            if (bf(n->l) < 0) {
                n->l = rotateLeft(n->l);
            }
            return rotateRight(n);
        }

        // Right heavy
        if (b < -1) {
            // If right-left case, convert to right-right
            if (bf(n->r) > 0) {
                n->r = rotateRight(n->r);
            }
            return rotateLeft(n);
        }
        return n; // already balanced
    }

    // Standard BST insert (duplicates go to the right), then rebalance.
    static Node* insertRec(Node* n, int key) {
        if (!n) return new Node(key);
        if (key < n->key) {
            n->l = insertRec(n->l, key);
        }
        else {
            n->r = insertRec(n->r, key); // duplicates to the right
        }
        return rebalance(n);
    }

    // Iterative inorder traversal (no recursion) to produce sorted output.
    static void inorderIter(Node* n, std::vector<int>& out) {
        out.clear();
        std::vector<Node*> st;
        st.reserve(64);
        Node* cur = n;
        while (cur || !st.empty()) {
            while (cur) {
                st.push_back(cur);
                cur = cur->l;
            }
            cur = st.back(); st.pop_back();
            out.push_back(cur->key);
            cur = cur->r;
        }
    }

    static void destroy(Node* n) noexcept {
        // Post-order iterative to avoid recursion
        std::vector<Node*> st;
        Node* cur = n;
        Node* last = nullptr;
        while (cur || !st.empty()) {
            if (cur) {
                st.push_back(cur);
                cur = cur->l;
            }
            else {
                Node* peek = st.back();
                if (peek->r && last != peek->r) {
                    cur = peek->r;
                }
                else {
                    st.pop_back();
                    last = peek;
                    delete peek;
                }
            }
        }
    }

    // --- public API ---
    void insert(int k) { root = insertRec(root, k); }

    void to_sorted(std::vector<int>& out) const {
        inorderIter(root, out);
    }
};

// -------------------- AVL Tree (insert + inorder) --------------------
struct AVL2 {
    struct Node {
        int key;
        int h;
        Node* l;
        Node* r;
        explicit Node(int k) : key(k), h(1), l(nullptr), r(nullptr) {}
    };
    Node* root = nullptr;
    ~AVL2() { destroy(root); }

    static inline int height(Node* n) { return n ? n->h : 0; }
    static inline int bf(Node* n) { return n ? height(n->l) - height(n->r) : 0; }
    static inline void fix(Node* n) { n->h = 1 + std::max(height(n->l), height(n->r)); }

    static Node* rotR(Node* y) {
        Node* x = y->l; Node* T2 = x->r;
        x->r = y; y->l = T2; fix(y); fix(x); return x;
    }
    static Node* rotL(Node* x) {
        Node* y = x->r; Node* T2 = y->l;
        y->l = x; x->r = T2; fix(x); fix(y); return y;
    }

    static Node* insert(Node* n, int key) {
        if (!n) return new Node(key);
        if (key < n->key) n->l = insert(n->l, key);
        else              n->r = insert(n->r, key); // duplicates go right; stable order not guaranteed

        fix(n);
        int balance = bf(n);

        // LL
        if (balance > 1 && key < n->l->key) return rotR(n);
        // RR
        if (balance < -1 && key > n->r->key) return rotL(n);
        // LR
        if (balance > 1 && key > n->l->key) { n->l = rotL(n->l); return rotR(n); }
        // RL
        if (balance < -1 && key < n->r->key) { n->r = rotR(n->r); return rotL(n); }

        return n;
    }

    static void inorder(Node* n, std::vector<int>& out) {
        if (!n) return;
        inorder(n->l, out);
        out.push_back(n->key);
        inorder(n->r, out);
    }
    static void destroy(Node* n) {
        if (!n) return;
        destroy(n->l); destroy(n->r); delete n;
    }

    void insert(int k) { root = insert(root, k); }
    void to_sorted(std::vector<int>& out) const { out.clear(); out.reserve(size_hint); inorder(root, out); }
    std::size_t size_hint = 0; // optional: set before inserts to reserve() during inorder
};

// -------------------- Utilities --------------------
std::vector<int> generateRandomVector4(std::size_t n,
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
double timeMs(F&& f) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    f();
    const auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template <class T>
bool isSortedAsc(const std::vector<T>& v) {
    for (std::size_t i = 1; i < v.size(); ++i) if (v[i] < v[i - 1]) return false;
    return true;
}

// -------------------- Benchmark harness --------------------
struct Row {
    std::size_t n;
    double heap_ms{};
    double sort_ms{};

    double avl_build_ms{};
    double avl_inorder_ms{};
    double avl_total_ms{};

    double rb_build_ms{};
    double rb_iter_ms{};
    double rb_total_ms{};
};

int avl_vs_rb_tree_sort_main() {
    // Choose sizes. Tree cases at 10M/20M can be huge; see guard below.
    const std::vector<std::size_t> sizes = {
        10'000, 100'000, 500'000, 1'000'000, 2'000'000, 3'000'000, 5'000'000, 7'500'000, 10'000'000, 15'000'000, 20'000'000, 25'000'000, 30'000'000, 35'000'000, 40'000'000, 45'000'000, 50'000'000
    };

    // For very large n, you can skip tree-based sorts to avoid OOM/long runs:
    const bool INCLUDE_LARGE_TREE_CASES = true; // set true if you want to try 10M/20M via trees

    std::vector<Row> table;
    table.reserve(sizes.size());

    for (auto n : sizes) {
        // Make the same random data for all algorithms
        auto base = generateRandomVector4(n, -1'000'000, 1'000'000, /*seed*/ 123456789ULL + n);

        Row row{};
        row.n = n;

        // Heapsort (ascending)
        {
            auto v = base;
            Profiler prof(50); // sampled; set to 1 if you want per-call records
            row.heap_ms = timeMs([&] { heapSortAscending(v, &prof); });
            std::cout << "[n=" << n << "] HeapSort ok=" << std::boolalpha << isSortedAsc(v)
                << " time=" << row.heap_ms << " ms\n";
        }

        // std::sort (baseline)
        {
            auto v = base;
            row.sort_ms = timeMs([&] { std::sort(v.begin(), v.end()); });
            std::cout << "[n=" << n << "] std::sort ok=" << std::boolalpha << isSortedAsc(v)
                << " time=" << row.sort_ms << " ms\n";
        }

        // AVL build + inorder (sorted)
        bool doTrees = (n <= 50'000'000) || INCLUDE_LARGE_TREE_CASES; // guard for large cases
        if (doTrees) {
            // AVL
            {
                AVL avl;
                row.avl_build_ms = timeMs([&] {
                    for (auto x : base) avl.insert(x);
                    });
                std::vector<int> out;
                out.reserve(n);
                row.avl_inorder_ms = timeMs([&] { avl.to_sorted(out); });
                row.avl_total_ms = row.avl_build_ms + row.avl_inorder_ms;
                std::cout << "[n=" << n << "] AVL sorted ok=" << std::boolalpha << isSortedAsc(out)
                    << " build=" << row.avl_build_ms << " ms inorder=" << row.avl_inorder_ms
                    << " ms total=" << row.avl_total_ms << " ms\n";
            }

            // Red-Black via std::set
            {
                std::set<int> s; // (Typically RB-tree in MSVC)
                row.rb_build_ms = timeMs([&] {
                    for (auto x : base) s.insert(x);
                    });
                std::vector<int> out;
                out.reserve(s.size());
                row.rb_iter_ms = timeMs([&] {
                    for (auto x : s) out.push_back(x);
                    });
                row.rb_total_ms = row.rb_build_ms + row.rb_iter_ms;
                std::cout << "[n=" << n << "] RB(std::set) sorted ok="
                    << std::boolalpha << isSortedAsc(out)
                    << " build=" << row.rb_build_ms << " ms iterate=" << row.rb_iter_ms
                    << " ms total=" << row.rb_total_ms << " ms\n";
            }
        }
        else {
            row.avl_build_ms = row.avl_inorder_ms = row.avl_total_ms = NAN;
            row.rb_build_ms = row.rb_iter_ms = row.rb_total_ms = NAN;
            std::cout << "[n=" << n << "] (Skipping tree-based sorts for this large size)\n";
        }

        table.push_back(row);
        std::cout << "-----------------------------------------------------------\n";
    }

    // Print summary table
    std::cout << "\n=== Comparative Sorting Times (ms) ===\n";
    std::cout << std::left << std::setw(12) << "n"
        << std::right << std::setw(12) << "HeapSort"
        << std::setw(12) << "std::sort"
        << std::setw(14) << "AVL build"
        << std::setw(14) << "AVL inorder"
        << std::setw(12) << "AVL total"
        << std::setw(14) << "RB build"
        << std::setw(12) << "RB iter"
        << std::setw(12) << "RB total"
        << "\n";

    for (const auto& r : table) {
        auto fmt = [&](double x) {
            if (std::isnan(x)) return std::string("  (skip)  ");
            std::ostringstream os; os << std::fixed << std::setprecision(2) << x;
            return os.str();
            };
        std::cout << std::left << std::setw(12) << r.n
            << std::right << std::setw(12) << fmt(r.heap_ms)
            << std::setw(12) << fmt(r.sort_ms)
            << std::setw(14) << fmt(r.avl_build_ms)
            << std::setw(14) << fmt(r.avl_inorder_ms)
            << std::setw(12) << fmt(r.avl_total_ms)
            << std::setw(14) << fmt(r.rb_build_ms)
            << std::setw(12) << fmt(r.rb_iter_ms)
            << std::setw(12) << fmt(r.rb_total_ms)
            << "\n";
    }

    return 0;
}
