/**
 * @file call_info.hpp
 * @brief Lua调用信息：函数调用上下文管理
 * 
 * 详细说明：
 * CallInfo类存储单次函数调用的所有上下文信息，包括：
 * - 函数对象在栈中的位置
 * - 函数的栈基址（参数和局部变量的起始位置）
 * - 函数的栈顶（可用栈空间的上界）
 * - 期望的返回值数量
 * - 程序计数器（对于Lua函数）
 * - 尾调用计数
 * 
 * 核心特性：
 * - 轻量级：只存储必要的上下文信息
 * - 高效访问：所有字段都是简单类型，访问开销低
 * - 链式管理：通过数组管理调用栈，支持快速遍历
 * 
 * 栈布局示例：
 * ```
 * ┌─────────────┐ ← top (栈顶)
 * │  局部变量3  │
 * │  局部变量2  │
 * │  局部变量1  │
 * ├─────────────┤ ← base (栈基址)
 * │   参数2     │
 * │   参数1     │
 * │  函数对象   │ ← func
 * └─────────────┘
 * ```
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"

#ifdef DEBUG
#include <sstream>
#include <string>
#include <cassert>
#endif

namespace Lua {

// 前向声明
using Instruction = u32;  // 与 core/function.hpp 中的定义一致

/**
 * @brief 调用信息类
 * 
 * 存储单次函数调用的上下文信息。
 * 
 * 使用示例：
 * @code
 * CallInfo ci;
 * ci.func = 10;        // 函数对象在栈索引10
 * ci.base = 11;        // 参数从索引11开始
 * ci.top = 20;         // 栈顶在索引20
 * ci.nresults = 2;     // 期望2个返回值
 * ci.savedpc = nullptr; // C函数没有PC
 * ci.tailcalls = 0;    // 没有尾调用
 * @endcode
 */
class CallInfo {
public:
    // =====================================================================
    // 构造函数
    // =====================================================================
    
    /**
     * @brief 默认构造函数
     */
    CallInfo()
        : func(0)
        , base(0)
        , top(0)
        , savedpc(nullptr)
        , nresults(0)
        , tailcalls(0)
        , hookLine(-1)
        , hookPc(-1)
    {}
    
    // =====================================================================
    // 成员变量（公开访问，类似C结构体）
    // =====================================================================
    
    /**
     * @brief 函数对象在栈中的索引
     * 
     * 指向当前正在执行的函数对象在栈中的位置。
     * 这个位置在整个函数调用期间保持不变。
     */
    usize func;
    
    /**
     * @brief 栈基址索引
     * 
     * 指向当前函数第一个参数或局部变量的位置。
     * 所有局部变量都通过相对于base的偏移来访问。
     */
    usize base;
    
    /**
     * @brief 栈顶索引
     * 
     * 指向当前函数可用栈空间的上界。
     * 在函数执行过程中，栈指针不能超过这个位置。
     */
    usize top;
    
    /**
     * @brief 保存的程序计数器（✅ 改进：使用精确类型）
     *
     * 对于Lua函数，指向当前正在执行的字节码指令。
     * 对于C函数，这个字段为nullptr。
     */
    const Instruction* savedpc;
    
    /**
     * @brief 期望返回值数量
     * 
     * 调用者期望的返回值个数：
     * - -1 (LUA_MULTRET)：接受所有返回值
     * - 0：不需要返回值
     * - n (n>0)：需要n个返回值
     */
    i32 nresults;
    
    /**
     * @brief 尾调用计数
     * 
     * 记录了在当前调用下进行的尾调用优化次数。
     * 这个信息用于调试和错误跟踪。
     */
    i32 tailcalls;

    /**
     * @brief 最近一次向调试行钩子报告的源码行号
     *
     * -1 表示当前调用帧尚未触发行钩子。
     */
    i32 hookLine;

    /**
     * @brief 调试行钩子最近一次处理的字节码程序计数器
     *
     * 用于在向后跳转后再次触发同一行的钩子，以匹配 Lua 的循环行钩子行为。
     */
    i32 hookPc;
    
    // =====================================================================
    // 辅助方法
    // =====================================================================

    /**
     * @brief 重置调用信息
     */
    void reset() {
        func = 0;
        base = 0;
        top = 0;
        savedpc = nullptr;
        nresults = 0;
        tailcalls = 0;
        hookLine = -1;
        hookPc = -1;
    }

    // =====================================================================
    // ✅ 改进：调试支持
    // =====================================================================

    #ifdef DEBUG
    /**
     * @brief 验证CallInfo状态的有效性
     * @param stackSize 当前栈大小
     */
    void validate(usize stackSize) const {
        assert(func < stackSize && "func index out of range");
        assert(base < stackSize && "base index out of range");
        assert(top <= stackSize && "top index out of range");
        assert(base >= func && "base must be >= func");
        assert(top >= base && "top must be >= base");
        assert(nresults >= -1 && "nresults must be >= -1");
        assert(tailcalls >= 0 && "tailcalls must be >= 0");
    }

    /**
     * @brief 转换为字符串（用于调试）
     * @return CallInfo的字符串表示
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "CallInfo{func=" << func
            << ", base=" << base
            << ", top=" << top
            << ", nresults=" << nresults
            << ", tailcalls=" << tailcalls
            << ", hookLine=" << hookLine
            << ", hookPc=" << hookPc
            << ", savedpc=" << (savedpc ? "set" : "null")
            << "}";
        return oss.str();
    }
    #endif
};

} // namespace Lua

