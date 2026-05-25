// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer

#include "TestContext.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/test_macros.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalCode.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/NasalString.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/map.hxx>

#include <cstring>

enum MyEnum
{
  ENUM_FIRST,
  ENUM_ANOTHER,
  ENUM_LAST
};
struct Base
{
  naRef member(const nasal::CallContext&) { return naNil(); }
  virtual ~Base(){};

  std::string getString() const { return ""; }
  void setString(const std::string&) {}
  void constVoidFunc() const {}
  size_t test1Arg(const std::string& str) const { return str.length(); }
  bool test2Args(const std::string& s, bool c) { return c && s.empty(); }

  std::string var;
  const std::string& getVar() const { return var; }
  void setVar(const std::string v) { var = v; }

  unsigned long getThis() const { return (unsigned long)this; }
  bool genericSet(const std::string& key, const std::string& val)
  {
    return key == "test";
  }
  bool genericGet(const std::string& key, std::string& val_out) const
  {
    if( key != "get_test" )
      return false;

    val_out = "generic-get";
    return true;
  }
};

void baseVoidFunc(Base& b) {}
void baseConstVoidFunc(const Base& b) {}
size_t baseFunc2Args(Base& b, int x, const std::string& s) { return x + s.size(); }
std::string testPtr(Base& b) { return b.getString(); }
void baseFuncCallContext(const Base&, const nasal::CallContext&) {}

struct Derived:
  public Base
{
  int _x;
  int getX() const { return _x; }
  void setX(int x) { _x = x; }
};

naRef f_derivedGetRandom(const Derived&, naContext)
{
  return naNil();
}
void f_derivedSetX(Derived& d, naContext, naRef r)
{
  d._x = static_cast<int>(naNumValue(r).num);
}

struct DoubleDerived:
  public Derived
{

};

typedef std::shared_ptr<Base> BasePtr;
typedef std::vector<BasePtr> BaseVec;

struct DoubleDerived2:
  public Derived
{
  const BasePtr& getBase() const{return _base;}
  BasePtr _base;
  BaseVec doSomeBaseWork(const BaseVec& v) { return v; }
};

class SGReferenceBasedClass:
  public SGReferenced
{

};

class SGWeakReferenceBasedClass:
  public SGWeakReferenced
{

};

typedef std::shared_ptr<Derived> DerivedPtr;
typedef std::shared_ptr<DoubleDerived> DoubleDerivedPtr;
typedef std::shared_ptr<DoubleDerived2> DoubleDerived2Ptr;
typedef SGSharedPtr<SGReferenceBasedClass> SGRefBasedPtr;
typedef SGSharedPtr<SGWeakReferenceBasedClass> SGWeakRefBasedPtr;

typedef std::weak_ptr<Derived> DerivedWeakPtr;

naRef derivedFreeMember(Derived&, const nasal::CallContext&) { return naNil(); }
naRef f_derivedGetX(const Derived& d, naContext c)
{
  return nasal::to_nasal(c, d.getX());
}
naRef f_freeFunction(nasal::CallContext c) { return c.requireArg<naRef>(0); }

namespace std
{
  template<class T, std::size_t N>
  ostream& operator<<(ostream& strm, const array<T, N>& vec)
  {
    for(auto const& v: vec)
      strm << "'" << v << "',";
    return strm;
  }
}

void test_cppbind_arrays()
{
  TestContext ctx;

  naRef na_vec = ctx.to_nasal_vec(1., 2., 3.42);
  SG_VERIFY(naIsVector(na_vec));
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 0)), 1);
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 1)), 2);
  SG_CHECK_EQUAL(ctx.from_nasal<double>(naVec_get(na_vec, 2)), 3.42);

  na_vec = ctx.to_nasal(std::initializer_list<double>({1., 2., 3.42}));
  SG_VERIFY(naIsVector(na_vec));
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 0)), 1);
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 1)), 2);
  SG_CHECK_EQUAL(ctx.from_nasal<double>(naVec_get(na_vec, 2)), 3.42);

  using arr_d3_t = std::array<double, 3>;
  arr_d3_t std_arr = {1., 2., 3.42};
  na_vec = ctx.to_nasal(std_arr);
  SG_VERIFY(naIsVector(na_vec));
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 0)), 1);
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 1)), 2);
  SG_CHECK_EQUAL(ctx.from_nasal<double>(naVec_get(na_vec, 2)), 3.42);

  double d_arr[] = {1., 2., 3.42};
  na_vec = ctx.to_nasal(d_arr);
  SG_VERIFY(naIsVector(na_vec));
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 0)), 1);
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(na_vec, 1)), 2);
  SG_CHECK_EQUAL(ctx.from_nasal<double>(naVec_get(na_vec, 2)), 3.42);

  SG_CHECK_EQUAL(std_arr, ctx.from_nasal<arr_d3_t>(na_vec));
}

