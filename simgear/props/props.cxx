// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2000 David Megginson <david@megginson.com>

/**
 * @file
 * @brief Implementation of a property list
 *
 * See props.html for documentation [replace with URL when available].
 */

#ifdef SG_PROPS_UNTHREADSAFE

    #include "props-unsafe.cxx"

#else

#include <simgear_config.h>

#include "props.hxx"

#include <algorithm>
#include <format>
#include <limits>

#include <set>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <exception> // can't use sg_exception because of PROPS_STANDALONE
#include <mutex>
#include <thread>
#include <vector>

#include <stdio.h>
#include <string.h>

#if PROPS_STANDALONE
# include <iostream>
using std::cerr;
#else
        #include "PropertyInterpolationMgr.hxx"
        #include "vectorPropTemplates.hxx"
        #include <simgear/compiler.h>
        #include <simgear/debug/logstream.hxx>
        #include <simgear/misc/hash_utils.hxx>
        #include <simgear/sg_inlines.h>
        #include <simgear/structure/exception.hxx>
        #include <string_view>

#endif

using std::endl;
using std::find;
using std::sort;
using std::stringstream;

using namespace simgear;
