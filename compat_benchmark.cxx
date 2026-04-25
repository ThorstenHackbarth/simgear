// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Performance and correctness comparison benchmark for simgear/compat/ headers.
//
// Compile twice — once with and once without -DSG_NO_BOOST — then compare:
//   --correctness       print deterministic KEY=VALUE lines (diffable)
//   --hash-correctness  hash-only correctness check
//   --timing            print KEY=ns_per_op lines for performance table
//   (no args)           run both modes
//
// Dataset sizes are ~100x larger than the original and are generated with a
// seeded PCG32 PRNG so runs are reproducible across both variants.
//
// Build:
//   g++ -std=c++20 -O2 -I<repo>              compat_benchmark.cxx -o bench_boost
//   g++ -std=c++20 -O2 -I<repo> -DSG_NO_BOOST compat_benchmark.cxx -o bench_compat

// ── Conditional includes ───────────────────────────────────────────────────────

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
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// ── PCG32 — seeded PRNG for reproducible fuzzing ──────────────────────────────
// Minimal implementation of the PCG-XSH-RR generator (Melissa O'Neill, 2014).
// Fixed seed ensures identical datasets across both Boost and compat binaries.

struct Pcg32 {
    uint64_t state;
    uint64_t inc;

    explicit Pcg32(uint64_t seed = 0x853c49e6748fea9bULL,
                   uint64_t seq  = 0xda3e39cb94b95bdbULL)
        : state(0), inc((seq << 1u) | 1u)
    {
        step(); state += seed; step();
    }

