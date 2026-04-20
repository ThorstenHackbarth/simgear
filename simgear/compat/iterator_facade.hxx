// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/iterator/iterator_facade.hpp (C++20).
// Provides boost::iterator_facade<Derived,Value,CategoryOrTraversal,Reference,Difference>
// and boost::iterator_core_access in the boost:: namespace.
//
// The Derived class must implement (all private, with iterator_core_access as friend):
//   reference dereference() const
//   void      increment()
//   void      decrement()           (for bidirectional)
//   bool      equal(const Derived&) const
//
// Traversal tags are aliased to std iterator categories.

#pragma once

#include <iterator>
#include <type_traits>

namespace boost {

// Traversal category aliases (Boost uses its own tags; we alias std equivalents)
using forward_traversal_tag       = std::forward_iterator_tag;
using bidirectional_traversal_tag = std::bidirectional_iterator_tag;
using random_access_traversal_tag = std::random_access_iterator_tag;

// ── iterator_core_access ──────────────────────────────────────────────────────
// Friend class that Derived declares so iterator_facade can call its private
// dereference/increment/decrement/equal methods.

struct iterator_core_access
{
    template<class Facade>
    static decltype(auto) dereference(const Facade& f)
    {
        return f.dereference();
    }

    template<class Facade>
    static void increment(Facade& f) { f.increment(); }

    template<class Facade>
    static void decrement(Facade& f) { f.decrement(); }

    template<class F1, class F2>
    static bool equal(const F1& a, const F2& b)
    {
        return a.equal(b);
    }
};

// ── iterator_facade ───────────────────────────────────────────────────────────

template< class Derived
        , class Value
        , class CategoryOrTraversal
        , class Reference  = Value&
        , class Difference = std::ptrdiff_t >
class iterator_facade
{
public:
    using value_type        = std::remove_const_t<Value>;
    using reference         = Reference;
    using pointer           = Value*;
    using difference_type   = Difference;
    using iterator_category = CategoryOrTraversal;

    // ── dereference ──────────────────────────────────────────────────────────
    // Returns decltype(auto) so proxy iterators whose dereference() returns by
    // value (e.g. Reference = int instead of int&) compile without truncation.
    // iterator_traits::reference still reflects the declared Reference type.

    decltype(auto) operator*() const
    {
        return iterator_core_access::dereference(derived());
    }

    // operator-> only when reference is a real reference (not a proxy value)
    auto operator->() const
    {
        // For proxy iterators that return by value, store the value and return ptr.
        // The struct below avoids dangling pointer for proxy references.
        struct arrow_proxy {
            mutable value_type val;
            value_type* operator->() const { return &val; }
        };
        if constexpr (std::is_reference_v<reference>)
            return std::addressof(**this);
        else
            return arrow_proxy{**this};
    }

    // ── increment / decrement ────────────────────────────────────────────────

    Derived& operator++()
    {
        iterator_core_access::increment(derived());
        return derived();
    }

    Derived operator++(int)
    {
        Derived tmp(derived());
        iterator_core_access::increment(derived());
        return tmp;
    }

    Derived& operator--()
    {
        iterator_core_access::decrement(derived());
        return derived();
    }

    Derived operator--(int)
    {
        Derived tmp(derived());
        iterator_core_access::decrement(derived());
        return tmp;
    }

    // ── equality ─────────────────────────────────────────────────────────────

    template<class OtherDerived>
    bool operator==(const OtherDerived& other) const
    {
        return iterator_core_access::equal(derived(), other);
    }

    template<class OtherDerived>
    bool operator!=(const OtherDerived& other) const
    {
        return !operator==(other);
    }

private:
    Derived&       derived()       { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

} // namespace boost
