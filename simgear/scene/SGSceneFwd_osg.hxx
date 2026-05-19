/*
 * SPDX-FileName: SGSceneFwd_osg.hxx
 * SPDX-FileComment: OSG-backend forward declarations for SimGear's scene subsystem
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
 */

#pragma once

// OSG-flavored convenience forward declarations. Used by scene-subsystem
// internals (simgear/scene/sky/, simgear/scene/tgdb/, ...) that legitimately
// hold OSG types in their signatures. Public, backend-neutral declarations
// live in SGSceneFwd.hxx; do not include this header from a public API
// surface.

#include <simgear/scene/SGSceneFwd.hxx>

#include <osg/ref_ptr>

namespace osg {
class Geometry;
class Group;
class Node;
} // namespace osg

// Convenience alias: osg_ref<T> is shorthand for osg::ref_ptr<T>.
// Note: using osg_ref<T> as a data member requires the complete definition
// of T to be visible in any translation unit that constructs or destroys
// the containing object.
template <typename T>
using osg_ref = osg::ref_ptr<T>;
