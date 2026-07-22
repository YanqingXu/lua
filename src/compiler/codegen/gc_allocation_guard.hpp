#pragma once

/**
 * @file gc_allocation_guard.hpp
 * @brief 代码生成期间保护已注册垃圾回收对象的资源获取即初始化守卫
 */

#include "common/types.hpp"
#include "gc/garbage_collector.hpp"

#include <utility>

namespace Lua {

/** @brief 保护尚未正式托管的垃圾回收对象。 */
template <typename T> class GCAllocationGuard {
public:
    template <typename... Args>
    explicit GCAllocationGuard(GarbageCollector& gc, Args&&... args)
        : gc_(gc), object_(gc_.create<T>(std::forward<Args>(args)...)) {}

    template <typename... Args>
    GCAllocationGuard(GarbageCollector& gc, T*& observer, Args&&... args)
        : gc_(gc), object_(gc_.create<T>(std::forward<Args>(args)...)) {
        observer_ = &observer;
        observer = object_;
    }

    ~GCAllocationGuard() {
        reset();
    }

    GCAllocationGuard(const GCAllocationGuard&) = delete;
    GCAllocationGuard& operator=(const GCAllocationGuard&) = delete;

    T* get() const noexcept {
        return object_;
    }

    T& operator*() const noexcept {
        return *object_;
    }

    T* operator->() const noexcept {
        return object_;
    }

    [[nodiscard]] T* commit() noexcept {
        observer_ = nullptr;
        T* committed = object_;
        object_ = nullptr;
        return committed;
    }

private:
    void reset() noexcept {
        if (object_ == nullptr) {
            return;
        }

        if (observer_ != nullptr && *observer_ == object_) {
            *observer_ = nullptr;
        }
        gc_.destroyManagedObject(object_);
        object_ = nullptr;
    }

    GarbageCollector& gc_;
    T* object_ = nullptr;
    T** observer_ = nullptr;
};

} // namespace Lua
