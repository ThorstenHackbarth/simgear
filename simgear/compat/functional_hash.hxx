// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/functional/hash.hpp using std::hash (C++20).
// Provides boost::hash<T>, boost::hash_value(), boost::hash_combine(), and
// boost::hash_range() in the boost:: namespace so call sites need no changes.

#pragma once

#include <functional>
#include <cstddef>
#include <iterator>

namespace boost {

// ── hash_combine ─────────────────────────────────────────────────────────────
// Mix a single value's hash into an existing seed.
// Uses the golden-ratio constant from the original Boost implementation.

template<class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    seed ^= std::hash<T>{}(v) + std::size_t{0x9e3779b9u}
            + (seed << 6) + (seed >> 2);
}

// ── hash_range ────────────────────────────────────────────────────────────────
// Accumulate the hash of every element in [first, last) into seed.

template<class InputIt>
inline void hash_range(std::size_t& seed, InputIt first, InputIt last)
{
    for (; first != last; ++first)
        hash_combine(seed, *first);
}

// Convenience overload: compute from scratch and return the seed.
template<class InputIt>
inline std::size_t hash_range(InputIt first, InputIt last)
{
    std::size_t seed = 0;
    hash_range(seed, first, last);
    return seed;
}

// ── hash_value ────────────────────────────────────────────────────────────────
// Free-function API matching boost::hash_value(). Delegates to std::hash<T>.
// User-defined overloads (via ADL) shadow this template just like Boost.

template<class T>
inline std::size_t hash_value(const T& v)
{
    return std::hash<T>{}(v);
}

// ── hash<T> ───────────────────────────────────────────────────────────────────
// Callable hasher for use as a template argument (e.g. unordered_map).
// Mirrors boost::hash<T>: first tries ADL hash_value(), falls back to
// std::hash<T> for types without a custom overload.

namespace detail {

// Concept: T has a user-defined hash_value() reachable via ADL
template<class T>
concept HasHashValue = requires(const T& v) {
    { hash_value(v) } -> std::convertible_to<std::size_t>;
};

} // namespace detail

template<class T>
struct hash {
    std::size_t operator()(const T& v) const noexcept
    {
        if constexpr (detail::HasHashValue<T>)
            return hash_value(v);
        else
            return std::hash<T>{}(v);
    }
};

} // namespace boost
