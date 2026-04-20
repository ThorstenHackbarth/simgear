// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Unit tests for simgear/compat/tokenizer.hxx

#ifdef SG_NO_BOOST
#  include "tokenizer.hxx"
#else
#  include <boost/tokenizer.hpp>
#endif

#include <simgear/misc/test_macros.hxx>
#include <string>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
// Helpers

static std::vector<std::string>
tokenize(const std::string& s, const char* dropped = " \t\n")
{
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    Tok t(s.begin(), s.end(), boost::char_separator<char>(dropped));
    return {t.begin(), t.end()};
}

// ──────────────────────────────────────────────────────────────────────────────

static void test_empty_string()
{
    auto v = tokenize("");
    SG_CHECK_EQUAL(v.size(), std::size_t{0});
}

static void test_single_token()
{
    auto v = tokenize("hello");
    SG_CHECK_EQUAL(v.size(), std::size_t{1});
    SG_CHECK_EQUAL(v[0], std::string("hello"));
}

static void test_two_tokens_space()
{
    auto v = tokenize("meet slice");
    SG_CHECK_EQUAL(v.size(), std::size_t{2});
    SG_CHECK_EQUAL(v[0], std::string("meet"));
    SG_CHECK_EQUAL(v[1], std::string("slice"));
}

static void test_multiple_whitespace_types()
{
    auto v = tokenize("a\tb\nc");
    SG_CHECK_EQUAL(v.size(), std::size_t{3});
    SG_CHECK_EQUAL(v[0], std::string("a"));
    SG_CHECK_EQUAL(v[1], std::string("b"));
    SG_CHECK_EQUAL(v[2], std::string("c"));
}

static void test_leading_trailing_whitespace()
{
    auto v = tokenize("  hello world  ");
    SG_CHECK_EQUAL(v.size(), std::size_t{2});
    SG_CHECK_EQUAL(v[0], std::string("hello"));
    SG_CHECK_EQUAL(v[1], std::string("world"));
}

static void test_consecutive_delimiters_no_empty_tokens()
{
    auto v = tokenize("a   b");
    SG_CHECK_EQUAL(v.size(), std::size_t{2});
    SG_CHECK_EQUAL(v[0], std::string("a"));
    SG_CHECK_EQUAL(v[1], std::string("b"));
}

static void test_only_delimiters()
{
    auto v = tokenize("   \t  ");
    SG_CHECK_EQUAL(v.size(), std::size_t{0});
}

static void test_css_border_pattern()
{
    // Mirrors the CSSBorder.cxx usage: "5 10% 20 none"
    auto v = tokenize("5 10% 20 none");
    SG_CHECK_EQUAL(v.size(), std::size_t{4});
    SG_CHECK_EQUAL(v[0], std::string("5"));
    SG_CHECK_EQUAL(v[1], std::string("10%"));
    SG_CHECK_EQUAL(v[2], std::string("20"));
    SG_CHECK_EQUAL(v[3], std::string("none"));
}

static void test_svg_preserveaspectratio_pattern()
{
    // Mirrors the SVGpreserveAspectRatio.cxx usage: "xMidYMid meet"
    auto v = tokenize("xMidYMid meet");
    SG_CHECK_EQUAL(v.size(), std::size_t{2});
    SG_CHECK_EQUAL(v[0], std::string("xMidYMid"));
    SG_CHECK_EQUAL(v[1], std::string("meet"));
}

static void test_dereference_string_interface()
{
    // *tok gives std::string with begin()/end()/rbegin()
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    Tok t(std::string("50%").cbegin(), std::string("50%").cend(),
          boost::char_separator<char>(" "));
    auto it = t.begin();
    SG_VERIFY(it != t.end());
    SG_CHECK_EQUAL(*it, std::string("50%"));
    SG_CHECK_EQUAL(*it->rbegin(), '%');        // last char is '%'
    SG_CHECK_EQUAL(*it->begin(),  '5');        // first char is '5'
}

static void test_arrow_member_access()
{
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    Tok t(std::string("alpha").cbegin(), std::string("alpha").cend(),
          boost::char_separator<char>(" "));
    auto it = t.begin();
    SG_CHECK_EQUAL(it->length(), std::size_t{5});
}

static void test_current_token()
{
    // SVGpreserveAspectRatio.cxx calls tok.current_token()
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    std::string s = "defer xMidYMid meet";
    Tok t(s.begin(), s.end(), boost::char_separator<char>(" \t\n"));
    auto it = t.begin();
    SG_CHECK_EQUAL(it.current_token(), std::string("defer"));
    ++it;
    SG_CHECK_EQUAL(it.current_token(), std::string("xMidYMid"));
}

static void test_range_based_for()
{
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    std::string s = "one two three";
    Tok t(s.begin(), s.end(), boost::char_separator<char>(" "));
    std::vector<std::string> v;
    for (const auto& tok : t)
        v.push_back(tok);
    SG_CHECK_EQUAL(v.size(), std::size_t{3});
    SG_CHECK_EQUAL(v[0], std::string("one"));
    SG_CHECK_EQUAL(v[1], std::string("two"));
    SG_CHECK_EQUAL(v[2], std::string("three"));
}

static void test_reiteration()
{
    // Same tokenizer object can be iterated twice
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    std::string s = "a b c";
    Tok t(s.begin(), s.end(), boost::char_separator<char>(" "));
    auto v1 = std::vector<std::string>(t.begin(), t.end());
    auto v2 = std::vector<std::string>(t.begin(), t.end());
    SG_CHECK_EQUAL(v1, v2);
}

static void test_custom_delimiter()
{
    auto v = tokenize("a,b,,c", ",");
    SG_CHECK_EQUAL(v.size(), std::size_t{3});
    SG_CHECK_EQUAL(v[0], std::string("a"));
    SG_CHECK_EQUAL(v[1], std::string("b"));
    SG_CHECK_EQUAL(v[2], std::string("c"));
}

static void test_iterator_equality()
{
    typedef boost::tokenizer<boost::char_separator<char>> Tok;
    Tok t(std::string("x").begin(), std::string("x").end(),
          boost::char_separator<char>(" "));
    SG_VERIFY(t.end() == t.end());
    SG_VERIFY(t.begin() != t.end());
}

// ──────────────────────────────────────────────────────────────────────────────

int main(int, char**)
{
    test_empty_string();
    test_single_token();
    test_two_tokens_space();
    test_multiple_whitespace_types();
    test_leading_trailing_whitespace();
    test_consecutive_delimiters_no_empty_tokens();
    test_only_delimiters();
    test_css_border_pattern();
    test_svg_preserveaspectratio_pattern();
    test_dereference_string_interface();
    test_arrow_member_access();
    test_current_token();
    test_range_based_for();
    test_reiteration();
    test_custom_delimiter();
    test_iterator_equality();
    return 0;
}
