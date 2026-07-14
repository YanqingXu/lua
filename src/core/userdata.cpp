/**
 * @file userdata.cpp
 * @brief Lua用户数据类型实现
 */

#include "userdata.hpp"
#include "table.hpp"
#include "gc/garbage_collector.hpp"
#include "runtime/lua_allocator.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

// MSVC特定的对齐内存分配函数
#ifdef _MSC_VER
#include <malloc.h> // _aligned_malloc, _aligned_free
#endif

namespace Lua {

namespace {

constexpr usize kUserdataAlignment = 8;

usize alignedAllocationSize(usize size) noexcept {
    const usize remainder = size % kUserdataAlignment;
    return remainder == 0 ? size : size + (kUserdataAlignment - remainder);
}

usize checkedUserdataBufferSize(usize size) {
    if (size > std::numeric_limits<usize>::max() - sizeof(Userdata)) {
        throw std::bad_alloc();
    }

    const usize allocationSize = std::max(size, kUserdataAlignment);
    if (allocationSize > std::numeric_limits<usize>::max() - (kUserdataAlignment - 1)) {
        throw std::bad_alloc();
    }
    return allocationSize;
}

std::byte* allocateUserdataBuffer(LuaAllocator* allocator, usize size) {
    const usize allocationSize = checkedUserdataBufferSize(size);
    if (allocator != nullptr && allocator->isConfigured()) {
        void* data = allocator->allocate(allocationSize);
        if (data == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<std::byte*>(data);
    }

#ifdef _MSC_VER
    void* data = _aligned_malloc(allocationSize, kUserdataAlignment);
#else
    void* data = std::aligned_alloc(kUserdataAlignment, alignedAllocationSize(allocationSize));
#endif

    if (data == nullptr) {
        throw std::bad_alloc();
    }

    return static_cast<std::byte*>(data);
}

} // namespace

void UserdataBufferDeleter::operator()(std::byte* data) const noexcept {
    if (data == nullptr) {
        return;
    }

    if (allocator != nullptr && allocator->isConfigured()) {
        allocator->deallocate(data, allocationSize);
        return;
    }

#ifdef _MSC_VER
    _aligned_free(data);
#else
    std::free(data);
#endif
}

// =====================================================================
// 静态工厂方法
// =====================================================================

UPtr<Userdata> Userdata::createFullOwned(usize size) {
    (void)getGCAllocationSize(size);

    return makeUnique<Userdata>(size);
}

usize Userdata::getGCAllocationSize(usize size) {
    return sizeof(Userdata) + checkedUserdataBufferSize(size);
}

Userdata* Userdata::createFull(usize size) {
    return createFullOwned(size).release();
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

Userdata::Userdata(usize size) : Userdata(nullptr, size) {}

Userdata::Userdata(LuaAllocator* allocator, usize size)
    : GCObject(GCObjectType::Userdata), size_(size),
      data_(allocateUserdataBuffer(allocator, size), UserdataBufferDeleter{allocator, checkedUserdataBufferSize(size)}),
      metatable_(nullptr), environment_(nullptr), dataDestructor_(nullptr) {
    // 零初始化用户数据
    std::memset(data_.get(), 0, size);
}

Userdata::~Userdata() {
    if (GarbageCollector* owner = getOwnerCollector()) {
        owner->unregisterObject(this);
    }
    if (dataDestructor_ != nullptr) {
        dataDestructor_(data_.get());
    }
}

void Userdata::setMetatable(Table* mt) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, mt);
    }

    metatable_ = mt;
}

void Userdata::setEnvironment(Table* environment) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, environment);
    }

    environment_ = environment;
}

// =====================================================================
// GCObject接口实现
// =====================================================================

void Userdata::mark(GarbageCollector& gc) {
    // 标记元表(如果存在)
    gc.markObject(metatable_);
    gc.markObject(environment_);

    // 注意: 我们不标记用户数据内容,因为我们不知道它是否包含GC引用
    // 如果用户数据包含GC对象,用户应该通过元表的__gc方法或子类化来处理
}

usize Userdata::getSize() const {
    // 返回对象本身的大小 + 用户数据大小
    return getGCAllocationSize(size_);
}

} // namespace Lua
