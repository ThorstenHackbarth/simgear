// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/iterator/iterator_adaptor.hpp (C++20).
// Provides boost::iterator_adaptor<Derived,Base,Value,CategoryOrTraversal,
//                                  Reference,Difference>
// and boost::use_default in the boost:: namespace.
//
// Derived inherits from iterator_adaptor and optionally overrides:
//   reference dereference() const           -- customise dereferencing
//   void      increment() / decrement()     -- customise advance
//   bool      equal(const Derived&) const   -- customise comparison
// The constructor taking a Base iterator is forwarded to iterator_adaptor_.
//
// base_reference() gives protected access to the stored base iterator.

#pragma once

#include "iterator_facade.hxx"
#include <iterator>
#include <type_traits>

namespace boost {

// Sentinel tag: "inherit this trait from the Base iterator"
struct use_default {};

namespace detail {

// Resolve use_default for CategoryOrTraversal
template<class Base, class CategoryOrTraversal>
using resolved_category = std::conditional_t<
    std::is_same_v<CategoryOrTraversal, use_default>,
    typename std::iterator_traits<Base>::iterator_category,
    CategoryOrTraversal>;

// Resolve use_default for Reference
template<class Base, class Value, class Ref>
using resolved_reference = std::conditional_t<
    std::is_same_v<Ref, use_default>,
    std::conditional_t<
        std::is_same_v<Value, use_default>,
        typename std::iterator_traits<Base>::reference,
        Value&>,
    Ref>;

// Resolve use_default for Value
template<class Base, class Value>
using resolved_value = std::conditional_t<
    std::is_same_v<Value, use_default>,
    typename std::iterator_traits<Base>::value_type,
    Value>;

} // namespace detail

// ── iterator_adaptor ──────────────────────────────────────────────────────────

template< class Derived
        , class Base
        , class Value              = use_default
        , class CategoryOrTraversal = use_default
        , class Reference          = use_default
        , class Difference         = std::ptrdiff_t >
class iterator_adaptor
    : public iterator_facade<
          Derived,
          detail::resolved_value<Base, Value>,
          detail::resolved_category<Base, CategoryOrTraversal>,
          detail::resolved_reference<Base, Value, Reference>,
          Difference>
{
    using facade_t = iterator_facade<
        Derived,
        detail::resolved_value<Base, Value>,
        detail::resolved_category<Base, CategoryOrTraversal>,
        detail::resolved_reference<Base, Value, Reference>,
        Difference>;

public:
    using base_type = Base;
    using typename facade_t::reference;
    using typename facade_t::difference_type;

    // Boost names the base class iterator_adaptor_ so Derived constructors can
    // write: Derived(Base b) : Derived::iterator_adaptor_(b) {}
    using iterator_adaptor_ = iterator_adaptor;

    iterator_adaptor() = default;

    explicit iterator_adaptor(const Base& iter) : base_(iter) {}

    Base const& base() const { return base_; }

protected:
    // Mutable access to the stored base iterator for use by Derived
    Base& base_reference()             { return base_; }
    Base const& base_reference() const { return base_; }

    // Default implementations – Derived may shadow any of these.
    // iterator_core_access calls them on the Derived type via CRTP.

    reference dereference() const { return *base_; }

    void increment() { ++base_; }
    void decrement() { --base_; }

    template<class OtherDerived>
    bool equal(const iterator_adaptor<OtherDerived, Base,
                                      Value, CategoryOrTraversal,
                                      Reference, Difference>& other) const
    {
        return base_ == other.base_;
    }

public:
    // Random-access operators – delegate directly to the Base iterator.
    // Only well-formed when Base itself supports random access; the compiler
    // produces a clear diagnostic otherwise.

    Derived& operator+=(difference_type n)
    {
        base_ += n;
        return static_cast<Derived&>(*this);
    }

    Derived& operator-=(difference_type n)
    {
        base_ -= n;
        return static_cast<Derived&>(*this);
    }

    friend Derived operator+(Derived d, difference_type n) { d += n; return d; }
    friend Derived operator+(difference_type n, Derived d) { d += n; return d; }
    friend Derived operator-(Derived d, difference_type n) { d -= n; return d; }

    template<class OtherDerived>
    difference_type operator-(
        const iterator_adaptor<OtherDerived, Base,
                               Value, CategoryOrTraversal,
                               Reference, Difference>& other) const
    {
        return base_ - other.base();
    }

    decltype(auto) operator[](difference_type n) const
    {
        return *(static_cast<const Derived&>(*this) + n);
    }

private:
    friend class iterator_core_access;
    Base base_;
};

} // namespace boost
