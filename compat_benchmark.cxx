// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Performance and correctness comparison benchmark for simgear/compat/ headers.
//
// Compile twice — once with and once without -DSG_NO_BOOST — then compare:
//   --correctness  print deterministic KEY=VALUE lines (diffable)
//   --timing       print KEY=ns_per_op lines for performance table
//   (no args)      run both modes
//
// Build:
//   g++ -std=c++20 -O2 -I<repo>              compat_benchmark.cxx -o bench_boost
//   g++ -std=c++20 -O2 -I<repo> -DSG_NO_BOOST compat_benchmark.cxx -o bench_compat

// ── Conditional includes (same pattern as production source files) ─────────────

#ifdef SG_NO_BOOST
#  include <simgear/compat/tokenizer.hxx>
#  include <simgear/compat/functional_hash.hxx>
#  include <simgear/compat/optional.hxx>
#  include <simgear/compat/algorithm_string.hxx>
#  include <simgear/compat/iterator_facade.hxx>
#  include <simgear/compat/iterator_adaptor.hxx>
#else
#  include <boost/tokenizer.hpp>
#  include <boost/functional/hash.hpp>
#  include <boost/optional.hpp>
#  include <boost/algorithm/string/find_iterator.hpp>
#  include <boost/algorithm/string/predicate.hpp>
#  include <boost/range.hpp>
#  include <boost/iterator/iterator_facade.hpp>
#  include <boost/iterator/iterator_adaptor.hpp>
#endif

#include <chrono>
#include <climits>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// ── Timing harness ────────────────────────────────────────────────────────────

template<typename Fn>
double time_ns(long N, Fn&& fn)
{
    // Warm-up pass to bring code and data into cache
    fn();
    auto t0 = std::chrono::high_resolution_clock::now();
    for (long i = 0; i < N; ++i) fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
}

// ── Fixed datasets ────────────────────────────────────────────────────────────
// Inputs mirror the actual SimGear call sites (CSSBorder.cxx,
// SVGpreserveAspectRatio.cxx, props.cxx) plus edge cases.

namespace datasets {

// Tokenizer: split on " \t\n" (dropped delimiters, no kept, no empty tokens)
const std::vector<std::string> tok = {
    "5 10% 20 none",          // CSSBorder: 4 tokens
    "xMidYMid meet",          // SVGpreserveAspectRatio: 2 tokens
    "defer xMidYMid meet",    // SVGpreserveAspectRatio with defer: 3 tokens
    "100% 0 0 none",          // CSSBorder variant
    "  hello   world  ",      // leading/trailing whitespace: 2 tokens
    "a\tb\nc",                // tab and newline delimiters: 3 tokens
    "a   b   c   d   e",      // consecutive spaces: 5 tokens
    "",                       // empty string → 0 tokens
    "singletoken",            // no delimiter → 1 token
    "1 2 3 4",                // 4 numeric tokens
};

// Hash: integer inputs (hash_value identical between Boost and std::hash on GCC x86-64)
const std::vector<int> hash_ints = {0, 1, -1, 42, 1000000, INT_MAX};

// Hash: string inputs (used for timing only; values differ by implementation)
const std::vector<std::string> hash_strings = {
    "", ".", "..", "foo", "engine", "/foo/bar/baz", std::string(200, 'x')
};

// Hash: integer sequence for hash_range (simulates VEC3D/VEC4D hashing in props.cxx)
const std::vector<int> hash_range_ints = {1, 2, 3, 4, 100, -1};

// Optional: present/absent/conditional
const std::vector<std::string> opt_values = {"", "hello", "a very long string value"};

// equals / iterator_range: (candidate, query) pairs — mirrors find_node_aux / find_child
const std::vector<std::pair<std::string, std::string>> eq_pairs = {
    {"foo",    "foo"},      // equal
    {"foo",    "fo"},       // query shorter
    {"foo",    "fooo"},     // query longer
    {".",      "."},        // dot (identity path component)
    {"..",     ".."},       // dotdot (parent)
    {"engine", "engine"},   // typical property name
    {"",       ""},         // both empty
    {"foo",    "bar"},      // different strings
};

// split_iterator: paths mirrors SGPropertyNode getNode() traversal patterns
const std::vector<std::string> paths = {
    "/",
    "/foo",
    "/foo/bar/baz",
    "engine[0]/thrust",
    "foo/bar",
    "name",
    "/foo/",              // trailing slash
    "/a/b/c/d/e",
};

} // namespace datasets

