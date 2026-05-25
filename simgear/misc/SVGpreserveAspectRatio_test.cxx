// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

#include "SVGpreserveAspectRatio.hxx"
#include <simgear/misc/test_macros.hxx>

void test_parse_attribute()
{
  using simgear::SVGpreserveAspectRatio;

  SVGpreserveAspectRatio ar = SVGpreserveAspectRatio::parse("none");
  SG_VERIFY(ar.scaleToFill());
  SG_VERIFY(!ar.scaleToFit());
  SG_VERIFY(!ar.scaleToCrop());
  SG_CHECK_EQUAL_NOSTREAM(ar.alignX(), SVGpreserveAspectRatio::ALIGN_NONE);
  SG_CHECK_EQUAL_NOSTREAM(ar.alignY(), SVGpreserveAspectRatio::ALIGN_NONE);

  SVGpreserveAspectRatio ar_meet = SVGpreserveAspectRatio::parse("none meet");
  SVGpreserveAspectRatio ar_slice = SVGpreserveAspectRatio::parse("none slice");

  SG_CHECK_EQUAL_NOSTREAM(ar, ar_meet);
  SG_CHECK_EQUAL_NOSTREAM(ar, ar_slice);

  ar_meet  = SVGpreserveAspectRatio::parse("xMidYMid meet");
  SG_VERIFY(!ar_meet.scaleToFill());
  SG_VERIFY(ar_meet.scaleToFit());
  SG_VERIFY(!ar_meet.scaleToCrop());
  SG_CHECK_EQUAL_NOSTREAM(ar_meet.alignX(), SVGpreserveAspectRatio::ALIGN_MID);
  SG_CHECK_EQUAL_NOSTREAM(ar_meet.alignY(), SVGpreserveAspectRatio::ALIGN_MID);

  ar_slice = SVGpreserveAspectRatio::parse("xMidYMid slice");
  SG_VERIFY(!ar_slice.scaleToFill());
  SG_VERIFY(!ar_slice.scaleToFit());
  SG_VERIFY(ar_slice.scaleToCrop());
  SG_CHECK_EQUAL_NOSTREAM(ar_slice.alignX(), SVGpreserveAspectRatio::ALIGN_MID);
  SG_CHECK_EQUAL_NOSTREAM(ar_slice.alignY(), SVGpreserveAspectRatio::ALIGN_MID);

  SG_CHECK_NE_NOSTREAM(ar_meet, ar_slice);

  // defer is ignored, meet is default
  ar_meet  = SVGpreserveAspectRatio::parse("defer xMinYMin");
  SG_VERIFY(!ar_meet.scaleToFill());
  SG_VERIFY(ar_meet.scaleToFit());
  SG_VERIFY(!ar_meet.scaleToCrop());
  SG_CHECK_EQUAL_NOSTREAM(ar_meet.alignX(), SVGpreserveAspectRatio::ALIGN_MIN);
  SG_CHECK_EQUAL_NOSTREAM(ar_meet.alignY(), SVGpreserveAspectRatio::ALIGN_MIN);
}

int main()
{
    test_parse_attribute();
    return 0;
}
