// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

#include "function_list.hxx"
#include <simgear/misc/test_macros.hxx>

static int func_called = 0,
           func2_called = 0;

int func(int x)
{
  ++func_called;
  return x;
}

int func2(int x)
{
  ++func2_called;
  return 2 * x;
}

int func_add(int x, int y)
{
  return func(x) + y;
}

int func2_add(int x, int y)
{
  return func2(x) + y;
}

static int func_void_called = 0;
void func_void()
{
  ++func_void_called;
}

void test_create_and_call()
{
  using namespace simgear;

  function_list<int (int)> func_list;

  SG_VERIFY(func_list.empty());

  func_list.push_back(&func);
  SG_CHECK_EQUAL(func_list(2), 2);
  SG_CHECK_EQUAL(func_called, 1);

  func_list.push_back(&func2);
  SG_CHECK_EQUAL(func_list(2), 4);
  SG_CHECK_EQUAL(func_called, 2);
  SG_CHECK_EQUAL(func2_called, 1);

  function_list<std::function<int (int)> > func_list2;
  func_list2.push_back(&func);
  func_list2.push_back(&func2);
  func_list2.push_back(&func2);

  SG_CHECK_EQUAL(func_list2(2), 4);
  SG_CHECK_EQUAL(func_called, 3);
  SG_CHECK_EQUAL(func2_called, 3);

  // two parameters
  function_list<std::function<int (int, int)> > func_list3;
  func_list3.push_back(&func_add);
  func_list3.push_back(&func2_add);

  SG_CHECK_EQUAL(func_list3(2, 3), 7);
  SG_CHECK_EQUAL(func_called, 4);
  SG_CHECK_EQUAL(func2_called, 4);

  // returning void/no params
  function_list<void()> void_func_list;
  void_func_list.push_back(&func_void);
  void_func_list();
  SG_CHECK_EQUAL(func_void_called, 1);
}

int main()
{
  test_create_and_call();
  return 0;
}
