// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

#include "TestContext.hxx"

#include <simgear/misc/test_macros.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalContext.hxx>

#ifdef __OpenBSD__

#warning "OpenBSD's clang cannot cope with this code."

#else

class Base1:
  public virtual SGVirtualWeakReferenced
{};

class Base2:
  public virtual SGVirtualWeakReferenced
{};

class Derived:
  public Base1,
  public Base2
{};

typedef SGSharedPtr<Base1> Base1Ptr;
typedef SGSharedPtr<Base2> Base2Ptr;
typedef SGSharedPtr<Derived> DerivedPtr;
typedef SGWeakPtr<Derived> DerivedWeakPtr;

typedef SGSharedPtr<SGReferenced> SGReferencedPtr;

// Check if shared_ptr_traits give correct types for strong and weak shared
// pointer
    #define CHECK_PTR_TRAIT_TYPE(ptr_t, type_name, type)          \
        static_assert(std::is_same_v<                             \
                      nasal::shared_ptr_traits<ptr_t>::type_name, \
                      type>);

    #define CHECK_PTR_TRAIT(ref, weak)             \
        CHECK_PTR_TRAIT_TYPE(ref, strong_ref, ref) \
        CHECK_PTR_TRAIT_TYPE(weak, weak_ref, weak)

CHECK_PTR_TRAIT(DerivedPtr, DerivedWeakPtr)
CHECK_PTR_TRAIT(std::shared_ptr<Base1>, std::weak_ptr<Base1>)

    #undef CHECK_PTR_TRAIT
    #undef CHECK_PTR_TRAIT_TYPE

static_assert(nasal::supports_weak_ref<Base1Ptr>::value);
static_assert(nasal::supports_weak_ref<Base2Ptr>::value);
static_assert(nasal::supports_weak_ref<DerivedPtr>::value);
static_assert(nasal::supports_weak_ref<DerivedWeakPtr>::value);
static_assert(!nasal::supports_weak_ref<SGReferencedPtr>::value);

static void setupGhosts()
{
  nasal::Ghost<Base1Ptr>::init("Base1");
  nasal::Ghost<Base2Ptr>::init("Base2");
  nasal::Ghost<DerivedPtr>::init("Derived")
    .bases<Base1Ptr>()
    .bases<Base2Ptr>();
}

//------------------------------------------------------------------------------
void test_ghost_weak_strong_nasal_conversion()
{
  setupGhosts();
  naContext c = naNewContext();

  DerivedPtr d = new Derived();
  DerivedWeakPtr weak(d);

  // store weak pointer and extract strong pointer
  naRef na_d = nasal::to_nasal(c, DerivedWeakPtr(d));
  SG_VERIFY(naIsGhost(na_d));
  SG_CHECK_EQUAL(nasal::from_nasal<Base1Ptr>(c, na_d), d);
  SG_CHECK_EQUAL(nasal::from_nasal<Base2Ptr>(c, na_d), d);
  SG_CHECK_EQUAL(nasal::from_nasal<DerivedPtr>(c, na_d), d);

  d.reset();
  try {
      nasal::from_nasal<DerivedPtr>(c, na_d);
      SG_TEST_FAIL("expected nasal::bad_nasal_cast not thrown");
  } catch (const nasal::bad_nasal_cast&) {
  }

  // store strong pointer and extract weak pointer
  d.reset(new Derived);
  na_d = nasal::to_nasal(c, d);
  SG_VERIFY(naIsGhost(na_d));

  weak = nasal::from_nasal<DerivedWeakPtr>(c, na_d);
  SG_CHECK_EQUAL(weak.lock(), d);

  d.reset();
  SG_VERIFY(nasal::from_nasal<DerivedPtr>(c, na_d));
  SG_VERIFY(weak.lock());

  naFreeContext(c);
  naGC();
  nasal::ghostProcessDestroyList();

  SG_VERIFY(!weak.lock());
}

//------------------------------------------------------------------------------
void test_ghost_casting_storage()
{
  setupGhosts();
  nasal::Context c;

  // Check converting to and from every class in the hierarchy for an instance
  // of the leaf class
  DerivedPtr d = new Derived();

  naRef na_d = nasal::to_nasal(c, d),
        na_b1 = nasal::to_nasal(c, Base1Ptr(d)),
        na_b2 = nasal::to_nasal(c, Base2Ptr(d));

  Derived *d0 = nasal::from_nasal<Derived*>(c, na_d),
          *d1 = nasal::from_nasal<Derived*>(c, na_b1),
          *d2 = nasal::from_nasal<Derived*>(c, na_b2);

  SG_CHECK_EQUAL(d0, d.get());
  SG_CHECK_EQUAL(d1, d.get());
  SG_CHECK_EQUAL(d2, d.get());

  Base1 *b1 = nasal::from_nasal<Base1*>(c, na_b1);
  SG_CHECK_EQUAL(b1, static_cast<Base1*>(d.get()));

  Base2 *b2 = nasal::from_nasal<Base2*>(c, na_b2);
  SG_CHECK_EQUAL(b2, static_cast<Base2*>(d.get()));

  // Check converting from base class instance to derived classes is not
  // possible
  Base1Ptr b1_ref = new Base1();
  na_b1 = nasal::to_nasal(c, b1_ref);

  SG_CHECK_EQUAL(nasal::from_nasal<Base1*>(c, na_b1), b1_ref.get());
  try {
      nasal::from_nasal<Base2*>(c, na_b1);
      SG_TEST_FAIL("expected nasal::bad_nasal_cast not thrown");
  } catch (const nasal::bad_nasal_cast&) {
  }
  try {
      nasal::from_nasal<Derived*>(c, na_b1);
      SG_TEST_FAIL("expected nasal::bad_nasal_cast not thrown");
  } catch (const nasal::bad_nasal_cast&) {
  }
}