// ── iterator_facade demo type ─────────────────────────────────────────────────
// Mirrors DoubledIterator from iterator_test.cxx — doubles each element on deref.

class DoubledIterator
    : public boost::iterator_facade<DoubledIterator, int,
                                    boost::forward_traversal_tag,
                                    int>   // proxy reference: returns by value
{
public:
    DoubledIterator() : ptr_(nullptr) {}
    explicit DoubledIterator(int* p) : ptr_(p) {}

private:
    friend class boost::iterator_core_access;
    int  dereference() const { return *ptr_ * 2; }
    void increment()         { ++ptr_; }
    bool equal(const DoubledIterator& o) const { return ptr_ == o.ptr_; }
    int* ptr_;
};

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string to_hex(std::size_t v)
{
    std::ostringstream os;
    os << "0x" << std::hex << v;
    return os.str();
}

// ═════════════════════════════════════════════════════════════════════════════
// CORRECTNESS SECTION
//
// Outputs KEY=VALUE lines — deterministic, diffable between the two binaries.
// Only APIs whose output is guaranteed identical between Boost and compat are
// included here: tokenizer, optional, equals, split_iterator, iterator_facade.
//
// Hash values are implementation-defined and differ between Boost and std::hash
// for non-integer types; they are benchmarked for timing only.
// ═════════════════════════════════════════════════════════════════════════════

static void correctness_tokenizer()
{
    using Tok = boost::tokenizer<boost::char_separator<char>>;
    const boost::char_separator<char> sep(" \t\n");

    for (std::size_t i = 0; i < datasets::tok.size(); ++i) {
        const auto& s = datasets::tok[i];
        Tok t(s.begin(), s.end(), sep);
        std::string joined;
        for (auto it = t.begin(); it != t.end(); ++it) {
            if (!joined.empty()) joined += '|';
            joined += *it;
        }
        std::cout << "tok." << i << '=' << joined << '\n';
    }

    // current_token() — mirrors SVGpreserveAspectRatio.cxx usage
    {
        std::string s = "defer xMidYMid meet";
        Tok t(s.begin(), s.end(), sep);
        auto it = t.begin();
        std::cout << "tok.current_token.0=" << it.current_token() << '\n';
        ++it;
        std::cout << "tok.current_token.1=" << it.current_token() << '\n';
    }
}

static void correctness_optional()
{
    // none state
    boost::optional<std::string> empty = boost::none;
    std::cout << "opt.none.has_value=" << (empty ? "1" : "0") << '\n';
    std::cout << "opt.none.value_or="  << empty.value_or("DEFAULT") << '\n';

    // valued states
    for (std::size_t i = 0; i < datasets::opt_values.size(); ++i) {
        boost::optional<std::string> o = datasets::opt_values[i];
        std::cout << "opt." << i << ".has_value=" << (o ? "1" : "0") << '\n';
        std::cout << "opt." << i << ".value="     << (o ? *o : "none") << '\n';
        std::cout << "opt." << i << ".value_or="  << o.value_or("FALLBACK") << '\n';
    }

    // conditional make_optional
    auto co = boost::make_optional(true,  std::string("yes"));
    auto cn = boost::make_optional(false, std::string("no"));
    std::cout << "opt.make_optional.true="  << (co ? *co : "none") << '\n';
    std::cout << "opt.make_optional.false=" << (cn ? *cn : "none") << '\n';

    // operator-> access
    boost::optional<std::string> s = std::string("hello");
    std::cout << "opt.arrow.size=" << s->size() << '\n';

    // assignment from none
    boost::optional<std::string> reassign = std::string("initial");
    reassign = boost::none;
    std::cout << "opt.reassign.after_none=" << (reassign ? "1" : "0") << '\n';
}

