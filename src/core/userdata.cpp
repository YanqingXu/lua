/**
 * @file userdata.cpp
 * @brief Lua用户数据类型实现
 */

#include "userdata.hpp"
#include "table.hpp"
#include "gc/garbage_collector.hpp"
#include <stdexcept>

// MSVC特定的对齐内存分配函数
#ifdef _MSC_VER
    #include <malloc.h>  // _aligned_malloc, _aligned_free
#endif

namespace Lua {

// =====================================================================
// 静态工厂方法
// =====================================================================

Userdata* Userdata::createFull(usize size) {
    if (size == 0) {
        throw std::invalid_argument("Userdata size cannot be zero");
    }
    
    // 检查大小溢出
    constexpr usize MAX_SIZE = static_cast<usize>(-1) - sizeof(Userdata);
    if (size > MAX_SIZE) {
        throw std::bad_alloc();
    }
    
    return new Userdata(size);
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

Userdata::Userdata(usize size)
    : GCObject(GCObjectType::Userdata)
    , size_(size)
    , data_(nullptr)
    , metatable_(nullptr)
{
    // 分配用户数据内存(8字节对齐)
    // 注意: MSVC使用_aligned_malloc而不是std::aligned_alloc
    #ifdef _MSC_VER
        data_ = _aligned_malloc(size, 8);
    #else
        data_ = std::aligned_alloc(8, size);
    #endif

    if (!data_) {
        throw std::bad_alloc();
    }

    // 零初始化用户数据
    std::memset(data_, 0, size);
}

Userdata::~Userdata() {
    // 释放用户数据内存
    if (data_) {
        #ifdef _MSC_VER
            _aligned_free(data_);
        #else
            std::free(data_);
        #endif
        data_ = nullptr;
    }
}

void Userdata::setMetatable(Table* mt) noexcept {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, mt);
    }

    metatable_ = mt;
}

// =====================================================================
// GCObject接口实现
// =====================================================================

void Userdata::mark(GarbageCollector& gc) {
    // 标记元表(如果存在)
    gc.markObject(metatable_);

    // 注意: 我们不标记用户数据内容,因为我们不知道它是否包含GC引用
    // 如果用户数据包含GC对象,用户应该通过元表的__gc方法或子类化来处理
}

usize Userdata::getSize() const {
    // 返回对象本身的大小 + 用户数据大小
    return sizeof(Userdata) + size_;
}

} // namespace Lua

