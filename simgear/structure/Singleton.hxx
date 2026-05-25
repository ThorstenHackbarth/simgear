// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Tim Moore <timoore@redhat.com>

#ifndef SIMGEAR_SINGLETON_HXX
#define SIMGEAR_SINGLETON_HXX 1

namespace simgear
{
/**
 * Class that supplies the address of a singleton instance. This class
 * can be inherited by its Class argument in order to support the
 * instance() method in that class.
 */
template <typename Class>
class Singleton
{
protected:
    Singleton() {}
public:
    static Class* instance()
    {
        static Class singleton;
        return &singleton;
    }
};

}
#endif
