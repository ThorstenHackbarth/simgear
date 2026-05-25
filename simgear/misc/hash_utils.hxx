// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
//
// Minimal hash utilities: hash_combine and hash_range using std::hash.

#pragma once
#include <cstddef>
#include <functional>
#include <utility>

namespace simgear {

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    seed ^= std::hash<T>{}(v) + std::size_t{0x9e3779b9u} + (seed << 6) + (seed >> 2);
}

// Pair overload: recursively combine both elements.
template <class A, class B>
inline void hash_combine(std::size_t& seed, const std::pair<A, B>& p)
{
    hash_combine(seed, p.first);
    hash_combine(seed, p.second);
}

template <class InputIt>
inline void hash_range(std::size_t& seed, InputIt first, InputIt last)
{
    for (; first != last; ++first)
        hash_combine(seed, *first);
}

template <class InputIt>
inline std::size_t hash_range(InputIt first, InputIt last)
{
    std::size_t seed = 0;
    hash_range(seed, first, last);
    return seed;
}

} // namespace simgear
