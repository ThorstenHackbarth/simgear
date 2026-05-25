// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Rebecca Palmer

#include "strutils.hxx"
#include <simgear/misc/test_macros.hxx>
#include <string>

void test_utf8_latin1_conversion()
{
  std::string utf8_string1 = "Zweibr\u00FCcken";
  //valid UTF-8, convertible to Latin-1
  std::string latin1_string1 = "Zweibr\374cken";
  //Latin-1, not valid UTF-8
  std::string utf8_string2 = "\u600f\U00010143";
  //valid UTF-8, out of range for Latin-1

  SG_CHECK_EQUAL(simgear::strutils::utf8ToLatin1(utf8_string1), latin1_string1);
  SG_CHECK_EQUAL(simgear::strutils::utf8ToLatin1(latin1_string1), latin1_string1);
  // out-of-range for Latin-1: just verify it doesn't crash
  simgear::strutils::utf8ToLatin1(utf8_string2);
}

int main()
{
    test_utf8_latin1_conversion();
    return 0;
}