void test_cppbind_misc_testing()
{
  TestContext c;
  naRef r;

  using namespace nasal;

  r = c.to_nasal(ENUM_ANOTHER);
  SG_CHECK_EQUAL(c.from_nasal<int>(r), ENUM_ANOTHER);

  r = c.to_nasal("Test");
  SG_VERIFY(strncmp("Test", naStr_data(r), naStr_len(r)) == 0);
  SG_CHECK_EQUAL(c.from_nasal<std::string>(r), "Test");

  r = c.to_nasal(std::string("Test"));
  SG_VERIFY(strncmp("Test", naStr_data(r), naStr_len(r)) == 0);
  SG_CHECK_EQUAL(c.from_nasal<std::string>(r), "Test");

  r = c.to_nasal(42);
  SG_CHECK_EQUAL(naNumValue(r).num, 42);
  SG_CHECK_EQUAL(c.from_nasal<int>(r), 42);

  r = c.to_nasal(4.2f);
  SG_CHECK_EQUAL(naNumValue(r).num, 4.2f);
  SG_CHECK_EQUAL(c.from_nasal<float>(r), 4.2f);

  float test_data[3] = {0, 4, 2};
  r = c.to_nasal(test_data);

  SGVec2f vec(0,2);
  r = c.to_nasal(vec);
  SG_CHECK_EQUAL(c.from_nasal<SGVec2f>(r), vec);

  std::vector<int> std_vec;
  r = c.to_nasal(std_vec);

  r = c.to_nasal("string");
  SG_CHECK_THROW(c.from_nasal<int>(r), bad_nasal_cast);

  Hash hash(c);
  hash.set("vec", r);
  hash.set("vec2", vec);
  hash.set("name", "my-name");
  hash.set("string", std::string("blub"));
  hash.set("func", &f_freeFunction);

  SG_CHECK_EQUAL(hash.size(), 5);
  for(Hash::const_iterator it = hash.begin(); it != hash.end(); ++it)
      SG_CHECK_EQUAL(hash.get<std::string>(it->getKey()),
                     it->getValue<std::string>());

  Hash::iterator it1, it2;
  Hash::const_iterator it3 = it1, it4(it2);
  it1 = it2;
  it3 = it2;

  r = c.to_nasal(hash);
  SG_VERIFY(naIsHash(r));

  simgear::StringMap string_map = c.from_nasal<simgear::StringMap>(r);
  SG_CHECK_EQUAL(string_map.at("vec"), "string");
  SG_CHECK_EQUAL(string_map.at("name"), "my-name");
  SG_CHECK_EQUAL(string_map.at("string"), "blub");

  SG_CHECK_EQUAL(hash.get<std::string>("name"), "my-name");
  SG_VERIFY(naIsString(hash.get("name")));

  Hash mod = hash.createHash("mod");
  mod.set("parent", hash);


  // 'func' is a C++ function registered to Nasal and now converted back to C++
  std::function<int (int)> f = hash.get<int (int)>("func");
  SG_VERIFY(f);
  SG_CHECK_EQUAL(f(3), 3);

  std::function<std::string (int)> fs = hash.get<std::string (int)>("func");
  SG_VERIFY(fs);
  SG_CHECK_EQUAL(fs(14), "14");

  typedef std::function<void (int)> FuncVoidInt;
  FuncVoidInt fvi = hash.get<FuncVoidInt>("func");
  SG_VERIFY(fvi);
  fvi(123);

  typedef std::function<std::string (const std::string&, int, float)> FuncMultiArg;
  FuncMultiArg fma = hash.get<FuncMultiArg>("func");
  SG_VERIFY(fma);
  SG_CHECK_EQUAL(fma("test", 3, .5), "test");

  typedef std::function<naRef (naRef)> naRefMemFunc;
  naRefMemFunc fmem = hash.get<naRefMemFunc>("func");
  SG_VERIFY(fmem);
  naRef ret = fmem(hash.get_naRef()),
        hash_ref = hash.get_naRef();
  SG_VERIFY(naIsIdentical(ret, hash_ref));

  // Check if nasal::Me gets passed as self/me and remaining arguments are
  // passed on to function
  typedef std::function<int (Me, int)> MeIntFunc;
  MeIntFunc fmeint = hash.get<MeIntFunc>("func");
  SG_CHECK_EQUAL(fmeint(Me{}, 5), 5);

  //----------------------------------------------------------------------------
  // Test exposing classes to Nasal
  //----------------------------------------------------------------------------

  Ghost<BasePtr>::init("BasePtr")
    .method("member", &Base::member)
    .method("strlen", &Base::test1Arg)
    .member("str", &Base::getString, &Base::setString)
    .method("str_m", &Base::getString)
    .method("void", &Base::constVoidFunc)
    .member("var_r", &Base::getVar)
    .member("var_w", &Base::setVar)
    .member("var", &Base::getVar, &Base::setVar)
    .method("void", &baseVoidFunc)
    .method("void_c", &baseConstVoidFunc)
    .method("int2args", &baseFunc2Args)
    .method("bool2args", &Base::test2Args)
    .method("str_ptr", &testPtr)
    .method("this", &Base::getThis)
    ._set(&Base::genericSet)
    ._get(&Base::genericGet);
  Ghost<DerivedPtr>::init("DerivedPtr")
    .bases<BasePtr>()
    .member("x", &Derived::getX, &Derived::setX)
    .member("x_alternate", &f_derivedGetX)
    .member("x_mixed", &f_derivedGetRandom, &Derived::setX)
    .member("x_mixed2", &Derived::getX, &f_derivedSetX)
    .member("x_w", &f_derivedSetX)
    .method("free_fn", &derivedFreeMember)
    .method("free_member", &derivedFreeMember)
    .method("baseDoIt", &baseFuncCallContext);
  Ghost<DoubleDerivedPtr>::init("DoubleDerivedPtr")
    .bases<DerivedPtr>();
  Ghost<DoubleDerived2Ptr>::init("DoubleDerived2Ptr")
    .bases< Ghost<DerivedPtr> >()
    .member("base", &DoubleDerived2::getBase)
    .method("doIt", &DoubleDerived2::doSomeBaseWork);

  Ghost<SGRefBasedPtr>::init("SGRefBasedPtr");
  Ghost<SGWeakRefBasedPtr>::init("SGWeakRefBasedPtr");

  SGWeakRefBasedPtr weak_ptr(new SGWeakReferenceBasedClass());
  naRef nasal_ref = c.to_nasal(weak_ptr),
        nasal_ptr = c.to_nasal(weak_ptr.get());

  SG_VERIFY(naIsGhost(nasal_ref));
  SG_VERIFY(naIsGhost(nasal_ptr));

  SGWeakRefBasedPtr ptr1 = c.from_nasal<SGWeakRefBasedPtr>(nasal_ref),
                    ptr2 = c.from_nasal<SGWeakRefBasedPtr>(nasal_ptr);

  SG_CHECK_EQUAL_NOSTREAM(weak_ptr, ptr1);
  SG_CHECK_EQUAL_NOSTREAM(weak_ptr, ptr2);


  SG_VERIFY(Ghost<BasePtr>::isInit());
  c.to_nasal(DoubleDerived2Ptr());

  BasePtr d( new Derived );
  naRef derived = c.to_nasal(d);
  SG_VERIFY(naIsGhost(derived));
  SG_CHECK_EQUAL(std::string("DerivedPtr"), naGhost_type(derived)->name);

  // Get member function from ghost...
  naRef thisGetter = naNil();
  SG_VERIFY(naMember_get(c, derived, c.to_nasal("this"), &thisGetter));
  SG_VERIFY(naIsFunc(thisGetter));

  // ...and check if it really gets passed the correct instance
  typedef std::function<unsigned long (Me)> MemFunc;
  MemFunc fGetThis = c.from_nasal<MemFunc>(thisGetter);
  SG_VERIFY(fGetThis);
  SG_CHECK_EQUAL(fGetThis(Me{derived}), (unsigned long)d.get());

  BasePtr d2( new DoubleDerived );
  derived = c.to_nasal(d2);
  SG_VERIFY(naIsGhost(derived));
  SG_CHECK_EQUAL(std::string("DoubleDerivedPtr"),
                 naGhost_type(derived)->name);

  BasePtr d3( new DoubleDerived2 );
  derived = c.to_nasal(d3);
  SG_VERIFY(naIsGhost(derived));
  SG_CHECK_EQUAL(std::string("DoubleDerived2Ptr"),
                 naGhost_type(derived)->name);

  SGRefBasedPtr ref_based( new SGReferenceBasedClass );
  naRef na_ref_based = c.to_nasal(ref_based.get());
  SG_VERIFY(naIsGhost(na_ref_based));
  SG_CHECK_EQUAL(c.from_nasal<SGReferenceBasedClass*>(na_ref_based),
                 ref_based.get());
  SG_CHECK_EQUAL_NOSTREAM(c.from_nasal<SGRefBasedPtr>(na_ref_based), ref_based);

  SG_CHECK_EQUAL(c.from_nasal<BasePtr>(derived), d3);
  SG_CHECK_NE(c.from_nasal<BasePtr>(derived), d2);
  SG_CHECK_EQUAL(c.from_nasal<DerivedPtr>(derived),
                 std::dynamic_pointer_cast<Derived>(d3));
  SG_CHECK_EQUAL(c.from_nasal<DoubleDerived2Ptr>(derived),
                 std::dynamic_pointer_cast<DoubleDerived2>(d3));
  SG_CHECK_THROW(c.from_nasal<DoubleDerivedPtr>(derived), bad_nasal_cast);

  std::map<std::string, BasePtr> instances;
  SG_VERIFY(naIsHash(c.to_nasal(instances)));

  std::map<std::string, DerivedPtr> instances_d;
  SG_VERIFY(naIsHash(c.to_nasal(instances_d)));

  std::map<std::string, int> int_map;
  SG_VERIFY(naIsHash(c.to_nasal(int_map)));

  std::map<std::string, std::vector<int> > int_vector_map;
  SG_VERIFY(naIsHash(c.to_nasal(int_vector_map)));

  simgear::StringMap dict =
    simgear::StringMap("hello", "value")
                      ("key2", "value2");
  naRef na_dict = c.to_nasal(dict);
  SG_VERIFY(naIsHash(na_dict));
  SG_CHECK_EQUAL(Hash(na_dict, c).get<std::string>("key2"), "value2");

  // Check converting to Ghost if using Nasal hashes with actual ghost inside
  // the hashes parents vector
  std::vector<naRef> parents;
  parents.push_back(hash.get_naRef());
  parents.push_back(derived);

  Hash obj(c);
  obj.set("parents", parents);
  SG_CHECK_EQUAL(c.from_nasal<BasePtr>(obj.get_naRef()), d3);

  // Check recursive parents (aka parent-of-parent)
  std::vector<naRef> parents2;
  parents2.push_back(obj.get_naRef());
  Hash derived_obj(c);
  derived_obj.set("parents", parents2);
  SG_CHECK_EQUAL(c.from_nasal<BasePtr>(derived_obj.get_naRef()), d3);

  std::vector<naRef> nasal_objects;
  nasal_objects.push_back( Ghost<BasePtr>::makeGhost(c, d) );
  nasal_objects.push_back( Ghost<BasePtr>::makeGhost(c, d2) );
  nasal_objects.push_back( Ghost<BasePtr>::makeGhost(c, d3) );
  naRef obj_vec = c.to_nasal(nasal_objects);

  std::vector<BasePtr> objects = c.from_nasal<std::vector<BasePtr> >(obj_vec);
  SG_CHECK_EQUAL(objects[0], d);
  SG_CHECK_EQUAL(objects[1], d2);
  SG_CHECK_EQUAL(objects[2], d3);

  // Calling fallback setter for unset values
  SG_CHECK_EQUAL(c.exec<int>("me.test = 3;", Me{derived}), 3);

  // Calling generic (fallback) getter
  SG_CHECK_EQUAL(c.exec<std::string>("var a = me.get_test;", Me{derived}),
                 "generic-get");

  //----------------------------------------------------------------------------
  // Test nasal::CallContext
  //----------------------------------------------------------------------------

  int int_vec[] = {1,2,3};
  std::map<std::string, std::string> map;
  naRef args[] = {
    c.to_nasal(std::string("test-arg")),
    c.to_nasal(4),
    c.to_nasal(int_vec),
    c.to_nasal(map)
  };
  CallContext cc(c, naNil(), sizeof(args)/sizeof(args[0]), args);
  SG_CHECK_EQUAL(cc.requireArg<std::string>(0), "test-arg");
  SG_CHECK_EQUAL(cc.getArg<std::string>(0), "test-arg");
  SG_CHECK_EQUAL(cc.getArg<std::string>(10), "");
  SG_VERIFY(cc.isString(0));
  SG_VERIFY(!cc.isNumeric(0));
  SG_VERIFY(!cc.isVector(0));
  SG_VERIFY(!cc.isHash(0));
  SG_VERIFY(!cc.isGhost(0));
  SG_VERIFY(cc.isNumeric(1));
  SG_VERIFY(cc.isVector(2));
  SG_VERIFY(cc.isHash(3));

  naRef args_vec = c.to_nasal(args);
  SG_VERIFY(naIsVector(args_vec));

  //----------------------------------------------------------------------------
  // Test nasal::String
  //----------------------------------------------------------------------------

  String string( c.to_nasal("Test") );
  SG_CHECK_EQUAL(c.from_nasal<std::string>(string.get_naRef()), "Test");
  SG_CHECK_EQUAL(string.c_str(), std::string("Test"));
  SG_VERIFY(string.starts_with(string));
  SG_VERIFY(string.starts_with(String(c, "T")));
  SG_VERIFY(string.starts_with(String(c, "Te"))); // codespell:ignore te
  SG_VERIFY(string.starts_with(String(c, "Tes")));
  SG_VERIFY(string.starts_with(String(c, "Test")));
  SG_VERIFY(!string.starts_with(String(c, "Test1")));
  SG_VERIFY(!string.starts_with(String(c, "bb")));
  SG_VERIFY(!string.starts_with(String(c, "bbasdasdafasd")));
  SG_VERIFY(string.ends_with(String(c, "t")));
  SG_VERIFY(string.ends_with(String(c, "st")));
  SG_VERIFY(string.ends_with(String(c, "est")));
  SG_VERIFY(string.ends_with(String(c, "Test")));
  SG_VERIFY(!string.ends_with(String(c, "1Test")));
  SG_VERIFY(!string.ends_with(String(c, "abc")));
  SG_VERIFY(!string.ends_with(String(c, "estasdasd")));
  SG_CHECK_EQUAL(string.find('e'), 1);
  SG_CHECK_EQUAL(string.find('9'), String::npos);
  SG_CHECK_EQUAL(string.find_first_of(String(c, "st")), 2);
  SG_CHECK_EQUAL(string.find_first_of(String(c, "st"), 3), 3);
  SG_CHECK_EQUAL(string.find_first_of(String(c, "xyz")), String::npos);
  SG_CHECK_EQUAL(string.find_first_not_of(String(c, "Tst")), 1);
  SG_CHECK_EQUAL(string.find_first_not_of(String(c, "Tse"), 2), 3);
  SG_CHECK_EQUAL(string.find_first_not_of(String(c, "abc")), 0);
  SG_CHECK_EQUAL(string.find_first_not_of(String(c, "abc"), 20), String::npos);
}

