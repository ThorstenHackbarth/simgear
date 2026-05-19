/*
 * SPDX-FileName: SGSceneFwd.hxx
 * SPDX-FileComment: Backend-neutral forward declarations for SimGear's scene subsystem
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 James Turner <james@flightgear.org>
 *                         2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
 */

#pragma once

// Public, backend-neutral forward declarations for the SimGear scene
// subsystem. This header MUST NOT include or reference OSG types; the legacy
// OSG-flavored forwards live in the sibling SGSceneFwd_osg.hxx, which is
// internal to the osg-backend build.

#include <simgear/scene/Handle.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGMaterialLib;
