// userdata.hxx -- two classes for populating ssg user data slots in association
//                 with our implimenation of random surface objects.
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2001 David Megginson

#pragma once

#include <simgear/compiler.h>

class SGPropertyNode;

/**
 * the application must call sgUserDataInit() and specify the
 * following values (needed by the model loader callback at draw time)
 * before drawing any scenery.
 */
void sgUserDataInit(SGPropertyNode *p);

namespace simgear
{
/**
 * Get the property root for the simulation
 */
SGPropertyNode* getPropertyRoot();
} // namespace simgear
