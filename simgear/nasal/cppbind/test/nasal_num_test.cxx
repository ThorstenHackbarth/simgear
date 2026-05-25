// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

#include "TestContext.hxx"
#include <cmath>
#include <simgear/misc/test_macros.hxx>

static void runNumTests( double (TestContext::*test_double)(const std::string&),
                         int (TestContext::*test_int)(const std::string&) )
{
  TestContext c;
  const double eps = 1e-5;

  SG_CHECK_EQUAL_EP2((c.*test_double)("0.5"), 0.5, eps);
  SG_CHECK_EQUAL_EP2((c.*test_double)(".6"), 0.6, eps);
  SG_CHECK_EQUAL_EP2((c.*test_double)("-.7"), -0.7, eps);
  SG_CHECK_EQUAL_EP2((c.*test_double)("-0.8"), -0.8, eps);
  SG_VERIFY(std::abs((c.*test_double)("0.0")) < eps);
  SG_VERIFY(std::abs((c.*test_double)("-.0")) < eps);

  SG_CHECK_EQUAL_EP2((c.*test_double)("1.23e4"), 1.23e4, eps);
  SG_CHECK_EQUAL_EP2((c.*test_double)("1.23e-4"), 1.23e-4, 1e-9);
  SG_CHECK_EQUAL_EP2((c.*test_double)("-1.23e4"), -1.23e4, eps);
  SG_CHECK_EQUAL_EP2((c.*test_double)("-1.23e-4"), -1.23e-4, 1e-9);
  SG_CHECK_EQUAL_EP2((c.*test_double)("1e-4"), 1e-4, 1e-9);
  SG_CHECK_EQUAL_EP2((c.*test_double)("-1e-4"), -1e-4, 1e-9);

  SG_CHECK_EQUAL((c.*test_int)("123"), 123);
  SG_CHECK_EQUAL((c.*test_int)("-958"), -958);

  SG_CHECK_EQUAL_EP2((c.*test_int)("-1e7"), -1e7, eps);
  SG_CHECK_EQUAL_EP2((c.*test_int)("2E07"), 2e7, eps);

  SG_CHECK_EQUAL((c.*test_int)("0755"), 755);
  SG_CHECK_EQUAL((c.*test_int)("0055"), 55);
  SG_CHECK_EQUAL((c.*test_int)("-0155"), -155);

  SG_CHECK_EQUAL((c.*test_int)("0o755"), 0755);
  SG_CHECK_EQUAL((c.*test_int)("0o055"), 055);
  SG_CHECK_EQUAL((c.*test_int)("-0o155"), -0155);

  SG_CHECK_EQUAL((c.*test_int)("0x755"), 0x755);
  SG_CHECK_EQUAL((c.*test_int)("0x055"), 0x55);
  SG_CHECK_EQUAL((c.*test_int)("-0x155"), -0x155);

  SG_CHECK_EQUAL_EP2((c.*test_double)("2.000000953656983160"),
                     2.000000953656983160, eps);
}

void test_parse_num()
{
  runNumTests(&TestContext::convert<double>, &TestContext::convert<int>);
}

void test_lex_num()
{
  runNumTests(&TestContext::exec<double>, &TestContext::exec<int>);
}

int main()
{
  test_parse_num();
  test_lex_num();
  return 0;
}
