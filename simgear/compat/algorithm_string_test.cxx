// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Unit tests for simgear/compat/algorithm_string.hxx

#ifdef SG_NO_BOOST
#  include "algorithm_string.hxx"
#else
#  include <boost/algorithm/string/find_iterator.hpp>
#  include <boost/algorithm/string/predicate.hpp>
#  include <boost/range.hpp>
#endif

#include <simgear/misc/test_macros.hxx>
#include <cstring>
#include <string>
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
// iterator_range tests
// ═════════════════════════════════════════════════════════════════════════════

static void test_range_from_string_iterators()
{
    std::string s = "hello";
    boost::iterator_range<std::string::iterator> r(s.begin(), s.end());
    SG_VERIFY(!r.empty());
    SG_CHECK_EQUAL(r.size(), std::ptrdiff_t{5});
    SG_CHECK_EQUAL(*r.begin(), 'h');
    SG_CHECK_EQUAL(r[0], 'h');
    SG_CHECK_EQUAL(r[4], 'o');
}

static void test_range_from_char_ptr()
{
    const char* s = "world";
    boost::iterator_range<const char*> r(s, s + 5);
    SG_VERIFY(!r.empty());
    SG_CHECK_EQUAL(r.size(), std::ptrdiff_t{5});
    SG_CHECK_EQUAL(*r.begin(), 'w');
}

static void test_range_empty()
{
    std::string s = "abc";
    boost::iterator_range<std::string::iterator> r(s.begin(), s.begin());
    SG_VERIFY(r.empty());
    SG_CHECK_EQUAL(r.size(), std::ptrdiff_t{0});
}

static void test_make_iterator_range()
{
    const char* s = "foo";
    auto r = boost::make_iterator_range(s, s + 3);
    SG_VERIFY(!r.empty());
    SG_CHECK_EQUAL(r.size(), std::ptrdiff_t{3});
    SG_CHECK_EQUAL(*r.begin(), 'f');
}

static void test_range_begin_dereference()
{
    // Mirrors getNode: *path.begin() to check first char
    const char* path = "/foo/bar";
    auto r = boost::make_iterator_range(path, path + strlen(path));
    SG_CHECK_EQUAL(*r.begin(), '/');
}

// ═════════════════════════════════════════════════════════════════════════════
// equals tests
// ═════════════════════════════════════════════════════════════════════════════

static void test_equals_range_and_cstring()
{
    // Mirrors find_node_aux: equals(name, ".")
    const char* s = "foo";
    auto r = boost::make_iterator_range(s, s + 3);
    SG_VERIFY( boost::equals(r, "foo"));
    SG_VERIFY(!boost::equals(r, "fo"));
    SG_VERIFY(!boost::equals(r, "fooo"));
    SG_VERIFY(!boost::equals(r, "bar"));
}

static void test_equals_dot_dotdot()
{
    const char* dot    = ".";
    const char* dotdot = "..";
    auto r_dot    = boost::make_iterator_range(dot,    dot    + 1);
    auto r_dotdot = boost::make_iterator_range(dotdot, dotdot + 2);
    SG_VERIFY( boost::equals(r_dot,    "."));
    SG_VERIFY(!boost::equals(r_dot,    ".."));
    SG_VERIFY( boost::equals(r_dotdot, ".."));
    SG_VERIFY(!boost::equals(r_dotdot, "."));
}

static void test_equals_string_and_range()
{
    // Mirrors find_child: boost::equals(node->getNameString(), name)
    std::string name = "engine";
    const char* path = "engine";
    auto r = boost::make_iterator_range(path, path + 6);
    SG_VERIFY( boost::equals(name, r));
    SG_VERIFY(!boost::equals(std::string("engIne"), r));
}

static void test_equals_empty()
{
    const char* s = "";
    auto r = boost::make_iterator_range(s, s);
    SG_VERIFY( boost::equals(r, ""));
    SG_VERIFY(!boost::equals(r, "x"));
    SG_VERIFY(!boost::equals(std::string("x"), r));
}

// ═════════════════════════════════════════════════════════════════════════════
// is_equal tests
// ═════════════════════════════════════════════════════════════════════════════

static void test_is_equal_predicate()
{
    boost::is_equal pred;
    SG_VERIFY( pred('/', '/'));
    SG_VERIFY(!pred('/', 'x'));
    SG_VERIFY( pred('a', 'a'));
}

// ═════════════════════════════════════════════════════════════════════════════
// first_finder tests
// ═════════════════════════════════════════════════════════════════════════════

static void test_first_finder_finds_char()
{
    const char* s = "foo/bar";
    auto r = boost::make_iterator_range(s, s + strlen(s));
    auto finder = boost::first_finder("/", boost::is_equal());
    auto match = finder(r);
    SG_VERIFY(!match.empty());
    SG_CHECK_EQUAL(*match.begin(), '/');
}

