#pragma once

/**
 * @file gc_allocation_guard.hpp
 * @brief RAII guard for GC-registered objects during code generation.
 */

#include "common/types.hpp"
#include "gc/garbage_collector.hpp"

#include <memory>
#include <utility>

namespace Lua {

template<typename T>
class GCAllocationGuard {
public:
    template<typename... Args>
    explicit GCAllocationGuard(GarbageCollector& gc, Args&&... args)
        : gc_(gc)
        , object_(std::make_unique<T>(std::forward<Args>(args)...)) {
        gc_.registerObject(object_.get());
    }

    template<typename... Args>
    GCAllocationGuard(GarbageCollector& gc, T*& observer, Args&&... args)
        : gc_(gc)
        , object_(std::make_unique<T>(std::forward<Args>(args)...)) {
        gc_.registerObject(object_.get());
        observer_ = &observer;
        observer = object_.get();
    }

    ~GCAllocationGuard() {
        reset();
    }

    GCAllocationGuard(const GCAllocationGuard&) = delete;
    GCAllocationGuard& operator=(const GCAllocationGuard&) = delete;

    T* get() const noexcept {
        return object_.get();
    }

    T& operator*() const noexcept {
        return *object_;
    }

    T* operator->() const noexcept {
        return object_.get();
    }

    [[nodiscard]] T* commit() noexcept {
        observer_ = nullptr;
        return object_.release();
    }

private:
    void reset() noexcept {
        if (object_ == nullptr) {
            return;
        }

        if (observer_ != nullptr && *observer_ == object_.get()) {
            *observer_ = nullptr;
        }
        gc_.unregisterObject(object_.get());
        object_.reset();
    }

    GarbageCollector& gc_;
    UPtr<T> object_;
    T** observer_ = nullptr;
};

} // namespace Lua
