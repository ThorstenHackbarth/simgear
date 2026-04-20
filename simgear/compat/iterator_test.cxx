// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Unit tests for:
//   simgear/compat/iterator_facade.hxx
//   simgear/compat/iterator_adaptor.hxx
//   simgear/compat/mpl_has_xxx.hxx

#ifdef SG_NO_BOOST
#  include "iterator_facade.hxx"
#  include "iterator_adaptor.hxx"
#  include "mpl_has_xxx.hxx"
#else
#  include <boost/iterator/iterator_facade.hpp>
#  include <boost/iterator/iterator_adaptor.hpp>
#  include <boost/mpl/has_xxx.hpp>
#endif

#include <simgear/misc/test_macros.hxx>
#include <vector>
#include <numeric>
#include <string>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
// iterator_facade tests
// ═════════════════════════════════════════════════════════════════════════════

// A simple forward iterator over int* that doubles every value on dereference
class DoubledIterator
    : public boost::iterator_facade<
          DoubledIterator,
          int,
          boost::forward_traversal_tag>
{
public:
    DoubledIterator() : ptr_(nullptr) {}
    explicit DoubledIterator(int* p) : ptr_(p) {}

private:
    friend class boost::iterator_core_access;

    int dereference() const { return *ptr_ * 2; }
    void increment()        { ++ptr_; }
    bool equal(const DoubledIterator& o) const { return ptr_ == o.ptr_; }

    int* ptr_;
};

static void test_facade_forward_iteration()
{
    int arr[] = {1, 2, 3};
    DoubledIterator begin(arr), end(arr + 3);

    std::vector<int> result;
    for (auto it = begin; it != end; ++it)
        result.push_back(*it);

    SG_CHECK_EQUAL(result.size(), std::size_t{3});
    SG_CHECK_EQUAL(result[0], 2);
    SG_CHECK_EQUAL(result[1], 4);
    SG_CHECK_EQUAL(result[2], 6);
}

static void test_facade_post_increment()
{
    int arr[] = {10, 20};
    DoubledIterator it(arr);
    auto old = it++;
    SG_CHECK_EQUAL(*old, 20);  // 10 * 2
    SG_CHECK_EQUAL(*it,  40);  // 20 * 2
}

// A bidirectional iterator over a std::vector<std::string>
// that returns string lengths (proxy reference: returns by value)
class LengthIterator
    : public boost::iterator_facade<
          LengthIterator,
          std::size_t,
          boost::bidirectional_traversal_tag,
          std::size_t>  // Reference = value type (proxy)
{
public:
    LengthIterator() : it_() {}
    explicit LengthIterator(std::vector<std::string>::const_iterator it) : it_(it) {}

private:
    friend class boost::iterator_core_access;

    std::size_t dereference() const { return it_->size(); }
    void increment()  { ++it_; }
    void decrement()  { --it_; }
    bool equal(const LengthIterator& o) const { return it_ == o.it_; }

    std::vector<std::string>::const_iterator it_;
};

static void test_facade_bidirectional()
{
    std::vector<std::string> v{"hi", "hello", "hey"};
    LengthIterator begin(v.begin()), end(v.end());

    // Forward
    std::vector<std::size_t> fwd;
    for (auto it = begin; it != end; ++it) fwd.push_back(*it);
    SG_CHECK_EQUAL(fwd[0], std::size_t{2});
    SG_CHECK_EQUAL(fwd[1], std::size_t{5});
    SG_CHECK_EQUAL(fwd[2], std::size_t{3});

    // Backward from end
    auto it = end;
    --it; SG_CHECK_EQUAL(*it, std::size_t{3});
    --it; SG_CHECK_EQUAL(*it, std::size_t{5});
}

static void test_facade_equality()
{
    int arr[] = {1};
    DoubledIterator a(arr), b(arr), c(arr + 1);
    SG_VERIFY(a == b);
    SG_VERIFY(a != c);
}

// ═════════════════════════════════════════════════════════════════════════════
// iterator_adaptor tests
// ═════════════════════════════════════════════════════════════════════════════

// Mirrors EffectGeode.hxx: adapts vector<string*>::iterator to yield string length
class LengthAdaptor
    : public boost::iterator_adaptor<
          LengthAdaptor,
          std::vector<std::string>::iterator,
          std::size_t,                        // Value
          boost::use_default,                 // Category = from Base
          std::size_t>                        // Reference = value (proxy)
{
public:
    LengthAdaptor() = default;
    explicit LengthAdaptor(std::vector<std::string>::iterator it)
        : LengthAdaptor::iterator_adaptor_(it) {}

private:
    friend class boost::iterator_core_access;

    std::size_t dereference() const { return base_reference()->size(); }
};

static void test_adaptor_custom_dereference()
{
    std::vector<std::string> v{"ab", "cde", "f"};
    LengthAdaptor begin(v.begin()), end(v.end());

    std::vector<std::size_t> lengths(begin, end);
    SG_CHECK_EQUAL(lengths.size(), std::size_t{3});
    SG_CHECK_EQUAL(lengths[0], std::size_t{2});
    SG_CHECK_EQUAL(lengths[1], std::size_t{3});
    SG_CHECK_EQUAL(lengths[2], std::size_t{1});
}

static void test_adaptor_base_increment_used()
{
    std::vector<std::string> v{"x", "y"};
    LengthAdaptor it(v.begin());
    SG_CHECK_EQUAL(*it, std::size_t{1});
    ++it;
    SG_CHECK_EQUAL(*it, std::size_t{1});
}

static void test_adaptor_equality()
{
    std::vector<std::string> v{"a"};
    LengthAdaptor a(v.begin()), b(v.begin()), end(v.end());
    SG_VERIFY(a == b);
    SG_VERIFY(a != end);
}

static void test_adaptor_with_std_algorithm()
{
    std::vector<std::string> v{"one", "two", "three"};
    LengthAdaptor begin(v.begin()), end(v.end());
    auto it = std::find(begin, end, std::size_t{3});
    SG_VERIFY(it != end);
    SG_CHECK_EQUAL(*it, std::size_t{3});
}

// ═════════════════════════════════════════════════════════════════════════════
// mpl_has_xxx tests (BOOST_MPL_HAS_XXX_TRAIT_DEF)
// ═════════════════════════════════════════════════════════════════════════════

namespace test_mpl {
    BOOST_MPL_HAS_XXX_TRAIT_DEF(element_type)

    struct WithElementType  { using element_type = int; };
    struct WithoutElementType {};
}

static void test_has_xxx_positive()
{
    static_assert(test_mpl::has_element_type<test_mpl::WithElementType>::value);
}

static void test_has_xxx_negative()
{
    static_assert(!test_mpl::has_element_type<test_mpl::WithoutElementType>::value);
}

static void test_has_xxx_primitive()
{
    static_assert(!test_mpl::has_element_type<int>::value);
}

// ═════════════════════════════════════════════════════════════════════════════

int main(int, char**)
{
    test_facade_forward_iteration();
    test_facade_post_increment();
    test_facade_bidirectional();
    test_facade_equality();

    test_adaptor_custom_dereference();
    test_adaptor_base_increment_used();
    test_adaptor_equality();
    test_adaptor_with_std_algorithm();

    test_has_xxx_positive();
    test_has_xxx_negative();
    test_has_xxx_primitive();

    return 0;
}
