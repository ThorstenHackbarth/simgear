// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>

/**
 * @file
 * @brief Automated tests for simgear::hash_combine and simgear::hash_range
 *
 * The tests check properties (determinism, sensitivity to input and order)
 * rather than exact hash values, since std::hash is implementation-defined
 * and the combine formula is intentionally allowed to evolve.
 */

#include <simgear_config.h>

#include <cstdlib>              // EXIT_SUCCESS
#include <string>
#include <utility>
#include <vector>

#include <simgear/misc/test_macros.hxx>
#include "hash_utils.hxx"

using std::string;

template <class T>
static std::size_t combined(const T& v)
{
    std::size_t seed = 0;
    simgear::hash_combine(seed, v);
    return seed;
}

template <class T, class U>
static std::size_t combined(const T& a, const U& b)
{
    std::size_t seed = 0;
    simgear::hash_combine(seed, a);
    simgear::hash_combine(seed, b);
    return seed;
}

void test_deterministic()
{
    // Same input → same output, always.
    SG_CHECK_EQUAL(combined(42), combined(42));
    SG_CHECK_EQUAL(combined(string("hello")), combined(string("hello")));
    SG_CHECK_EQUAL(combined(1, 2), combined(1, 2));
}

void test_inputSensitivity()
{
    // Different inputs should produce different seeds.
    // (Theoretically a hash collision is possible, but for these tiny
    // inputs against std::hash it would be a major implementation bug.)
    SG_CHECK_NE(combined(0), combined(1));
    SG_CHECK_NE(combined(42), combined(43));
    SG_CHECK_NE(combined(string("foo")), combined(string("bar")));
    SG_CHECK_NE(combined(1, 2), combined(1, 3));
    SG_CHECK_NE(combined(1, 2), combined(2, 2));
}

void test_orderSensitivity()
{
    // Order of combine matters.
    SG_CHECK_NE(combined(1, 2), combined(2, 1));
    SG_CHECK_NE(combined(string("a"), string("b")),
                combined(string("b"), string("a")));
}

void test_seedAccumulates()
{
    // Combining with a non-zero seed differs from combining with zero.
    std::size_t s1 = 0;
    simgear::hash_combine(s1, 42);

    std::size_t s2 = 12345;
    simgear::hash_combine(s2, 42);

    SG_CHECK_NE(s1, s2);
}

void test_pairOverload()
{
    // The pair overload must be equivalent to combining the elements
    // sequentially in order.
    std::pair<int, string> p{7, "x"};

    std::size_t s_pair = 0;
    simgear::hash_combine(s_pair, p);

    std::size_t s_manual = 0;
    simgear::hash_combine(s_manual, p.first);
    simgear::hash_combine(s_manual, p.second);

    SG_CHECK_EQUAL(s_pair, s_manual);
}

void test_hashRange()
{
    std::vector<int> empty;
    std::vector<int> v123{1, 2, 3};
    std::vector<int> v124{1, 2, 4};
    std::vector<int> v123_again{1, 2, 3};

    // Determinism.
    SG_CHECK_EQUAL(simgear::hash_range(v123.begin(), v123.end()),
                   simgear::hash_range(v123_again.begin(), v123_again.end()));

    // Sensitivity to element change.
    SG_CHECK_NE(simgear::hash_range(v123.begin(), v123.end()),
                simgear::hash_range(v124.begin(), v124.end()));

    // The two-arg form is equivalent to seeding with 0 and accumulating.
    std::size_t seed = 0;
    simgear::hash_range(seed, v123.begin(), v123.end());
    SG_CHECK_EQUAL(seed, simgear::hash_range(v123.begin(), v123.end()));

    // Empty range returns the initial seed (0 for the two-arg form).
    SG_CHECK_EQUAL(simgear::hash_range(empty.begin(), empty.end()),
                   static_cast<std::size_t>(0));
}

int main(int, char**)
{
    test_deterministic();
    test_inputSensitivity();
    test_orderSensitivity();
    test_seedAccumulates();
    test_pairOverload();
    test_hashRange();

    return EXIT_SUCCESS;
}
