// SPDX-FileCopyrightText: 2006-2007 Mathias Froehlich
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <osg/Vec2d>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

namespace osgGA { class GUIEventAdapter; }

// Used to implement scenery interaction.
// The interface is still under development
class SGPickCallback : public SGReferenced {
public:
  enum Priority {
    PriorityGUI = 0,
    PriorityPanel = 1,
    PriorityOther = 2,
    PriorityScenery = 3
  };

  struct Info {
    SGVec3d wgs84;
    SGVec3d local;
    SGVec2d uv;
  };

  SGPickCallback(Priority priority = PriorityOther) :
    _priority(priority)
  { }

  virtual ~SGPickCallback() {}

  // TODO maybe better provide a single callback to handle all events
  virtual bool buttonPressed( int button,
                              const osgGA::GUIEventAdapter& ea,
                              const Info& info )
  { return false; }

  virtual void update(double dt, int keyModState)
  { }

  /**
   * @param info    Can be null if no info is available (eg. mouse not over 3d
   *                object anymore)
   */
  virtual void buttonReleased( int keyModState,
                               const osgGA::GUIEventAdapter& ea,
                               const Info* info )
  { }

  /**
   * @param info    Can be null if no info is available (eg. mouse not over 3d
   *                object anymore)
   */
  virtual void mouseMoved( const osgGA::GUIEventAdapter& ea,
                           const Info* info )
  { }

  /**
   * The mouse is not hovering anymore over the element.
   */
  virtual void mouseLeave(const osg::Vec2d& windowPos)
  { }

  virtual bool hover( const osg::Vec2d& windowPos,
                      const Info& info )
  {  return false; }

  virtual Priority getPriority() const
  { return _priority; }

  /**
   * retrieve the name of the cursor to user when hovering this pickable
   * object. Mapping is undefined, since SimGear doesn't know about cursors.
   */
  virtual std::string getCursor() const
  { return std::string(); }

  /**
   * Whether the scene coordinates of the picking action should be calculated for mouseMoved().
   */
  virtual bool needsDragPosition() const
  { return false; }

  /**
   * Whether the uv coordinates of the picking action should be calculated upon
   * an intersection.
   */
  virtual bool needsUV() const
  { return false; }

private:
  Priority _priority;
};

struct SGSceneryPick {
  SGPickCallback::Info info;
  SGSharedPtr<SGPickCallback> callback;
};