void test_cppbind_context()
{
  nasal::Context ctx;
  naRef vec = ctx.to_nasal_vec(1, 2, 3.4, "test");
  SG_VERIFY(naIsVector(vec));

  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(vec, 0)), 1);
  SG_CHECK_EQUAL(ctx.from_nasal<int>(naVec_get(vec, 1)), 2);
  SG_CHECK_EQUAL(ctx.from_nasal<double>(naVec_get(vec, 2)), 3.4);
  SG_CHECK_EQUAL(ctx.from_nasal<std::string>(naVec_get(vec, 3)), "test");
}

void test_nasal_code()
{
    using namespace nasal;

    // Malformed Nasal: parse error → not valid, errors recorded
    {
        NasalCode bad(naNil(), "var x = { unclosed hash", "bad.nas");
        SG_VERIFY(!bad.isValid());
        SG_VERIFY(!bad.getErrors().empty());
        SG_VERIFY(bad.getErrors()[0].find("bad.nas") != std::string::npos);
    }

    // Default-constructed NasalCode is not valid and has no errors
    {
        NasalCode empty;
        SG_VERIFY(!empty.isValid());
        SG_VERIFY(empty.getErrors().empty());
    }

    // call() with no arguments: uses the explicit no-args overload, returns naRef
    {
        NasalCode code(naNil(), "3 + 4");
        SG_VERIFY(code.isValid());
        TestContext ctx;
        naRef result = code.call();
        SG_VERIFY(!naIsNil(result));
        SG_CHECK_EQUAL(ctx.from_nasal<double>(result), 7.0);
    }

    // call<Ret>() with no arguments: unambiguously uses the typed-return overload
    {
        NasalCode code(naNil(), "3 + 4");
        SG_VERIFY(code.isValid());
        SG_VERIFY(code.getErrors().empty());
        SG_CHECK_EQUAL(code.call<double>(), 7.0);
        SG_CHECK_EQUAL(code.call<int>(), 7);
    }

    // call(args...) returns naRef — convert with from_nasal for different types
    {
        NasalCode code(naNil(), "arg[0] + arg[1]");
        SG_VERIFY(code.isValid());
        TestContext ctx;
        SG_CHECK_EQUAL(ctx.from_nasal<double>(code.call(10.0, 5.0)), 15.0);
        SG_CHECK_EQUAL(ctx.from_nasal<int>(code.call(3, 4)), 7);
    }

    // call<Ret, Arg1, Arg2>(a, b): providing all template args avoids ambiguity
    // and exercises the typed-return overload together with arguments
    {
        NasalCode code(naNil(), "arg[0] + arg[1]");
        SG_VERIFY(code.isValid());
        SG_CHECK_EQUAL((code.call<double, double, double>(10.0, 5.0)), 15.0);
    }

    // String arguments: Nasal concatenation operator ~
    {
        NasalCode code(naNil(), "arg[0] ~ arg[1]");
        SG_VERIFY(code.isValid());
        TestContext ctx;
        SG_CHECK_EQUAL(
            ctx.from_nasal<std::string>(code.call(std::string("hello"), std::string(" world"))),
            "hello world");
    }

    // Boolean result: Nasal comparison returns 1 (true) or 0 (false)
    {
        NasalCode code(naNil(), "arg[0] > arg[1]");
        SG_VERIFY(code.isValid());
        TestContext ctx;
        SG_CHECK_EQUAL(ctx.from_nasal<bool>(code.call(5.0, 3.0)), true);
        SG_CHECK_EQUAL(ctx.from_nasal<bool>(code.call(1.0, 9.0)), false);
    }

    // callWithLocals: variables from the locals hash are visible inside the code
    {
        TestContext ctx;
        Hash locals(ctx);
        locals.set("x", 6);
        locals.set("y", 7);

        NasalCode code(naNil(), "x * y");
        SG_VERIFY(code.isValid());
        naRef result = code.callWithLocals(locals.get_naRef());
        SG_CHECK_EQUAL(ctx.from_nasal<double>(result), 42.0);
    }

    // call() with a runtime error must throw sg_exception, not crash
    {
        TestContext ctx;
        Hash globals(ctx);

        NasalCode code(globals.get_naRef(), "undefined_sym()");
        SG_VERIFY(code.isValid()); // parses fine; error is at call time
        SG_CHECK_THROW(code.call(), sg_exception);
    }

    // callWithLocals() with a runtime error must also throw sg_exception
    {
        TestContext ctx;
        Hash locals(ctx);
        Hash globals(ctx);

        NasalCode code(globals.get_naRef(), "no_such_func()");
        SG_VERIFY(code.isValid());
        SG_CHECK_THROW(code.callWithLocals(locals.get_naRef()), sg_exception);
    }
}

int main()
{
  test_cppbind_arrays();
  test_cppbind_misc_testing();
  test_cppbind_context();
  test_nasal_code();
  return 0;
}
