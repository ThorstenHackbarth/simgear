// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>

/**
 * @file
 * @brief Automated tests for simgear::param_type_t
 *
 * The selection logic is pure compile-time, so the tests are all
 * static_asserts. The main() exists only so ctest has something to run
 * (and to confirm the translation unit linked successfully).
 */

#include <simgear_config.h>

#include <cstdlib>              // EXIT_SUCCESS
#include <string>
#include <type_traits>

#include "type_utils.hxx"

namespace {

struct ClassType {
    int x;
};

enum class EnumType { A, B };

// Scalars: passed by value (T → T).
static_assert(std::is_same_v<simgear::param_type_t<int>, int>);
static_assert(std::is_same_v<simgear::param_type_t<double>, double>);
static_assert(std::is_same_v<simgear::param_type_t<char>, char>);
static_assert(std::is_same_v<simgear::param_type_t<bool>, bool>);

// Pointers and enums count as scalar → passed by value.
static_assert(std::is_same_v<simgear::param_type_t<int*>, int*>);
static_assert(std::is_same_v<simgear::param_type_t<const int*>, const int*>);
static_assert(std::is_same_v<simgear::param_type_t<EnumType>, EnumType>);

// Reference types: forwarded as-is (T → T).
static_assert(std::is_same_v<simgear::param_type_t<int&>, int&>);
static_assert(std::is_same_v<simgear::param_type_t<const int&>, const int&>);
static_assert(std::is_same_v<simgear::param_type_t<ClassType&>, ClassType&>);

// Class / struct types: passed by const-reference (T → const T&).
static_assert(std::is_same_v<simgear::param_type_t<ClassType>, const ClassType&>);
static_assert(std::is_same_v<simgear::param_type_t<std::string>, const std::string&>);

} // anonymous namespace

int main(int, char**)
{
    // All real assertions are at compile time.
    return EXIT_SUCCESS;
}
