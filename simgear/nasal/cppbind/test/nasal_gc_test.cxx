// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

#include "TestContext.hxx"
#include <simgear/misc/test_macros.hxx>
#include <simgear/nasal/cppbind/NasalObjectHolder.hxx>

#include <set>

static std::set<intptr_t> active_instances;

static void ghost_destroy(void* p)
{
  active_instances.erase((intptr_t)p);
}

static naGhostType ghost_type = {
  &ghost_destroy,
  "TestGhost",
  0, // get_member
  0  // set_member
};

static naRef createTestGhost(TestContext& c, intptr_t p)
{
  active_instances.insert(p);
  return naNewGhost(c, &ghost_type, (void*)p);
}

void test_ghost_gc()
{
  TestContext c;
  SG_VERIFY(active_instances.empty());

  naRef g1 = createTestGhost(c, 1),
        g2 = createTestGhost(c, 2);

  SG_CHECK_EQUAL(active_instances.count(1), 1u);
  SG_CHECK_EQUAL(active_instances.count(2), 1u);
  SG_CHECK_EQUAL(active_instances.size(), 2u);

  c.runGC();
  SG_VERIFY(active_instances.empty());

  g1 = createTestGhost(c, 1);
  g2 = createTestGhost(c, 2);

  int gc1 = naGCSave(g1);
  c.runGC();

  SG_CHECK_EQUAL(active_instances.count(1), 1u);
  SG_CHECK_EQUAL(active_instances.size(), 1u);

  naGCRelease(gc1);
  c.runGC();
  SG_VERIFY(active_instances.empty());

  g1 = createTestGhost(c, 1);
  g2 = createTestGhost(c, 2);

  gc1 = naGCSave(g1);
  naGhost_setData(g1, g2);
  c.runGC();

  SG_CHECK_EQUAL(active_instances.count(1), 1u);
  SG_CHECK_EQUAL(active_instances.count(2), 1u);
  SG_CHECK_EQUAL(active_instances.size(), 2u);

  naGhost_setData(g1, naNil());
  c.runGC();

  SG_CHECK_EQUAL(active_instances.count(1), 1u);
  SG_CHECK_EQUAL(active_instances.size(), 1u);

  naGCRelease(gc1);
  c.runGC();
  SG_VERIFY(active_instances.empty());
}

void test_object_holder_gc()
{
  TestContext c;
  SG_CHECK_EQUAL(naNumSaved(), 0);
  SG_VERIFY(active_instances.empty());

  naRef g1 = createTestGhost(c, 1),
        g2 = createTestGhost(c, 2);

  nasal::ObjectHolder<> h1(g1);
  SG_CHECK_EQUAL(naNumSaved(), 1);
  SG_VERIFY(naIsGhost(h1.get_naRef()));

  nasal::ObjectHolder<> h2(g2);
  SG_CHECK_EQUAL(naNumSaved(), 2);
  SG_VERIFY(naIsGhost(h2.get_naRef()));

  c.runGC();
  SG_CHECK_EQUAL(active_instances.size(), 2u);
  SG_CHECK_EQUAL(naNumSaved(), 2);

  h1.reset(naNum(1));
  h2.reset(naNum(2));
  SG_CHECK_EQUAL(naNumSaved(), 2);

  h1.reset();
  SG_CHECK_EQUAL(naNumSaved(), 1);

  h2.reset();
  SG_CHECK_EQUAL(naNumSaved(), 0);

  c.runGC();
  SG_CHECK_EQUAL(active_instances.size(), 0u);
}

int main()
{
    test_ghost_gc();
    test_object_holder_gc();
    return 0;
}
