// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Mathias Froehlich <Mathias.Froehlich@web.de>

#ifndef SIMGEAR_OSGSINGLETON_HXX
#define SIMGEAR_OSGSINGLETON_HXX 1

#include <osg/Referenced>
#include <osg/ref_ptr>

namespace simgear {

template <typename RefClass>
class SingletonRefPtr
{
public:
    SingletonRefPtr()
    {
        ptr = new RefClass;
    }
    static RefClass* instance()
    {
        static SingletonRefPtr singleton;
        return singleton.ptr.get();
    }
private:
    osg::ref_ptr<RefClass> ptr;
};

template <typename RefClass>
class ReferencedSingleton : public virtual osg::Referenced
{
public:
    static RefClass* instance()
    {
        return SingletonRefPtr<RefClass>::instance();
    }
};

}

#endif
