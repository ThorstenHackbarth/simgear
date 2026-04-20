// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/mpl/has_xxx.hpp using C++20 std::void_t.
// Provides BOOST_MPL_HAS_XXX_TRAIT_DEF(name) macro in the boost:: namespace
// so call sites need no changes.
//
// The macro generates a struct `has_<name><T>` with a boolean `value` member
// that is true iff T has a nested type named `name`.

#pragma once

#include <type_traits>

// BOOST_MPL_HAS_XXX_TRAIT_DEF(member_name)
// Generates:
//   template<class T, class = void>
//   struct has_<member_name> : std::false_type {};
//   template<class T>
//   struct has_<member_name><T, std::void_t<typename T::member_name>> : std::true_type {};

#define BOOST_MPL_HAS_XXX_TRAIT_DEF(name)                                    \
    template<class _SG_T, class _SG_Void = void>                             \
    struct has_##name : std::false_type {};                                   \
    template<class _SG_T>                                                     \
    struct has_##name<_SG_T, std::void_t<typename _SG_T::name>>              \
        : std::true_type {};
