/*
 * SPDX-FileName: HandleTest.cxx
 * SPDX-FileComment: Unit tests for sg::scene::Handle (migration-labeled)
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
 */

#include <simgear/misc/test_macros.hxx>
#include <simgear/scene/Handle.hxx>

#include <memory>
#include <utility>

namespace {

struct TestImpl : sg::scene::detail::HandleImplBase {
    explicit TestImpl(int v) : value(v) { ++live_count; }
    ~TestImpl() override { --live_count; }
    int value{0};
    static inline int live_count{0};
};

// Distinct tag types so we can verify that the Handle templates produce
// distinct C++ types — a NodeHandle is *not* assignable from a TextureHandle.
using NodeH    = sg::scene::NodeHandle;
using TextureH = sg::scene::TextureHandle;

NodeH makeNode(int v)
{
    return NodeH{std::make_shared<TestImpl>(v)};
}

} // namespace


void testDefaultConstructedIsInvalid()
{
    NodeH h;
    SG_CHECK_EQUAL(h.valid(), false);
    SG_CHECK_EQUAL(static_cast<bool>(h), false);
    SG_CHECK_EQUAL(h.use_count(), 0);
    SG_CHECK_EQUAL(h.get(), nullptr);
}

void testConstructedFromImplIsValid()
{
    SG_CHECK_EQUAL(TestImpl::live_count, 0);
    {
        auto h = makeNode(42);
        SG_VERIFY(h.valid());
        SG_VERIFY(static_cast<bool>(h));
        SG_CHECK_EQUAL(h.use_count(), 1);
        SG_CHECK_EQUAL(TestImpl::live_count, 1);
        auto* impl = h.as<TestImpl>();
        SG_VERIFY(impl != nullptr);
        SG_CHECK_EQUAL(impl->value, 42);
    }
    SG_CHECK_EQUAL(TestImpl::live_count, 0);
}

void testCopyBumpsRefcount()
{
    SG_CHECK_EQUAL(TestImpl::live_count, 0);
    auto a = makeNode(7);
    SG_CHECK_EQUAL(a.use_count(), 1);
    {
        auto b = a;
        SG_CHECK_EQUAL(a.use_count(), 2);
        SG_CHECK_EQUAL(b.use_count(), 2);
        SG_VERIFY(a == b);
    }
    SG_CHECK_EQUAL(a.use_count(), 1);
    SG_CHECK_EQUAL(TestImpl::live_count, 1);
}

void testMoveDoesNotBumpRefcount()
{
    SG_CHECK_EQUAL(TestImpl::live_count, 0);
    auto a = makeNode(11);
    SG_CHECK_EQUAL(a.use_count(), 1);
    auto b = std::move(a);
    SG_CHECK_EQUAL(b.use_count(), 1);
    SG_VERIFY(!a.valid());   // NOLINT: move-from check is intentional
    SG_VERIFY(b.valid());
    SG_CHECK_EQUAL(TestImpl::live_count, 1);
}

void testResetReleasesImpl()
{
    SG_CHECK_EQUAL(TestImpl::live_count, 0);
    auto h = makeNode(3);
    SG_CHECK_EQUAL(TestImpl::live_count, 1);
    h.reset();
    SG_CHECK_EQUAL(h.valid(), false);
    SG_CHECK_EQUAL(TestImpl::live_count, 0);
}

void testWrongTypeAsReturnsNull()
{
    struct OtherImpl : sg::scene::detail::HandleImplBase {};

    auto h = makeNode(1);
    // The handle was constructed with a TestImpl. Asking for OtherImpl must
    // not succeed — dynamic_cast returns nullptr for the wrong concrete type.
    SG_CHECK_EQUAL(h.as<OtherImpl>(), nullptr);
    SG_VERIFY(h.as<TestImpl>() != nullptr);
}

void testDistinctTagTypesAreNotInterchangeable()
{
    // This is a compile-time guarantee — at runtime we just verify that the
    // two aliases instantiate to distinct types and don't share state.
    static_assert(!std::is_same_v<NodeH, TextureH>,
                  "Tagged handles must be distinct C++ types");

    NodeH n;
    TextureH t;
    SG_CHECK_EQUAL(n.valid(), false);
    SG_CHECK_EQUAL(t.valid(), false);
}


int main(int, char**)
{
    testDefaultConstructedIsInvalid();
    testConstructedFromImplIsValid();
    testCopyBumpsRefcount();
    testMoveDoesNotBumpRefcount();
    testResetReleasesImpl();
    testWrongTypeAsReturnsNull();
    testDistinctTagTypesAreNotInterchangeable();
    return 0;
}
