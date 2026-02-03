// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer

// Small header computed by CMake to set up boost test.
// include AFTER #define BOOST_TEST_MODULE whatever
// but before any other boost test includes.

// Using the Boost UTF dynamic library

#ifndef BOOST_TEST_DYN_LINK
    #define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
