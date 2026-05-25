// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
//
// Minimal type utilities using std::type_traits.

#pragma once
#include <type_traits>

namespace simgear {

// Selects the optimal function-parameter type for T:
//   scalar / pointer / enum  → T        (cheap to copy)
//   reference types          → T        (forwarded as-is)
//   class / struct           → const T& (avoid copy)
template <class T>
using param_type_t = std::conditional_t<
    std::is_reference_v<T>, T,
    std::conditional_t<std::is_scalar_v<T>, T, const T&>>;

} // namespace simgear