    uint32_t step() {
        uint64_t old = state;
        state = old * 6364136223846793005ULL + inc;
        uint32_t xsh = uint32_t(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = uint32_t(old >> 59u);
        return (xsh >> rot) | (xsh << ((-rot) & 31u));
    }

    // Uniform in [0, bound)
    uint32_t bounded(uint32_t bound) {
        if (bound == 0) return 0;
        uint32_t threshold = uint32_t(-bound) % bound;
        for (;;) { uint32_t r = step(); if (r >= threshold) return r % bound; }
    }

    // Random string from alphabet, length in [min_len, max_len]
    std::string gen_string(uint32_t min_len, uint32_t max_len,
                           const char* alpha = nullptr,
                           uint32_t   alen   = 0)
    {
        static const char DEF_ALPHA[] =
            "abcdefghijklmnopqrstuvwxyz_0123456789";
        if (!alpha) { alpha = DEF_ALPHA; alen = uint32_t(sizeof(DEF_ALPHA) - 1); }
        uint32_t len = min_len + (max_len > min_len ? bounded(max_len - min_len + 1) : 0);
        std::string s(len, '\0');
        for (char& c : s) c = alpha[bounded(alen)];
        return s;
    }

    int32_t gen_int() { return int32_t(step()); }
};

// ── Timing harness ────────────────────────────────────────────────────────────

template<typename Fn>
double time_ns(long N, Fn&& fn)
{
    fn(); // warm-up
    auto t0 = std::chrono::high_resolution_clock::now();
    for (long i = 0; i < N; ++i) fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
}

// ═════════════════════════════════════════════════════════════════════════════
// FIXED DATASETS — small, deterministic, used only for correctness checks.
// These are identical to the original benchmark so the diff output is stable.
// ═════════════════════════════════════════════════════════════════════════════

namespace fixed {

const std::vector<std::string> tok = {
    "5 10% 20 none", "xMidYMid meet", "defer xMidYMid meet",
    "100% 0 0 none", "  hello   world  ", "a\tb\nc",
    "a   b   c   d   e", "", "singletoken", "1 2 3 4",
};

const std::vector<int> hash_ints = {0, 1, -1, 42, 1000000, INT_MAX};

const std::vector<int> hash_range_ints = {1, 2, 3, 4, 100, -1};

const std::vector<std::string> opt_values = {
    "", "hello", "a very long string value"
};

const std::vector<std::pair<std::string,std::string>> eq_pairs = {
    {"foo","foo"}, {"foo","fo"}, {"foo","fooo"}, {".","."}, {"..",".."},
    {"engine","engine"}, {"",""}, {"foo","bar"},
};

const std::vector<std::string> paths = {
    "/", "/foo", "/foo/bar/baz", "engine[0]/thrust", "foo/bar",
    "name", "/foo/", "/a/b/c/d/e",
};

} // namespace fixed

// ═════════════════════════════════════════════════════════════════════════════
// FUZZED DATASETS — ~100x larger, generated with PCG32 seed=42.
// All timing benchmarks use these to expose cache-miss and branch-prediction
// effects that small fixed datasets cannot reveal.
// ═════════════════════════════════════════════════════════════════════════════

namespace fuzz {

// Target sizes (approximately 100x the original fixed datasets)
static constexpr int N_TOK         = 1000;  // was 10
static constexpr int N_HASH_INT    =  600;  // was 6
static constexpr int N_HASH_STR    =  700;  // was 7
static constexpr int N_HASH_RANGE  =  600;  // was 6
static constexpr int N_OPT         =  300;  // was 3
static constexpr int N_EQ          =  800;  // was 8
static constexpr int N_PATHS       =  800;  // was 8
static constexpr int N_ITER        = 6400;  // was 64

// CSS/SVG token pieces for realistic tokenizer inputs
static const char* const CSS_TOKENS[] = {
    "none","auto","inherit","px","em","rem","vh","vw","%","solid",
    "dashed","dotted","hidden","0","1","5","10","20","50","100","200",
    "top","bottom","left","right","center","stretch","fill","meet",
    "slice","defer","xMinYMin","xMidYMid","xMaxYMax","xMinYMid",
};
static constexpr int N_CSS = int(sizeof(CSS_TOKENS) / sizeof(CSS_TOKENS[0]));

// SimGear-like path components
static const char* const PATH_PARTS[] = {
    "position","orientation","velocity","acceleration","engines","engine",
    "fuel","systems","autopilot","controls","consumables","gear","fdm",
    "instrumentation","environment","sim","ai","traffic","multiplayer",
    "nasal","scenery","rendering","lighting","sound","payload","sensors",
};
static constexpr int N_PARTS = int(sizeof(PATH_PARTS) / sizeof(PATH_PARTS[0]));

static const char* const PATH_SEP[] = {" ","  ","\t","\n"};

// ── Generators ────────────────────────────────────────────────────────────────

static std::vector<std::string> make_tok(Pcg32& rng)
{
    std::vector<std::string> v;
    v.reserve(N_TOK);
    for (int i = 0; i < N_TOK; ++i) {
        uint32_t n_tokens = rng.bounded(8);       // 0–7 tokens per string
        std::string s;
        for (uint32_t t = 0; t < n_tokens; ++t) {
            if (t) s += PATH_SEP[rng.bounded(4)]; // varied separator
            s += CSS_TOKENS[rng.bounded(N_CSS)];
        }
        // Occasionally add leading/trailing whitespace
        if (rng.bounded(5) == 0) s = " " + s;
        if (rng.bounded(5) == 0) s = s + "  ";
        v.push_back(std::move(s));
    }
    return v;
}

static std::vector<int> make_hash_ints(Pcg32& rng)
{
    std::vector<int> v;
    v.reserve(N_HASH_INT);
    // Seed with known interesting values then fill randomly
    const int special[] = {0, 1, -1, INT_MAX, INT_MIN, 42, 255, 65535};
    for (int x : special) v.push_back(x);
    while (int(v.size()) < N_HASH_INT) v.push_back(rng.gen_int());
    return v;
}

static std::vector<std::string> make_hash_strings(Pcg32& rng)
{
    // Length distribution mirrors real property strings: many short, some long
    std::vector<std::string> v;
    v.reserve(N_HASH_STR);
    v.push_back("");                                             // empty
    for (int i = 0; i <  10; ++i) v.push_back(rng.gen_string(  1,   3)); // tiny
    for (int i = 0; i < 200; ++i) v.push_back(rng.gen_string(  4,  15)); // typical property name
    for (int i = 0; i < 250; ++i) v.push_back(rng.gen_string( 16,  60)); // path-like
    for (int i = 0; i < 150; ++i) v.push_back(rng.gen_string( 61, 150)); // long
    for (int i = 0; i <  89; ++i) v.push_back(rng.gen_string(151, 500)); // very long
    return v;
}

static std::vector<int> make_hash_range(Pcg32& rng)
{
    std::vector<int> v;
    v.reserve(N_HASH_RANGE);
    for (int i = 0; i < N_HASH_RANGE; ++i) v.push_back(rng.gen_int());
    return v;
}

static std::vector<std::string> make_opt_values(Pcg32& rng)
{
    std::vector<std::string> v;
    v.reserve(N_OPT);
    v.push_back("");
    for (int i = 0; i <  99; ++i) v.push_back(rng.gen_string( 1, 10));
    for (int i = 0; i < 100; ++i) v.push_back(rng.gen_string(11, 80));
    for (int i = 0; i < 100; ++i) v.push_back(rng.gen_string(81,400));
    return v;
}

static std::vector<std::pair<std::string,std::string>> make_eq_pairs(Pcg32& rng)
{
    std::vector<std::pair<std::string,std::string>> v;
    v.reserve(N_EQ);
    for (int i = 0; i < N_EQ; ++i) {
        std::string a = rng.gen_string(0, 30);
        if (rng.bounded(2)) {
            v.push_back({a, a});              // equal
        } else {
            std::string b = rng.gen_string(0, 30);
            v.push_back({a, std::move(b)});   // likely unequal
        }
    }
    return v;
}

static std::vector<std::string> make_paths(Pcg32& rng)
{
    std::vector<std::string> v;
    v.reserve(N_PATHS);
    v.push_back("/");
    for (int i = 1; i < N_PATHS; ++i) {
        uint32_t depth = 1 + rng.bounded(6);  // 1–6 components deep
        std::string p;
        if (rng.bounded(3)) p = "/";          // absolute 2/3 of the time
        for (uint32_t d = 0; d < depth; ++d) {
            if (d) p += '/';
            p += PATH_PARTS[rng.bounded(N_PARTS)];
            if (rng.bounded(4) == 0) {
                // array index notation: engine[0]
                p += '[';
                p += std::to_string(rng.bounded(8));
                p += ']';
            }
        }
        if (rng.bounded(8) == 0) p += '/';    // occasional trailing slash
        v.push_back(std::move(p));
    }
    return v;
}

static std::vector<int> make_iter_array()
{
    std::vector<int> v(N_ITER);
    std::iota(v.begin(), v.end(), 1);
    return v;
}

// ── Dataset singleton — built once at startup ─────────────────────────────────

struct Datasets {
    std::vector<std::string>                        tok;
    std::vector<int>                                hash_ints;
    std::vector<std::string>                        hash_strings;
    std::vector<int>                                hash_range_ints;
    std::vector<std::string>                        opt_values;
    std::vector<std::pair<std::string,std::string>> eq_pairs;
    std::vector<std::string>                        paths;
    std::vector<int>                                iter_array;

