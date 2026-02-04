// Parse CSS colors
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later


#include <simgear/misc/strutils.hxx>
#include "parse_color.hxx"

#include <map>

namespace simgear
{

  //----------------------------------------------------------------------------
  bool parseColor(std::string str, osg::Vec4& result)
  {
    str = simgear::strutils::strip(str);

    // Only needed for hexcodes, thus let's remove 1 for the hash character right away so we can make this a const
    const auto strLen = str.length() - 1;
    if (strLen < 3) {
      return false;
    }

    osg::Vec4 color(0, 0, 0, 1);

    // #rgb, #rgba, #rrggbb, #rrggbbaa
    if (str[0] == '#') {
      if (strLen > 8) {
        return false;
      }
      str.erase(0, 1);

      int offset = 2;
      float divider = 255.0;
      bool has_alpha = false;
      switch (str.length()) {
        case 3: // #rgb
          offset = 1;
          divider = 15.0;
          break;
        case 4: // #rgba
          offset = 1;
          divider = 15.0;
          has_alpha = true;
          break;
        case 6: // #rrggbb
          break;
        case 8: // #rrggbbaa
          has_alpha = true;
          break;
        default:
          return false;
      }

      const auto numComponents = (has_alpha ? 4 : 3);
      for (int comp = 0; comp < numComponents; comp++) {
        color[comp] = (float)std::stoi(str.substr(offset * comp, offset), nullptr, 16) / divider;
      }
    }
    // rgb(r,g,b)
    // rgba(r,g,b,a)
    else if (simgear::strutils::ends_with(str, ")"))
    {
      const std::string RGB = "rgb(",
                        RGBA = "rgba(";
      if (simgear::strutils::starts_with(str, RGB)) {
        str.erase(0, RGB.length());
      } else if (simgear::strutils::starts_with(str, RGBA)) {
        str.erase(0, RGBA.length());
      } else {
        return false;
      }
      str.pop_back();

      auto comps = simgear::strutils::split_on_any_of(str, "\\n\\t ,");
      const int numComponents = comps.size();
      if (numComponents < 3 or numComponents > 4) {
        return false;
      }

      for (auto i = 0; i < numComponents; i++) {
        color[i] = std::stof(comps[i])
                    // rgb = [0, 255], a = [0, 1]
                    / (i < 3 ? 255 : 1);
      }
    } else {
      // Basic color keywords
      // http://www.w3.org/TR/css3-color/#html4
      typedef std::map<std::string, osg::Vec4> ColorMap;
      static ColorMap colors;
      if (colors.empty()) {
        colors["red"    ] = osg::Vec4(1,    0,      0,      1);
        colors["black"  ] = osg::Vec4(0,    0,      0,      1);
        colors["silver" ] = osg::Vec4(0.75, 0.75,   0.75,   1);
        colors["gray"   ] = osg::Vec4(0.5,  0.5,    0.5,    1);
        colors["white"  ] = osg::Vec4(1,    1,      1,      1);
        colors["maroon" ] = osg::Vec4(0.5,  0,      0,      1);
        colors["purple" ] = osg::Vec4(0.5,  0,      0.5,    1);
        colors["fuchsia"] = osg::Vec4(1,    0,      1,      1);
        colors["green"  ] = osg::Vec4(0,    0.5,    0,      1);
        colors["lime"   ] = osg::Vec4(0,    1,      0,      1);
        colors["olive"  ] = osg::Vec4(0.5,  0.5,    0,      1);
        colors["yellow" ] = osg::Vec4(1,    1,      0,      1);
        colors["navy"   ] = osg::Vec4(0,    0,      0.5,    1);
        colors["blue"   ] = osg::Vec4(0,    0,      1,      1);
        colors["teal"   ] = osg::Vec4(0,    0.5,    0.5,    1);
        colors["aqua"   ] = osg::Vec4(0,    1,      1,      1);
      }
      ColorMap::const_iterator it = colors.find(str);
      if (it == colors.end()) {
        return false;
      }

      result = it->second;
      return true;
    }

    result = color;
    return true;
  }
} // namespace simgear
