// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Tim Moore <timoore@redhat.com>

#ifndef SIMGEAR_STRINGTABLE_HXX
#define SIMGEAR_STRINGTABLE_HXX 1

#include <mutex>
#include <string>
#include <unordered_set>

namespace simgear
{
typedef std::unordered_set<std::string> StringContainer;

class StringTable
{
    const std::string* insert(const std::string& str);
private:
    std::mutex _mutex;
    StringContainer _strings;
};
}
#endif
