#pragma once

/**
 * @file gc_object.hpp
 * @brief 垃圾回收对象基类——垃圾回收系统的基础
 *
 * 设计说明：
 * 垃圾回收对象是所有需要垃圾回收的对象的抽象基类。
 * 使用 C++ 的虚函数机制实现多态，提供统一的垃圾回收接口。
 *
 * 核心特性：
 * - 三色标记：支持白色、灰色、黑色三种颜色标记
 * - 链表管理：通过 next 指针形成垃圾回收对象链表
 * - 类型识别：每个对象都有明确的类型标识
 * - 虚函数接口：支持子类自定义标记和大小计算
 *
 * 相关文档：lua/docs/architecture/overview.md
 */

#include "common/types.hpp"

namespace Lua {

class GarbageCollector;
class LuaAllocator;

/**
 * @brief 垃圾回收对象抽象基类——所有可回收对象的基类
 *
 * 详细说明：
 * 垃圾回收对象实现了 Lua 垃圾回收系统的核心数据结构。每个需要垃圾回收器管理的对象
 * （字符串、表、函数、用户数据、线程等）都继承自这个基类。
 *
 * 内存布局：
 * - next_: 8字节（64位指针）
 * - type_：1 字节（垃圾回收对象类型枚举）
 * - marked_：1 字节（垃圾回收标记位）
 * - 填充：6 字节（对齐到 8 字节边界）
 * 总计：16字节（基类部分）
 *
 * 三色标记算法：
 * - 白色：未访问的对象，有两种白色标记
 * - 灰色：已访问但未扫描其引用的对象
 * - 黑色：已访问且已扫描所有引用的对象
 *
 * 标记位布局（marked_字段）：
 * - 位 0：第一种白色标记
 * - 位 1：第二种白色标记
 * - 位 2：黑色标记
 * - 位 3 至 7：保留用于特殊标记（终结、弱引用等）
 */
class GCObject {
public:
    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================

    /**
     * @brief 虚析构函数 - 确保正确的多态销毁
     */
    virtual ~GCObject();

    // 禁止拷贝和移动（GC对象由GC系统管理生命周期）
    GCObject(const GCObject&) = delete;
    GCObject(GCObject&&) = delete;
    GCObject& operator=(const GCObject&) = delete;
    GCObject& operator=(GCObject&&) = delete;

    // =====================================================================
    // 类型查询
    // =====================================================================

    /**
     * @brief 获取对象类型
     * @return GCObjectType枚举值
     */
    GCObjectType getType() const noexcept {
        return type_;
    }

    // =====================================================================
    // GC颜色管理
    // =====================================================================

    /**
     * @brief 获取GC颜色
     * @return GCColor枚举值
     */
    GCColor getColor() const noexcept;

    /**
     * @brief 设置GC颜色
     * @param color 要设置的颜色
     */
    void setColor(GCColor color) noexcept;

    /**
     * @brief 获取原始标记位
     * @return 标记位的原始值
     */
    u8 getMarked() const noexcept {
        return marked_;
    }

    /**
     * @brief 设置原始标记位
     * @param mark 标记位的值
     */
    void setMarked(u8 mark) noexcept {
        marked_ = mark;
    }

    /**
     * @brief 检查对象是否为白色
     * @return true 如果对象是白色
     */
    bool isWhite() const noexcept;

    /**
     * @brief 检查对象是否为灰色
     * @return true 如果对象是灰色
     */
    bool isGray() const noexcept;

    /**
     * @brief 检查对象是否为黑色
     * @return true 如果对象是黑色
     */
    bool isBlack() const noexcept;

    /**
     * @brief 检查对象是否已标记（非白色）
     * @return true 如果对象已标记
     */
    bool isMarked() const noexcept {
        return !isWhite();
    }

    // =====================================================================
    // GC链表管理
    // =====================================================================

    /**
     * @brief 获取链表中的下一个对象
     * @return 下一个GC对象的指针，如果是最后一个则返回nullptr
     */
    GCObject* getNext() const noexcept {
        return next_;
    }

    /**
     * @brief 设置链表中的下一个对象
     * @param next 下一个对象的指针
     */
    void setNext(GCObject* next) noexcept {
        next_ = next;
    }

    /**
     * @brief 获取当前管理此对象的垃圾回收器
     */
    GarbageCollector* getOwnerCollector() const noexcept {
        return ownerCollector_;
    }

    // =====================================================================
    // 虚函数接口（子类必须实现）
    // =====================================================================

    /**
     * @brief 标记对象引用的其他对象
     *
     * 这是GC标记阶段的核心方法。每个子类需要实现这个方法，
     * 标记它所引用的所有其他GC对象。
     *
     * 例如：
     * - Table需要标记其元表、数组和哈希表中的所有值
     * - Function需要标记其上值、环境表等
     * - Thread需要标记其栈上的所有值
     *
     * @note 这个方法在GC标记阶段被调用
     */
    virtual void mark(GarbageCollector& gc) = 0;

    /**
     * @brief 获取对象占用的内存大小
     *
     * 返回对象占用的总内存大小（字节），用于GC的内存统计。
     *
     * @return 对象占用的字节数
     */
    virtual usize getSize() const = 0;

protected:
    /**
     * @brief 受保护的构造函数 - 只能由子类调用
     * @param type 对象类型
     */
    explicit GCObject(GCObjectType type) noexcept
        : next_(nullptr), ownerCollector_(nullptr), allocationAllocator_(nullptr), allocationSize_(0),
          allocationDestructor_(nullptr), accountedSize_(0), type_(type), marked_(0) {}

private:
    friend class GarbageCollector;