static void test_first_finder_not_found()
{
    const char* s = "foobar";
    auto r = boost::make_iterator_range(s, s + strlen(s));
    auto finder = boost::first_finder("/", boost::is_equal());
    auto match = finder(r);
    SG_VERIFY(match.begin() == r.end());
}

static void test_first_finder_at_start()
{
    const char* s = "/foo";
    auto r = boost::make_iterator_range(s, s + strlen(s));
    auto finder = boost::first_finder("/", boost::is_equal());
    auto match = finder(r);
    SG_VERIFY(!match.empty());
    SG_VERIFY(match.begin() == r.begin());
}

// ═════════════════════════════════════════════════════════════════════════════
// make_split_iterator tests
// ═════════════════════════════════════════════════════════════════════════════

static std::vector<std::string> collect_segments(const char* path)
{
    auto r = boost::make_iterator_range(path, path + strlen(path));
    auto itr = boost::make_split_iterator(r, boost::first_finder("/", boost::is_equal()));
    std::vector<std::string> segs;
    while (!itr.eof()) {
        auto tok = *itr;
        segs.emplace_back(tok.begin(), tok.end());
        ++itr;
    }
    return segs;
}

static void test_split_simple_path()
{
    auto segs = collect_segments("foo/bar/baz");
    SG_CHECK_EQUAL(segs.size(), std::size_t{3});
    SG_CHECK_EQUAL(segs[0], std::string{"foo"});
    SG_CHECK_EQUAL(segs[1], std::string{"bar"});
    SG_CHECK_EQUAL(segs[2], std::string{"baz"});
}

static void test_split_absolute_path()
{
    // /foo/bar -> ["", "foo", "bar"]
    auto segs = collect_segments("/foo/bar");
    SG_CHECK_EQUAL(segs.size(), std::size_t{3});
    SG_CHECK_EQUAL(segs[0], std::string{""});
    SG_CHECK_EQUAL(segs[1], std::string{"foo"});
    SG_CHECK_EQUAL(segs[2], std::string{"bar"});
}

static void test_split_single_segment()
{
    auto segs = collect_segments("name");
    SG_CHECK_EQUAL(segs.size(), std::size_t{1});
    SG_CHECK_EQUAL(segs[0], std::string{"name"});
}

static void test_split_trailing_slash()
{
    auto segs = collect_segments("foo/");
    SG_CHECK_EQUAL(segs.size(), std::size_t{2});
    SG_CHECK_EQUAL(segs[0], std::string{"foo"});
    SG_CHECK_EQUAL(segs[1], std::string{""});
}

static void test_split_eof_detection()
{
    const char* s = "a/b";
    auto r = boost::make_iterator_range(s, s + 3);
    auto itr = boost::make_split_iterator(r, boost::first_finder("/", boost::is_equal()));
    SG_VERIFY(!itr.eof());
    ++itr;
    SG_VERIFY(!itr.eof());
    ++itr;
    SG_VERIFY(itr.eof());
}

static void test_split_arrow_operator()
{
    // Mirrors find_node_aux: itr->empty()
    const char* s = "/foo";
    auto r = boost::make_iterator_range(s, s + strlen(s));
    auto itr = boost::make_split_iterator(r, boost::first_finder("/", boost::is_equal()));
    SG_VERIFY(itr->empty());  // first segment is "" (before leading /)
    ++itr;
    SG_VERIFY(!itr->empty()); // "foo"
}

static void test_split_pre_increment_returns_ref()
{
    // Mirrors: while (!(++itr).eof())
    const char* s = "a/b/c";
    auto r = boost::make_iterator_range(s, s + 5);
    auto itr = boost::make_split_iterator(r, boost::first_finder("/", boost::is_equal()));
    SG_VERIFY(!(++itr).eof());
    SG_VERIFY(!(++itr).eof());
    SG_VERIFY((++itr).eof());
}

// ═════════════════════════════════════════════════════════════════════════════

int main(int, char**)
{
    test_range_from_string_iterators();
    test_range_from_char_ptr();
    test_range_empty();
    test_make_iterator_range();
    test_range_begin_dereference();

    test_equals_range_and_cstring();
    test_equals_dot_dotdot();
    test_equals_string_and_range();
    test_equals_empty();

    test_is_equal_predicate();

    test_first_finder_finds_char();
    test_first_finder_not_found();
    test_first_finder_at_start();

    test_split_simple_path();
    test_split_absolute_path();
    test_split_single_segment();
    test_split_trailing_slash();
    test_split_eof_detection();
    test_split_arrow_operator();
    test_split_pre_increment_returns_ref();

    return 0;
}
