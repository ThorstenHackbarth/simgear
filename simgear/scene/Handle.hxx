/*
 * SPDX-FileName: Handle.hxx
 * SPDX-FileComment: Opaque, type-safe handle to a scene resource (node, texture, material, model)
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
 */

#pragma once

#include <memory>

// Type-safe opaque handles to scene resources. Public SimGear headers that
// used to expose osg::Node*, osg::ref_ptr<osg::Texture2D>, etc. expose these
// instead, so FlightGear code does not depend on the render backend (osg,
// wicked, or none) at compile time. Backend implementations live in
// per-backend translation units (Handle_osg.cxx, Handle_wicked.cxx, ...) and
// are reached by downcasting the impl base.

namespace sg::scene {

namespace detail {

// Polymorphic base for the per-handle implementation. Backend code derives
// a concrete Impl from this class and stores its native handle (osg::ref_ptr,
// wi::ecs::Entity, ...) as a member. The virtual destructor lets
// std::shared_ptr<HandleImplBase> dispose of the derived type without the
// public header knowing its size.
struct HandleImplBase {
    virtual ~HandleImplBase() = default;
};

} // namespace detail


// Type-safe opaque handle. Tag is a phantom type that distinguishes
// e.g. NodeHandle from TextureHandle even though both store a shared_ptr to
// the same base. Copying a Handle is a refcount bump; the underlying scene
// resource lives as long as any handle references it.
template <typename Tag>
class Handle
{
public:
    using ImplBase = detail::HandleImplBase;

    Handle() = default;
    explicit Handle(std::shared_ptr<ImplBase> impl) noexcept
        : impl_(std::move(impl))
    {
    }

    bool valid() const noexcept { return static_cast<bool>(impl_); }
    explicit operator bool() const noexcept { return valid(); }
    void reset() noexcept { impl_.reset(); }

    long use_count() const noexcept { return impl_ ? impl_.use_count() : 0; }

    // For backend code that needs to recover its native handle. The caller
    // is responsible for using the correct type — `as<MyImpl>()` returns
    // nullptr if the handle is empty or the dynamic type does not match.
    template <typename T>
    T* as() const noexcept
    {
        return dynamic_cast<T*>(impl_.get());
    }

    // Untyped impl accessor; primarily for diagnostics.
    ImplBase* get() const noexcept { return impl_.get(); }

    friend bool operator==(const Handle& a, const Handle& b) noexcept
    {
        return a.impl_ == b.impl_;
    }
    friend bool operator!=(const Handle& a, const Handle& b) noexcept
    {
        return !(a == b);
    }

private:
    std::shared_ptr<ImplBase> impl_;
};


// Phantom tag types. They are intentionally incomplete — they exist only to
// give each Handle alias a distinct type identity.
struct NodeTag;
struct TextureTag;
struct MaterialTag;
struct ModelTag;
struct CompositorTag;
struct CanvasTag;

using NodeHandle       = Handle<NodeTag>;
using TextureHandle    = Handle<TextureTag>;
using MaterialHandle   = Handle<MaterialTag>;
using ModelHandle      = Handle<ModelTag>;
using CompositorHandle = Handle<CompositorTag>;
using CanvasHandle     = Handle<CanvasTag>;

} // namespace sg::scene
