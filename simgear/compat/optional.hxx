// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/optional.hpp using std::optional (C++20).
// Included automatically when SG_NO_BOOST is defined; provides the same
// boost::optional / boost::none API so call sites need no changes.

#pragma once

#include <optional>

namespace boost {

template<class T>
using optional = std::optional<T>;

// boost::none is the "empty optional" sentinel, analogous to std::nullopt
inline constexpr std::nullopt_t none = std::nullopt;

template<class T>
inline optional<T> make_optional(T&& v)
{
    return std::make_optional<T>(std::forward<T>(v));
}

template<class T>
inline optional<T> make_optional(bool condition, T&& v)
{
    return condition ? optional<T>(std::forward<T>(v)) : optional<T>{};
}

} // namespace boost
