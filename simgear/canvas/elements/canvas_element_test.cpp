// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/// Unit tests for canvas::Element
#include "CanvasElement.hxx"
#include "CanvasGroup.hxx"
#include <simgear/misc/test_macros.hxx>

namespace sc = simgear::canvas;

void test_attr_data()
{
  // http://www.w3.org/TR/html5/dom.html#attr-data-*

#define SG_CHECK_ATTR2PROP(attr, prop) \
    SG_CHECK_EQUAL(sc::Element::attrToDataPropName(attr), std::string(prop))

  // If name starts with "data-", for each "-" (U+002D) character in the name
  // that is followed by a lowercase ASCII letter, remove the "-" (U+002D)
  // character and replace the character that followed it by the same character
  // converted to ASCII uppercase.

  SG_CHECK_ATTR2PROP("no-data", "");
  SG_CHECK_ATTR2PROP("data-blub", "blub");
  SG_CHECK_ATTR2PROP("data-blub-x-y", "blubXY");
  SG_CHECK_ATTR2PROP("data-blub-x-y-", "blubXY-");

#undef SG_CHECK_ATTR2PROP

#define SG_CHECK_PROP2ATTR(prop, attr) \
    SG_CHECK_EQUAL(sc::Element::dataPropToAttrName(prop), std::string(attr))

  // If name contains a "-" (U+002D) character followed by a lowercase ASCII
  // letter, throw a SyntaxError exception (empty string) and abort these steps.
  // For each uppercase ASCII letter in name, insert a "-" (U+002D) character
  // before the character and replace the character with the same character
  // converted to ASCII lowercase.
  // Insert the string "data-" at the front of name.

  SG_CHECK_PROP2ATTR("test", "data-test");
  SG_CHECK_PROP2ATTR("testIt", "data-test-it");
  SG_CHECK_PROP2ATTR("testIt-Hyphen", "data-test-it--hyphen");
  SG_CHECK_PROP2ATTR("-test", "");
  SG_CHECK_PROP2ATTR("test-", "data-test-");

#undef SG_CHECK_PROP2ATTR

  SGPropertyNode_ptr node = new SGPropertyNode;
  sc::ElementPtr el =
    sc::Element::create<sc::Group>(sc::CanvasWeakPtr(), node);

  el->setDataProp("myData", 3);
  SG_CHECK_EQUAL(el->getDataProp<int>("myData"), 3);
  SG_CHECK_EQUAL(node->getIntValue("data-my-data"), 3);

  SGPropertyNode* prop = el->getDataProp<SGPropertyNode*>("notExistingProp");
  SG_VERIFY(!prop);
  prop = el->getDataProp<SGPropertyNode*>("myData");
  SG_VERIFY(prop);
  SG_CHECK_EQUAL(prop->getParent(), node);
  SG_CHECK_EQUAL(prop->getIntValue(), 3);

  SG_VERIFY(el->hasDataProp("myData"));
  el->removeDataProp("myData");
  SG_VERIFY(!el->hasDataProp("myData"));
  SG_CHECK_EQUAL(el->getDataProp("myData", 5), 5);
}

int main()
{
    test_attr_data();
    return 0;
}
