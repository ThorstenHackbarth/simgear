/*
 * SPDX-FileName: SGSceneFwd.hxx
 * SPDX-FileComment: Forward declarations for common OSG types used in SimGear's scene subsystem
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 James Turner <james@flightgear.org>
 */

#pragma once

// osg::ref_ptr<T> template - needed to form osg_ref<T> member declarations
#include <osg/ref_ptr>

/// Forward declarations for the most commonly used OSG scene-graph types.
namespace osg {
class Geometry;
class Group;
class Node;
} // namespace osg

/// Convenience alias: osg_ref<T> is shorthand for osg::ref_ptr<T>.
/// Note: using osg_ref<T> as a data member requires the complete definition
/// of T to be visible in any translation unit that constructs or destroys
/// the containing object.
template <typename T>
using osg_ref = osg::ref_ptr<T>;


namespace simgear {
class SGReaderWriterOptions;
}

class SGMaterialLib;