    void setOwnerCollector(GarbageCollector* collector) noexcept {
        ownerCollector_ = collector;
    }

    using AllocationDestructor = void (*)(GCObject*) noexcept;

    void setAllocatorAllocation(LuaAllocator* allocator, usize size, AllocationDestructor destructor) noexcept {
        allocationAllocator_ = allocator;
        allocationSize_ = size;
        allocationDestructor_ = destructor;
    }

    LuaAllocator* getAllocationAllocator() const noexcept {
        return allocationAllocator_;
    }

    usize getAllocationSize() const noexcept {
        return allocationSize_;
    }

    AllocationDestructor getAllocationDestructor() const noexcept {
        return allocationDestructor_;
    }

    void setAccountedSize(usize size) noexcept {
        accountedSize_ = size;
    }

    usize getAccountedSize() const noexcept {
        return accountedSize_;
    }

    /** @brief GC链表指针 */
    GCObject* next_;
    /** @brief 当前管理此对象的GC实例 */
    GarbageCollector* ownerCollector_;
    /** @brief 拥有当前对象内存块的分配器 */
    LuaAllocator* allocationAllocator_;
    /** @brief 传给 lua_Alloc 的精确对象内存块大小 */
    usize allocationSize_;
    /** @brief 具体的定位析构函数 */
    AllocationDestructor allocationDestructor_;
    /** @brief 最近一次计入垃圾回收器快速路径总量的大小 */
    usize accountedSize_;
    /** @brief 对象类型 */
    GCObjectType type_;
    /** @brief GC标记位 */
    u8 marked_;
};

// =====================================================================
// GC标记位操作常量
// =====================================================================

/**
 * @brief GC标记位定义
 *
 * 这些常量定义了marked_字段中各个位的含义，
 * 对应Lua 5.1.5中lgc.h的位定义。
 */
namespace GCBits {
/** @brief 白色类型0位索引 */
constexpr u8 WHITE0BIT = 0;
/** @brief 白色类型1位索引 */
constexpr u8 WHITE1BIT = 1;
/** @brief 黑色位索引 */
constexpr u8 BLACKBIT = 2;
/** @brief 终结位索引（用户数据） */
constexpr u8 FINALIZEDBIT = 3;
/** @brief 弱键表标记位索引 */
constexpr u8 WEAKKEYBIT = 4;
/** @brief 固定位索引（防止GC回收） */
constexpr u8 FIXEDBIT = 5;
/** @brief 弱值表标记位索引 */
constexpr u8 WEAKVALUEBIT = 6;

/** @brief 白色类型0掩码 */
constexpr u8 WHITE0 = (1 << WHITE0BIT);
/** @brief 白色类型1掩码 */
constexpr u8 WHITE1 = (1 << WHITE1BIT);
/** @brief 黑色掩码 */
constexpr u8 BLACK = (1 << BLACKBIT);
/** @brief 白色掩码（两种白色） */
constexpr u8 WHITEBITS = (WHITE0 | WHITE1);
/** @brief 终结掩码 */
constexpr u8 FINALIZED = (1 << FINALIZEDBIT);
/** @brief 弱键表掩码 */
constexpr u8 WEAKKEY = (1 << WEAKKEYBIT);
/** @brief 固定掩码 */
constexpr u8 FIXED = (1 << FIXEDBIT);
/** @brief 弱值表掩码 */
constexpr u8 WEAKVALUE = (1 << WEAKVALUEBIT);
/** @brief 弱表模式掩码 */
constexpr u8 WEAKBITS = (WEAKKEY | WEAKVALUE);
} // namespace GCBits

// =====================================================================
// 内联函数实现
// =====================================================================

/**
 * @brief 获取GC颜色
 */
inline GCColor GCObject::getColor() const noexcept {
    if (marked_ & GCBits::BLACK) {
        return GCColor::Black;
    } else if (marked_ & GCBits::WHITEBITS) {
        return GCColor::White;
    } else {
        return GCColor::Gray;
    }
}

/**
 * @brief 设置GC颜色
 */
inline void GCObject::setColor(GCColor color) noexcept {
    // 清除颜色位
    marked_ &= ~(GCBits::WHITEBITS | GCBits::BLACK);

    // 设置新颜色
    switch (color) {
    case GCColor::White:
        marked_ |= GCBits::WHITE0; // 默认使用WHITE0
        break;
    case GCColor::Gray:
        // 灰色不设置任何位（既不是白色也不是黑色）
        break;
    case GCColor::Black:
        marked_ |= GCBits::BLACK;
        break;
    }
}

/**
 * @brief 检查对象是否为白色
 */
inline bool GCObject::isWhite() const noexcept {
    return (marked_ & GCBits::WHITEBITS) != 0;
}

/**
 * @brief 检查对象是否为黑色
 */
inline bool GCObject::isBlack() const noexcept {
    return (marked_ & GCBits::BLACK) != 0;
}

/**
 * @brief 检查对象是否为灰色
 */
inline bool GCObject::isGray() const noexcept {
    return !isWhite() && !isBlack();
}

} // namespace Lua
