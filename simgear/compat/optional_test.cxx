// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Unit tests for simgear/compat/optional.hxx
// Verifies that the sg_compat/boost::optional replacement matches
// boost::optional semantics for all use-patterns found in simgear.

// Force the replacement header regardless of build settings
#ifdef SG_NO_BOOST
#  include "optional.hxx"
#else
#  include <boost/optional.hpp>
#endif

#include <simgear/misc/test_macros.hxx>
#include <string>
#include <vector>
#include <memory>

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

static void test_default_construction()
{
    boost::optional<int> o;
    SG_VERIFY(!o);
    SG_VERIFY(!o.has_value());
}

static void test_value_construction()
{
    boost::optional<int> o(42);
    SG_VERIFY(o);
    SG_VERIFY(o.has_value());
    SG_CHECK_EQUAL(*o, 42);
    SG_CHECK_EQUAL(o.value(), 42);
}

static void test_string_value()
{
    boost::optional<std::string> o(std::string("hello"));
    SG_VERIFY(o);
    SG_CHECK_EQUAL(*o, std::string("hello"));
}

static void test_none_construction()
{
    boost::optional<int> o(boost::none);
    SG_VERIFY(!o);
}

static void test_assignment_value()
{
    boost::optional<int> o;
    o = 7;
    SG_VERIFY(o);
    SG_CHECK_EQUAL(*o, 7);
}

static void test_assignment_none()
{
    boost::optional<int> o(5);
    o = boost::none;
    SG_VERIFY(!o);
}

static void test_value_or()
{
    boost::optional<int> empty;
    boost::optional<int> valued(99);
    SG_CHECK_EQUAL(empty.value_or(0),   0);
    SG_CHECK_EQUAL(valued.value_or(0), 99);
}

static void test_copy_construction()
{
    boost::optional<int> a(3);
    boost::optional<int> b(a);
    SG_VERIFY(b);
    SG_CHECK_EQUAL(*b, 3);
}

static void test_move_construction()
{
    boost::optional<std::string> a(std::string("move_me"));
    boost::optional<std::string> b(std::move(a));
    SG_VERIFY(b);
    SG_CHECK_EQUAL(*b, std::string("move_me"));
}

static void test_equality()
{
    boost::optional<int> e1, e2;
    boost::optional<int> v1(1), v2(1), v3(2);
    SG_VERIFY(e1 == e2);          // both empty
    SG_VERIFY(!(e1 == v1));       // empty != valued
    SG_VERIFY(v1 == v2);          // same value
    SG_VERIFY(!(v1 == v3));       // different values
}

static void test_make_optional()
{
    auto o = boost::make_optional(55);
    SG_VERIFY(o);
    SG_CHECK_EQUAL(*o, 55);
}

static void test_make_optional_conditional()
{
    auto yes = boost::make_optional(true,  10);
    auto no  = boost::make_optional(false, 10);
    SG_VERIFY(yes);
    SG_VERIFY(!no);
    SG_CHECK_EQUAL(*yes, 10);
}

static void test_pointer_dereference()
{
    struct S { int x; };
    boost::optional<S> o(S{7});
    SG_CHECK_EQUAL(o->x, 7);
}

// Simulates the pattern used in lru_cache.hxx: return boost::none from a method
static boost::optional<std::string> find_in_list(const std::vector<std::string>& v,
                                                  const std::string& target)
{
    for (const auto& s : v)
        if (s == target)
            return s;
    return boost::none;
}

static void test_return_none_pattern()
{
    std::vector<std::string> v{"alpha", "beta", "gamma"};
    auto found   = find_in_list(v, "beta");
    auto missing = find_in_list(v, "delta");
    SG_VERIFY(found);
    SG_CHECK_EQUAL(*found, std::string("beta"));
    SG_VERIFY(!missing);
}

// ──────────────────────────────────────────────────────────────────────────────

int main(int, char**)
{
    test_default_construction();
    test_value_construction();
    test_string_value();
    test_none_construction();
    test_assignment_value();
    test_assignment_none();
    test_value_or();
    test_copy_construction();
    test_move_construction();
    test_equality();
    test_make_optional();
    test_make_optional_conditional();
    test_pointer_dereference();
    test_return_none_pattern();
    return 0;
}
