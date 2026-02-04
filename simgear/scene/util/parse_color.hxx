// Parse CSS colors
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef PARSE_COLOR_HXX_
#define PARSE_COLOR_HXX_

#include <osg/Vec4>
#include <string>

namespace simgear
{

  /**
   * Parse a (CSS) color
   *
   * @param str     Text to parse
   * @param result  Output for parse color
   *
   * @return Whether str contained a valid color (and result has been modified)
   */
  bool parseColor(std::string str, osg::Vec4& result);

} // namespace simgear

#endif /* PARSE_COLOR_HXX_ */
