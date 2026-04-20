// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Unit tests for simgear/compat/functional_hash.hxx

#ifdef SG_NO_BOOST
#  include "functional_hash.hxx"
#else
#  include <boost/functional/hash.hpp>
#endif

#include <simgear/misc/test_macros.hxx>
#include <string>
#include <vector>
#include <unordered_map>

// ──────────────────────────────────────────────────────────────────────────────
// hash_value for scalar types delegates to std::hash

static void test_hash_value_int()
{
    std::size_t h = boost::hash_value(42);
    SG_CHECK_EQUAL(h, std::hash<int>{}(42));
}

static void test_hash_value_string()
{
    std::string s = "simgear";
    std::size_t h = boost::hash_value(s);
    SG_CHECK_EQUAL(h, std::hash<std::string>{}(s));
}

// ──────────────────────────────────────────────────────────────────────────────
// hash_combine: seeding with two different values gives a different result

static void test_hash_combine_different()
{
    std::size_t s1 = 0, s2 = 0;
    boost::hash_combine(s1, 1);
    boost::hash_combine(s2, 2);
    SG_VERIFY(s1 != s2);
}

static void test_hash_combine_deterministic()
{
    std::size_t a = 0, b = 0;
    boost::hash_combine(a, std::string("abc"));
    boost::hash_combine(b, std::string("abc"));
    SG_CHECK_EQUAL(a, b);
}

static void test_hash_combine_order_matters()
{
    std::size_t ab = 0, ba = 0;
    boost::hash_combine(ab, 1);
    boost::hash_combine(ab, 2);
    boost::hash_combine(ba, 2);
    boost::hash_combine(ba, 1);
    SG_VERIFY(ab != ba);
}

// ──────────────────────────────────────────────────────────────────────────────
// hash_range

static void test_hash_range_empty()
{
    std::vector<int> v;
    std::size_t h = boost::hash_range(v.begin(), v.end());
    SG_CHECK_EQUAL(h, std::size_t{0});
}

static void test_hash_range_single()
{
    std::vector<int> v{7};
    std::size_t h1 = boost::hash_range(v.begin(), v.end());
    std::size_t h2 = 0;
    boost::hash_combine(h2, 7);
    SG_CHECK_EQUAL(h1, h2);
}

static void test_hash_range_multiple()
{
    std::vector<std::string> v{"a", "b", "c"};
    std::size_t ha = boost::hash_range(v.begin(), v.end());
    std::size_t hb = boost::hash_range(v.begin(), v.end());
    SG_CHECK_EQUAL(ha, hb);  // deterministic
}

static void test_hash_range_order_matters()
{
    std::vector<int> v1{1, 2, 3};
    std::vector<int> v2{3, 2, 1};
    std::size_t h1 = boost::hash_range(v1.begin(), v1.end());
    std::size_t h2 = boost::hash_range(v2.begin(), v2.end());
    SG_VERIFY(h1 != h2);
}

static void test_hash_range_seed_overload()
{
    std::vector<int> v{1, 2};
    std::size_t seed1 = 0, seed2 = 99;
    boost::hash_range(seed1, v.begin(), v.end());
    boost::hash_range(seed2, v.begin(), v.end());
    SG_VERIFY(seed1 != seed2);   // different starting seeds → different results
}

// ──────────────────────────────────────────────────────────────────────────────
// boost::hash<T> functor

static void test_hash_functor_int()
{
    boost::hash<int> h;
    SG_CHECK_EQUAL(h(42), h(42));     // deterministic
    SG_VERIFY(h(1) != h(2));          // distinct values
}

static void test_hash_functor_string()
{
    boost::hash<std::string> h;
    SG_CHECK_EQUAL(h("foo"), h("foo"));
    SG_VERIFY(h("foo") != h("bar"));
}

// ── ADL hash_value override ───────────────────────────────────────────────────
namespace test_ns {
    struct Widget { int id; };
    std::size_t hash_value(const Widget& w) { return boost::hash_value(w.id); }
}

static void test_hash_functor_adl()
{
    boost::hash<test_ns::Widget> h;
    test_ns::Widget w1{5}, w2{5}, w3{6};
    SG_CHECK_EQUAL(h(w1), h(w2));
    SG_VERIFY(h(w1) != h(w3));
}

// ── usable as unordered_map hasher ───────────────────────────────────────────
static void test_as_unordered_map_key()
{
    std::unordered_map<test_ns::Widget, int, boost::hash<test_ns::Widget>> m;
    m[{1}] = 100;
    m[{2}] = 200;
    SG_CHECK_EQUAL(m.at({1}), 100);
    SG_CHECK_EQUAL(m.at({2}), 200);
}

// ──────────────────────────────────────────────────────────────────────────────

int main(int, char**)
{
    test_hash_value_int();
    test_hash_value_string();
    test_hash_combine_different();
    test_hash_combine_deterministic();
    test_hash_combine_order_matters();
    test_hash_range_empty();
    test_hash_range_single();
    test_hash_range_multiple();
    test_hash_range_order_matters();
    test_hash_range_seed_overload();
    test_hash_functor_int();
    test_hash_functor_string();
    test_hash_functor_adl();
    test_as_unordered_map_key();
    return 0;
}