static void correctness_equals()
{
    for (std::size_t i = 0; i < datasets::eq_pairs.size(); ++i) {
        const auto& [cand, query] = datasets::eq_pairs[i];
        auto r = boost::make_iterator_range(query.c_str(),
                                            query.c_str() + query.size());
        bool result = boost::equals(cand, r);
        std::cout << "eq." << i << '=' << (result ? "1" : "0") << '\n';
    }

    // is_equal predicate (used by first_finder)
    boost::is_equal pred;
    std::cout << "is_equal.same="      << pred('/', '/') << '\n';
    std::cout << "is_equal.different=" << pred('/', 'x') << '\n';
}

static void correctness_split()
{
    for (std::size_t i = 0; i < datasets::paths.size(); ++i) {
        const auto& path = datasets::paths[i];
        auto r = boost::make_iterator_range(path.c_str(),
                                            path.c_str() + path.size());
        auto itr = boost::make_split_iterator(
            r, boost::first_finder("/", boost::is_equal()));
        std::string joined;
        while (!itr.eof()) {
            auto seg = *itr;
            joined += std::string(seg.begin(), seg.end());
            ++itr;
            if (!itr.eof()) joined += '|';
        }
        std::cout << "split." << i << '=' << joined << '\n';
    }

    // arrow operator: itr->empty() — mirrors find_node_aux
    {
        const char* p = "/foo";
        auto r = boost::make_iterator_range(p, p + strlen(p));
        auto itr = boost::make_split_iterator(
            r, boost::first_finder("/", boost::is_equal()));
        std::cout << "split.arrow.first.empty=" << (itr->empty() ? "1" : "0") << '\n';
        ++itr;
        std::cout << "split.arrow.second.empty=" << (itr->empty() ? "1" : "0") << '\n';
    }
}

static void correctness_iter_facade()
{
    int arr[] = {1, 2, 3, 4, 5};
    std::string joined;
    for (auto it = DoubledIterator(arr); it != DoubledIterator(arr + 5); ++it) {
        if (!joined.empty()) joined += '|';
        joined += std::to_string(*it);
    }
    std::cout << "iter_facade.doubled=" << joined << '\n';

    // post-increment
    DoubledIterator it(arr);
    auto old = it++;
    std::cout << "iter_facade.post_inc.old=" << *old << '\n';   // arr[0]*2 = 2
    std::cout << "iter_facade.post_inc.new=" << *it  << '\n';   // arr[1]*2 = 4
}

// ── Hash correctness: integer values only (guaranteed identical on this platform)

static void correctness_hash_ints()
{
    // hash_value for integers: both Boost and std::hash<int> return identity on GCC x86-64
    for (std::size_t i = 0; i < datasets::hash_ints.size(); ++i)
        std::cout << "hash.int." << i << '='
                  << to_hex(boost::hash_value(datasets::hash_ints[i])) << '\n';

    // hash_range over integer sequence
    std::cout << "hash.range_ints="
              << to_hex(boost::hash_range(datasets::hash_range_ints.begin(),
                                           datasets::hash_range_ints.end()))
              << '\n';

    // hash_combine sequence (same formula in both: 0x9e3779b9 golden ratio)
    {
        std::size_t seed = 0;
        for (int v : datasets::hash_ints)
            boost::hash_combine(seed, v);
        std::cout << "hash.combine_ints=" << to_hex(seed) << '\n';
    }

    // hash<int> functor
    boost::hash<int> h;
    std::cout << "hash.functor.int=" << to_hex(h(42)) << '\n';
}

// ═════════════════════════════════════════════════════════════════════════════
// TIMING SECTION
//
// Each function prints: timing.<name>=<ns_per_operation>
// The shell script parses these and prints a side-by-side table.
// ═════════════════════════════════════════════════════════════════════════════

