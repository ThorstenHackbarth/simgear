// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

#include "CustomEvent.hxx"
#include "MouseEvent.hxx"
#include <simgear/misc/test_macros.hxx>

namespace sc = simgear::canvas;

void test_canvas_event_types()
{
  // Register type
  SG_CHECK_EQUAL_NOSTREAM(sc::Event::strToType("test"),
                          sc::Event::UNKNOWN);
  SG_CHECK_EQUAL_NOSTREAM(sc::Event::getOrRegisterType("test"),
                          sc::Event::CUSTOM_EVENT);
  SG_CHECK_EQUAL_NOSTREAM(sc::Event::strToType("test"),
                          sc::Event::CUSTOM_EVENT);
  SG_CHECK_EQUAL(sc::Event::typeToStr(sc::Event::CUSTOM_EVENT),
                 std::string("test"));

  // Basic internal type
  SG_CHECK_EQUAL(sc::Event::typeToStr(sc::Event::MOUSE_DOWN),
                 std::string("mousedown"));
  SG_CHECK_EQUAL_NOSTREAM(sc::Event::strToType("mousedown"),
                          sc::Event::MOUSE_DOWN);

  // Unknown type
  SG_CHECK_EQUAL(sc::Event::typeToStr(123),
                 std::string("unknown"));

  // Register type through custom event instance
  sc::CustomEvent e("blub");
  SG_CHECK_EQUAL(e.getTypeString(), std::string("blub"));
  SG_CHECK_NE_NOSTREAM(e.getType(), sc::Event::UNKNOWN);
}

int main()
{
    test_canvas_event_types();
    return 0;
}
