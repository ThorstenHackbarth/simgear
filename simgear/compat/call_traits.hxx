// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/call_traits.hpp using C++20 type_traits.
// Provides boost::call_traits<T> in the boost:: namespace so call sites
// need no changes.
//
// call_traits<T>::param_type selects the optimal function-parameter type:
//   - scalars and pointers     → passed by value  (T)
//   - reference types          → passed as-is     (T&)
//   - everything else          → passed by const& (const T&)

#pragma once

#include <type_traits>

namespace boost {

// Primary template: class/struct types – pass by const reference
template<class T, class Enable = void>
struct call_traits {
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using param_type      = const T&;
};

// Specialisation for scalar types (arithmetic, enum, pointer) – pass by value
template<class T>
struct call_traits<T, std::enable_if_t<std::is_scalar_v<T>>> {
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using param_type      = T;          // cheapest to copy
};

// Specialisation for lvalue references – forward the reference
template<class T>
struct call_traits<T&> {
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using param_type      = T&;
};

// Specialisation for rvalue references – forward the reference
template<class T>
struct call_traits<T&&> {
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using param_type      = T&&;
};

} // namespace boost
