/**
 * @file function.hpp
 * @brief Lua函数对象：函数原型和闭包实现
 * 
 * 本文件实现了Lua的函数对象系统，包括函数原型（Proto）和闭包（Closure）。
 * 
 * 核心概念：
 * - Proto：函数原型，包含字节码、常量表等编译时信息
 * - Closure：闭包对象，包装函数原型和上值（upvalues）
 * - C函数：用C++实现的函数，可以被Lua调用
 * - Lua函数：用Lua编写的函数，由虚拟机执行
 * 
 * 设计特点：
 * - 简化实现：当前版本不包含完整的字节码系统
 * - 支持C函数：可以注册C++函数供Lua调用
 * - 为后续扩展预留接口：字节码、上值等
 * - 继承GCObject：支持垃圾回收
 * 
 * 参考实现：
 * - lua_c_analysis/src/lobject.h - Proto和Closure定义
 * - lua_c_analysis/src/lfunc.h/c - 函数对象操作
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#ifndef LUA_CORE_FUNCTION_HPP
#define LUA_CORE_FUNCTION_HPP

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include "core/value.hpp"
#include <vector>

namespace Lua {

// 前向声明
class LuaState;
class GCString;

/**
 * @brief C函数类型定义
 * 
 * C函数接受LuaState指针作为参数，返回结果数量。
 * 
 * @param L Lua状态指针
 * @return 返回值数量
 */
using CFunction = i32 (*)(LuaState* L);

/**
 * @brief 函数原型类（简化版）
 * 
 * Proto包含了Lua函数编译后的信息。当前版本是简化实现，
 * 主要用于建立基础架构，后续会扩展完整的字节码支持。
 * 
 * 完整版本应包含：
 * - 字节码指令数组
 * - 常量表
 * - 调试信息（行号、局部变量名等）
 * - 子函数原型
 * - 上值信息
 * 
 * 当前简化版本包含：
 * - 基本元数据（参数数量、源文件等）
 * - 常量表（简化）
 * - GC支持
 */
class Proto : public GCObject {
public:
    /**
     * @brief 构造函数
     */
    Proto();
    
    /**
     * @brief 析构函数
     */
    ~Proto() override;
    
    // =====================================================================
    // 基本属性访问
    // =====================================================================
    
    /**
     * @brief 获取参数数量
     * @return 固定参数个数
     */
    u8 getNumParams() const noexcept { return numParams_; }
    
    /**
     * @brief 设置参数数量
     * @param n 参数个数
     */
    void setNumParams(u8 n) noexcept { numParams_ = n; }
    
    /**
     * @brief 是否为可变参数函数
     * @return 如果接受可变参数返回true
     */
    bool isVararg() const noexcept { return isVararg_; }
    
    /**
     * @brief 设置可变参数标志
     * @param vararg 是否为可变参数
     */
    void setVararg(bool vararg) noexcept { isVararg_ = vararg; }
    
    /**
     * @brief 获取最大栈大小
     * @return 函数执行需要的最大栈空间
     */
    u8 getMaxStackSize() const noexcept { return maxStackSize_; }
    
    /**
     * @brief 设置最大栈大小
     * @param size 栈大小
     */
    void setMaxStackSize(u8 size) noexcept { maxStackSize_ = size; }
    
    /**
     * @brief 获取源文件名
     * @return 源文件名字符串
     */
    GCString* getSource() const noexcept { return source_; }
    
    /**
     * @brief 设置源文件名
     * @param src 源文件名
     */
    void setSource(GCString* src) noexcept { source_ = src; }
    
    // =====================================================================
    // 常量表操作（简化版）
    // =====================================================================
    
    /**
     * @brief 添加常量
     * @param value 常量值
     * @return 常量在常量表中的索引
     */
    usize addConstant(const Value& value);
    
    /**
     * @brief 获取常量
     * @param index 常量索引
     * @return 常量值
     */
    Value getConstant(usize index) const;
    
    /**
     * @brief 获取常量数量
     * @return 常量表大小
     */
    usize getConstantCount() const noexcept { return constants_.size(); }
    
    // =====================================================================
    // GCObject接口实现
    // =====================================================================
    
    void mark() override;
    usize getSize() const override;

private:
    /// 参数数量
    u8 numParams_;
    
    /// 可变参数标志
    bool isVararg_;
    
    /// 最大栈大小
    u8 maxStackSize_;
    
    /// 源文件名
    GCString* source_;
    
    /// 常量表（简化版）
    Vec<Value> constants_;
};

/**
 * @brief 函数类（闭包）
 *
 * Function是Lua中的函数对象，可以是C函数或Lua函数。
 * 在Lua中也称为Closure（闭包）。
 *
 * C函数闭包：
 * - 包装C++函数指针
 * - 可以有上值（upvalues）
 * - 直接由C++代码执行
 *
 * Lua函数闭包：
 * - 包含函数原型（Proto）
 * - 可以有上值（upvalues）
 * - 由虚拟机解释执行
 *
 * 当前简化版本：
 * - 支持C函数
 * - 支持Lua函数（但暂无字节码执行）
 * - 暂不支持上值（后续添加）
 */
class Function : public GCObject {
public:
    /**
     * @brief 创建C函数闭包
     * @param func C函数指针
     */
    explicit Function(CFunction func);

    /**
     * @brief 创建Lua函数闭包
     * @param proto 函数原型
     */
    explicit Function(Proto* proto);

    /**
     * @brief 析构函数
     */
    ~Function() override;
    
    // =====================================================================
    // 类型检查
    // =====================================================================
    
    /**
     * @brief 是否为C函数
     * @return 如果是C函数返回true
     */
    bool isCFunction() const noexcept { return isC_; }
    
    /**
     * @brief 是否为Lua函数
     * @return 如果是Lua函数返回true
     */
    bool isLuaFunction() const noexcept { return !isC_; }
    
    // =====================================================================
    // C函数访问
    // =====================================================================
    
    /**
     * @brief 获取C函数指针
     * @return C函数指针（如果不是C函数返回nullptr）
     */
    CFunction getCFunction() const noexcept {
        return isC_ ? cFunction_ : nullptr;
    }
    
    // =====================================================================
    // Lua函数访问
    // =====================================================================
    
    /**
     * @brief 获取函数原型
     * @return 函数原型指针（如果不是Lua函数返回nullptr）
     */
    Proto* getProto() const noexcept {
        return isC_ ? nullptr : proto_;
    }
    
    // =====================================================================
    // GCObject接口实现
    // =====================================================================
    
    void mark() override;
    usize getSize() const override;

private:
    /// 是否为C函数
    bool isC_;
    
    /// C函数指针（仅当isC_为true时有效）
    CFunction cFunction_;
    
    /// 函数原型（仅当isC_为false时有效）
    Proto* proto_;
    
    // TODO: 添加上值支持
    // Vec<UpVal*> upvalues_;
};

} // namespace Lua

#endif // LUA_CORE_FUNCTION_HPP