    Datasets() {
        Pcg32 rng(/*seed=*/42, /*seq=*/1);
        tok             = make_tok(rng);
        hash_ints       = make_hash_ints(rng);
        hash_strings    = make_hash_strings(rng);
        hash_range_ints = make_hash_range(rng);
        opt_values      = make_opt_values(rng);
        eq_pairs        = make_eq_pairs(rng);
        paths           = make_paths(rng);
        iter_array      = make_iter_array();
    }
};

static const Datasets& get() {
    static Datasets d;
    return d;
}

} // namespace fuzz

// ── iterator_facade demo type ─────────────────────────────────────────────────

class DoubledIterator
    : public boost::iterator_facade<DoubledIterator, int,
                                    boost::forward_traversal_tag, int>
{
public:
    DoubledIterator() : ptr_(nullptr) {}
    explicit DoubledIterator(const int* p) : ptr_(p) {}
private:
    friend class boost::iterator_core_access;
    int  dereference() const { return *ptr_ * 2; }
    void increment()         { ++ptr_; }
    bool equal(const DoubledIterator& o) const { return ptr_ == o.ptr_; }
    const int* ptr_;
};

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string to_hex(std::size_t v) {
    std::ostringstream os;
    os << "0x" << std::hex << v;
    return os.str();
}

// ═════════════════════════════════════════════════════════════════════════════
// CORRECTNESS SECTION — uses full fuzzed datasets, outputs diffable KEY=VALUE.
// Both binaries use the same PCG32 seed so every line must be identical.
// ═════════════════════════════════════════════════════════════════════════════

static void correctness_tokenizer()
{
    using Tok = boost::tokenizer<boost::char_separator<char>>;
    const boost::char_separator<char> sep(" \t\n");
    const auto& inputs = fuzz::get().tok;

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        const auto& s = inputs[i];
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
    boost::optional<std::string> empty = boost::none;
    std::cout << "opt.none.has_value=" << (empty ? "1" : "0") << '\n';
    std::cout << "opt.none.value_or="  << empty.value_or("DEFAULT") << '\n';

    const auto& vals = fuzz::get().opt_values;
    for (std::size_t i = 0; i < vals.size(); ++i) {
        boost::optional<std::string> o = vals[i];
        std::cout << "opt." << i << ".has_value=" << (o ? "1" : "0") << '\n';
        std::cout << "opt." << i << ".value="     << (o ? *o : "none") << '\n';
        std::cout << "opt." << i << ".value_or="  << o.value_or("FALLBACK") << '\n';
    }

    auto co = boost::make_optional(true,  std::string("yes"));
    auto cn = boost::make_optional(false, std::string("no"));
    std::cout << "opt.make_optional.true="  << (co ? *co : "none") << '\n';
    std::cout << "opt.make_optional.false=" << (cn ? *cn : "none") << '\n';

    boost::optional<std::string> s = std::string("hello");
    std::cout << "opt.arrow.size=" << s->size() << '\n';

    boost::optional<std::string> reassign = std::string("initial");
    reassign = boost::none;
    std::cout << "opt.reassign.after_none=" << (reassign ? "1" : "0") << '\n';
}

static void correctness_equals()
{
    const auto& pairs = fuzz::get().eq_pairs;
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        const auto& [cand, query] = pairs[i];
        auto r = boost::make_iterator_range(query.c_str(),
                                            query.c_str() + query.size());
        bool result = boost::equals(cand, r);
        std::cout << "eq." << i << '=' << (result ? "1" : "0") << '\n';
    }
    boost::is_equal pred;
    std::cout << "is_equal.same="      << pred('/', '/') << '\n';
    std::cout << "is_equal.different=" << pred('/', 'x') << '\n';
}

