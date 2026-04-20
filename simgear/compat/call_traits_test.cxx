// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Unit tests for simgear/compat/call_traits.hxx

#ifdef SG_NO_BOOST
#  include "call_traits.hxx"
#else
#  include <boost/call_traits.hpp>
#endif

#include <simgear/misc/test_macros.hxx>
#include <type_traits>
#include <string>

// Helper: verify param_type matches expected type using static_assert
template<class T, class Expected>
constexpr void check_param_type()
{
    static_assert(std::is_same_v<typename boost::call_traits<T>::param_type, Expected>,
                  "param_type mismatch");
}

// ── Scalar types → passed by value ───────────────────────────────────────────

static void test_int_by_value()
{
    check_param_type<int, int>();
}

static void test_double_by_value()
{
    check_param_type<double, double>();
}

static void test_float_by_value()
{
    check_param_type<float, float>();
}

static void test_bool_by_value()
{
    check_param_type<bool, bool>();
}

static void test_pointer_by_value()
{
    check_param_type<int*, int*>();
    check_param_type<const char*, const char*>();
}

static void test_enum_by_value()
{
    enum class Color { Red };
    check_param_type<Color, Color>();
}

// ── Class/struct types → passed by const reference ───────────────────────────

static void test_string_by_const_ref()
{
    check_param_type<std::string, const std::string&>();
}

struct BigStruct { int data[32]; };

static void test_struct_by_const_ref()
{
    check_param_type<BigStruct, const BigStruct&>();
}

// ── Reference specialisations ─────────────────────────────────────────────────

static void test_lvalue_ref()
{
    check_param_type<int&, int&>();
    check_param_type<std::string&, std::string&>();
}

// ── All four member types present and correct ────────────────────────────────

static void test_all_members_int()
{
    using ct = boost::call_traits<int>;
    static_assert(std::is_same_v<ct::value_type,      int>);
    static_assert(std::is_same_v<ct::reference,       int&>);
    static_assert(std::is_same_v<ct::const_reference, const int&>);
    static_assert(std::is_same_v<ct::param_type,      int>);
}

static void test_all_members_string()
{
    using ct = boost::call_traits<std::string>;
    static_assert(std::is_same_v<ct::value_type,      std::string>);
    static_assert(std::is_same_v<ct::reference,       std::string&>);
    static_assert(std::is_same_v<ct::const_reference, const std::string&>);
    static_assert(std::is_same_v<ct::param_type,      const std::string&>);
}

// ── Runtime: use param_type in a generic dispatch function ───────────────────
// Mirrors the actual usage in NasalContext.hxx and to_nasal_helper.hxx

template<class T>
std::string dispatch(typename boost::call_traits<T>::param_type val)
{
    // For scalars param_type==T, for classes param_type==const T&
    return std::to_string(std::is_same_v<
        typename boost::call_traits<T>::param_type, T>);
}

static void test_dispatch_scalar()
{
    SG_CHECK_EQUAL(dispatch<int>(42),   std::string("1")); // by value → same type
}

static void test_dispatch_class()
{
    SG_CHECK_EQUAL(dispatch<std::string>("hi"), std::string("0")); // const ref → different
}

// ──────────────────────────────────────────────────────────────────────────────

int main(int, char**)
{
    test_int_by_value();
    test_double_by_value();
    test_float_by_value();
    test_bool_by_value();
    test_pointer_by_value();
    test_enum_by_value();
    test_string_by_const_ref();
    test_struct_by_const_ref();
    test_lvalue_ref();
    test_all_members_int();
    test_all_members_string();
    test_dispatch_scalar();
    test_dispatch_class();
    return 0;
}