//------------------------------------------------------------------------------
    #define CHECK_PTR_STORAGE_TRAIT_TYPE(ptr_t, storage)              \
        static_assert(std::is_same_v<                                 \
                      nasal::shared_ptr_storage<ptr_t>::storage_type, \
                      storage>);

CHECK_PTR_STORAGE_TRAIT_TYPE(DerivedPtr, Derived)
CHECK_PTR_STORAGE_TRAIT_TYPE(DerivedWeakPtr, DerivedWeakPtr)

typedef std::shared_ptr<Derived> StdDerivedPtr;
CHECK_PTR_STORAGE_TRAIT_TYPE(StdDerivedPtr, StdDerivedPtr)

typedef std::weak_ptr<Derived> StdDerivedWeakPtr;
CHECK_PTR_STORAGE_TRAIT_TYPE(StdDerivedWeakPtr, StdDerivedWeakPtr)

    #undef CHECK_PTR_STORAGE_TRAIT_TYPE

static_assert(nasal::shared_ptr_traits<Base1Ptr>::is_intrusive::value);
static_assert(nasal::shared_ptr_traits<Base2Ptr>::is_intrusive::value);
static_assert(nasal::shared_ptr_traits<DerivedPtr>::is_intrusive::value);
static_assert(nasal::shared_ptr_traits<DerivedWeakPtr>::is_intrusive::value);
static_assert(nasal::shared_ptr_traits<SGReferencedPtr>::is_intrusive::value);

static_assert(!nasal::shared_ptr_traits<std::shared_ptr<Derived>>::is_intrusive::value);
static_assert(!nasal::shared_ptr_traits<std::weak_ptr<Derived>>::is_intrusive::value);

void test_storage_traits()
{
  DerivedPtr d = new Derived();

  Derived* d_raw = nasal::shared_ptr_storage<DerivedPtr>::ref(d);
  SG_CHECK_EQUAL(d_raw, d.get());
  SG_CHECK_EQUAL(d.getNumRefs(), 2);

  DerivedWeakPtr* d_weak = nasal::shared_ptr_storage<DerivedWeakPtr>::ref(d);
  SG_CHECK_EQUAL(
    nasal::shared_ptr_storage<DerivedWeakPtr>::get<Derived*>(d_weak),
    d_raw
  );

  d.reset();
  SG_CHECK_EQUAL(Derived::count(d_raw), 1);

  nasal::shared_ptr_storage<DerivedPtr>::unref(d_raw);
  SG_VERIFY(d_weak->expired());

  nasal::shared_ptr_storage<DerivedWeakPtr>::unref(d_weak);
}

void test_bind_methods()
{
  struct TestClass
  {
    int arg1;
    std::string arg2;
    std::string arg3;
    int arg4;

    void set(int a1, const std::string& a2, const std::string& a3, int a4)
    {
      arg1 = a1;
      arg2 = a2;
      arg3 = a3;
      arg4 = a4;
    }
  };
  using TestClassPtr = std::shared_ptr<TestClass>;
  auto set_func = std::function<
    void (TestClass&, int, const std::string&, const std::string&, int)
  >(&TestClass::set);
  nasal::Ghost<TestClassPtr>::init("TestClass")
    .method("set", set_func)
    .method("setReverse", set_func, std::index_sequence<3,2,1,0>{});

  TestContext ctx;
  auto test = std::make_shared<TestClass>();

  ctx.exec("me.set(1, \"s2\", \"s3\", 4);", ctx.to_me(test));
  SG_CHECK_EQUAL(test->arg1, 1);
  SG_CHECK_EQUAL(test->arg2, "s2");
  SG_CHECK_EQUAL(test->arg3, "s3");
  SG_CHECK_EQUAL(test->arg4, 4);

  ctx.exec("me.setReverse(1, \"s2\", \"s3\", 4);", ctx.to_me(test));
  SG_CHECK_EQUAL(test->arg1, 4);
  SG_CHECK_EQUAL(test->arg2, "s3");
  SG_CHECK_EQUAL(test->arg3, "s2");
  SG_CHECK_EQUAL(test->arg4, 1);
}


void test_bind_named_args_methods()
{
    struct TestClass {
        int arg1;
        std::string arg2;
        std::string arg3;
        int arg4;

        naRef foo(const nasal::CallContext& ctx)
        {
            arg1 = ctx.requireNamedArg<int>("a");
            arg2 = ctx.getNamedArg("b", std::string("zzzz"));
            arg3 = ctx.getNamedArg("c", std::string("ssss"));
            return naNil();
        }
    };

    using TestClassPtr = std::shared_ptr<TestClass>;
    nasal::Ghost<TestClassPtr>::init("TestClass")
        .namedArgsMethod("set", &TestClass::foo);

    TestContext ctx;
    auto test = std::make_shared<TestClass>();

    ctx.exec("me.set(a: 1, b: \"s2\");", ctx.to_me(test));
    SG_CHECK_EQUAL(test->arg1, 1);
    SG_CHECK_EQUAL(test->arg2, "s2");
    SG_CHECK_EQUAL(test->arg3, "ssss");
}

#endif

int main()
{
#ifndef __OpenBSD__
  test_ghost_weak_strong_nasal_conversion();
  test_ghost_casting_storage();
  test_storage_traits();
  test_bind_methods();
  test_bind_named_args_methods();
#endif
  return 0;
}