static constexpr long N = 500'000;

static void timing_tokenizer()
{
    using Tok = boost::tokenizer<boost::char_separator<char>>;
    const boost::char_separator<char> sep(" \t\n");
    const auto& inputs = datasets::tok;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& s : inputs) {
            Tok t(s.begin(), s.end(), sep);
            for (const auto& tok : t)
                sink += tok.size();
        }
    });
    std::cout << "timing.tokenizer=" << ns / (double)inputs.size() << '\n';
}

static void timing_hash_int()
{
    const auto& ints = datasets::hash_ints;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (int v : ints)
            sink ^= boost::hash_value(v);
    });
    std::cout << "timing.hash_value_int=" << ns / (double)ints.size() << '\n';
}

static void timing_hash_string()
{
    const auto& strs = datasets::hash_strings;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& s : strs)
            sink ^= boost::hash_value(s);
    });
    std::cout << "timing.hash_value_str=" << ns / (double)strs.size() << '\n';
}

static void timing_hash_combine()
{
    const auto& strs = datasets::hash_strings;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        std::size_t seed = 0;
        for (const auto& s : strs)
            boost::hash_combine(seed, s);
        sink ^= seed;
    });
    std::cout << "timing.hash_combine=" << ns / (double)strs.size() << '\n';
}

static void timing_hash_range()
{
    const auto& v = datasets::hash_range_ints;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        sink ^= boost::hash_range(v.begin(), v.end());
    });
    std::cout << "timing.hash_range=" << ns << '\n';
}

static void timing_optional()
{
    const auto& vals = datasets::opt_values;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& v : vals) {
            boost::optional<std::string> o = v;
            if (o) sink += o->size();
            boost::optional<std::string> empty = boost::none;
            sink += empty.value_or("DEFAULT").size();
        }
    });
    std::cout << "timing.optional=" << ns / (double)vals.size() << '\n';
}

static void timing_equals()
{
    const auto& pairs = datasets::eq_pairs;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& [cand, query] : pairs) {
            auto r = boost::make_iterator_range(query.c_str(),
                                                query.c_str() + query.size());
            sink += boost::equals(cand, r) ? 1 : 0;
        }
    });
    std::cout << "timing.equals=" << ns / (double)pairs.size() << '\n';
}

static void timing_split()
{
    const auto& paths = datasets::paths;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& path : paths) {
            auto r = boost::make_iterator_range(path.c_str(),
                                                path.c_str() + path.size());
            auto itr = boost::make_split_iterator(
                r, boost::first_finder("/", boost::is_equal()));
            while (!itr.eof()) {
                auto seg = *itr;
                sink += std::distance(seg.begin(), seg.end());
                ++itr;
            }
        }
    });
    std::cout << "timing.split=" << ns / (double)paths.size() << '\n';
}

static void timing_iter_facade()
{
    int arr[64];
    std::iota(arr, arr + 64, 1);
    volatile int sink = 0;

    double ns = time_ns(N, [&]{
        for (auto it = DoubledIterator(arr); it != DoubledIterator(arr + 64); ++it)
            sink += *it;
    });
    std::cout << "timing.iter_facade=" << ns / 64.0 << '\n';
}

// ═════════════════════════════════════════════════════════════════════════════

int main(int argc, char** argv)
{
    bool do_correctness = true;
    bool do_timing      = true;

    if (argc >= 2) {
        std::string mode(argv[1]);
        do_correctness = (mode == "--correctness");
        do_timing      = (mode == "--timing");
        if (mode == "--hash-correctness") {
            correctness_hash_ints();
            return 0;
        }
    }

    if (do_correctness) {
        correctness_tokenizer();
        correctness_optional();
        correctness_equals();
        correctness_split();
        correctness_iter_facade();
    }

    if (do_timing) {
        timing_tokenizer();
        timing_hash_int();
        timing_hash_string();
        timing_hash_combine();
        timing_hash_range();
        timing_optional();
        timing_equals();
        timing_split();
        timing_iter_facade();
    }

    return 0;
}