static void correctness_split()
{
    const auto& paths = fuzz::get().paths;
    for (std::size_t i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];
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
    {
        const char* p = "/foo";
        auto r = boost::make_iterator_range(p, p + strlen(p));
        auto itr = boost::make_split_iterator(
            r, boost::first_finder("/", boost::is_equal()));
        std::cout << "split.arrow.first.empty="  << (itr->empty() ? "1" : "0") << '\n';
        ++itr;
        std::cout << "split.arrow.second.empty=" << (itr->empty() ? "1" : "0") << '\n';
    }
}

static void correctness_iter_facade()
{
    const auto& arr = fuzz::get().iter_array;
    long long sum = 0;
    for (auto it  = DoubledIterator(arr.data());
              it != DoubledIterator(arr.data() + arr.size()); ++it)
        sum += *it;
    std::cout << "iter_facade.doubled.sum=" << sum << '\n';
    std::cout << "iter_facade.doubled.n="   << arr.size() << '\n';

    DoubledIterator it(arr.data());
    auto old = it++;
    std::cout << "iter_facade.post_inc.old=" << *old << '\n';
    std::cout << "iter_facade.post_inc.new=" << *it  << '\n';
}

static void correctness_hash_ints()
{
    const auto& ints  = fuzz::get().hash_ints;
    const auto& range = fuzz::get().hash_range_ints;

    for (std::size_t i = 0; i < ints.size(); ++i)
        std::cout << "hash.int." << i << '='
                  << to_hex(boost::hash_value(ints[i])) << '\n';

    std::cout << "hash.range_ints="
              << to_hex(boost::hash_range(range.begin(), range.end())) << '\n';
    {
        std::size_t seed = 0;
        for (int v : ints) boost::hash_combine(seed, v);
        std::cout << "hash.combine_ints=" << to_hex(seed) << '\n';
    }
    boost::hash<int> h;
    std::cout << "hash.functor.int=" << to_hex(h(42)) << '\n';
}

// ═════════════════════════════════════════════════════════════════════════════
// TIMING SECTION — uses fuzzed (~100x) datasets.
// N is reduced proportionally so total wall time stays similar to the original.
// Each timing.* line prints: KEY=<ns per dataset item>
// ═════════════════════════════════════════════════════════════════════════════

// N=5,000 iterations × ~100 items/call ≈ 500,000 item-ops — same as original
// N=500,000 × 1 item-call.
static constexpr long N = 5'000;

static void timing_tokenizer()
{
    using Tok = boost::tokenizer<boost::char_separator<char>>;
    const boost::char_separator<char> sep(" \t\n");
    const auto& inputs = fuzz::get().tok;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& s : inputs) {
            Tok t(s.begin(), s.end(), sep);
            for (const auto& tok : t) sink += tok.size();
        }
    });
    std::cout << "timing.tokenizer=" << ns / double(inputs.size()) << '\n';
}

static void timing_hash_int()
{
    const auto& ints = fuzz::get().hash_ints;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (int v : ints) sink ^= boost::hash_value(v);
    });
    std::cout << "timing.hash_value_int=" << ns / double(ints.size()) << '\n';
}

static void timing_hash_string()
{
    const auto& strs = fuzz::get().hash_strings;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& s : strs) sink ^= boost::hash_value(s);
    });
    std::cout << "timing.hash_value_str=" << ns / double(strs.size()) << '\n';
}

static void timing_hash_combine()
{
    const auto& strs = fuzz::get().hash_strings;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        std::size_t seed = 0;
        for (const auto& s : strs) boost::hash_combine(seed, s);
        sink ^= seed;
    });
    std::cout << "timing.hash_combine=" << ns / double(strs.size()) << '\n';
}

static void timing_hash_range()
{
    const auto& v = fuzz::get().hash_range_ints;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        sink ^= boost::hash_range(v.begin(), v.end());
    });
    // Report per-element cost (range processed as a whole)
    std::cout << "timing.hash_range=" << ns / double(v.size()) << '\n';
}

static void timing_optional()
{
    const auto& vals = fuzz::get().opt_values;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& v : vals) {
            boost::optional<std::string> o = v;
            if (o) sink += o->size();
            boost::optional<std::string> empty = boost::none;
            sink += empty.value_or("DEFAULT").size();
        }
    });
    std::cout << "timing.optional=" << ns / double(vals.size()) << '\n';
}

static void timing_equals()
{
    const auto& pairs = fuzz::get().eq_pairs;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& [cand, query] : pairs) {
            auto r = boost::make_iterator_range(query.c_str(),
                                                query.c_str() + query.size());
            sink += boost::equals(cand, r) ? 1 : 0;
        }
    });
    std::cout << "timing.equals=" << ns / double(pairs.size()) << '\n';
}

static void timing_split()
{
    const auto& paths = fuzz::get().paths;
    volatile std::size_t sink = 0;

    double ns = time_ns(N, [&]{
        for (const auto& path : paths) {
            auto r = boost::make_iterator_range(path.c_str(),
                                                path.c_str() + path.size());
            auto itr = boost::make_split_iterator(
                r, boost::first_finder("/", boost::is_equal()));
            while (!itr.eof()) {
                auto seg = *itr;
                sink += std::size_t(std::distance(seg.begin(), seg.end()));
                ++itr;
            }
        }
    });
    std::cout << "timing.split=" << ns / double(paths.size()) << '\n';
}

static void timing_iter_facade()
{
    const auto& arr = fuzz::get().iter_array;
    volatile int sink = 0;

    double ns = time_ns(N, [&]{
        for (auto it  = DoubledIterator(arr.data());
                  it != DoubledIterator(arr.data() + arr.size()); ++it)
            sink += *it;
    });
    std::cout << "timing.iter_facade=" << ns / double(arr.size()) << '\n';
}

// ═════════════════════════════════════════════════════════════════════════════

int main(int argc, char** argv)
{
    bool do_correctness = true;
    bool do_timing      = true;

    if (argc >= 2) {
        std::string mode(argv[1]);
        if (mode == "--hash-correctness") { correctness_hash_ints(); return 0; }
        do_correctness = (mode == "--correctness");
        do_timing      = (mode == "--timing");
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
