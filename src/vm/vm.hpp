/**
 * @file vm.hpp
 * @brief Lua虚拟机执行引擎：字节码解释器
 * 
 * 详细说明：
 * VM类实现了Lua 5.1的字节码执行引擎，负责解释执行编译后的字节码指令。
 * 这是Lua解释器的核心组件，实现了基于寄存器的虚拟机架构。
 * 
 * 核心特性：
 * - 完整的Lua 5.1指令集支持（38条指令）
 * - 基于寄存器的虚拟机架构
 * - 高效的指令分发（switch-case）
 * - 函数调用和返回机制
 * - 栈帧管理和程序计数器
 * - 错误处理和异常传播
 * 
 * 参考实现：
 * - lua_c_analysis/src/lvm.h 和 lvm.c - Lua 5.1.5虚拟机实现
 * - lua_c_analysis/src/ldo.h 和 ldo.c - 函数调用和栈管理
 * 
 * @author Lua C++ Project
 * @date 2025-11-13
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "vm/lua_state.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

/**
 * @brief 虚拟机执行引擎类
 * 
 * 实现Lua字节码的解释执行。
 * 
 * 使用示例：
 * @code
 * LuaState* L = LuaState::newState();
 * VM vm(L);
 * 
 * // 执行函数
 * Function* func = ...;  // 从CodeGenerator获取
 * vm.execute(func);
 * 
 * // 获取返回值
 * Value result = L->getStack().top();
 * @endcode
 */
class VM {
public:
    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================
    
    /**
     * @brief 构造函数
     * @param L Lua状态指针
     */
    explicit VM(LuaState* L);
    
    /**
     * @brief 析构函数
     */
    ~VM() = default;
    
    // =====================================================================
    // 执行接口
    // =====================================================================
    
    /**
     * @brief 执行Lua函数
     * @param func 要执行的函数对象
     */
    void execute(Function* func);

    /**
     * @brief 执行字节码块（Proto）
     * @param proto 函数原型
     * @param nexeccalls 嵌套调用计数（用于检测栈溢出）
     */
    void executeProto(Proto* proto, i32 nexeccalls = 1);

    /**
     * @brief 更新base_指针（栈扩展后必须调用）
     *
     * 注意：
     * - 栈扩展后std::vector可能重新分配内存
     * - base_指针会失效，必须重新获取
     * - 这是一个关键的安全机制
     */
    void updateBasePointer();

    /**
     * @brief 确保栈空间并自动更新base_指针
     * @param needed 需要的栈空间数量
     *
     * 这是推荐的方式，自动处理栈扩展和指针更新。
     */
    void ensureStackSpace(usize needed);

private:
    // =====================================================================
    // 内部状态
    // =====================================================================
    
    LuaState* L_;           ///< Lua状态指针
    Proto* currentProto_;   ///< 当前执行的函数原型
    usize pc_;              ///< 程序计数器
    Value* base_;           ///< 当前栈帧基址（性能优化：缓存指针避免每次R()调用都查找CallInfo）
    
    // =====================================================================
    // 寄存器和常量访问
    // =====================================================================
    
    /**
     * @brief 获取寄存器引用
     * @param index 寄存器索引
     * @return 寄存器值的引用
     */
    Value& R(i32 index);
    
    /**
     * @brief RK寻址（Register-Constant）
     * @param rk RK值
     * @return 寄存器或常量的值
     */
    Value RK(i32 rk);
    
    /**
     * @brief 获取常量
     * @param index 常量索引
     * @return 常量值
     */
    Value K(i32 index);
    
    // =====================================================================
    // 算术和逻辑运算
    // =====================================================================
    
    /**
     * @brief 执行算术运算
     * @param op 操作码
     * @param a 目标寄存器
     * @param b 左操作数（RK）
     * @param c 右操作数（RK）
     */
    void arith(OpCode op, i32 a, i32 b, i32 c);
    
    /**
     * @brief 执行比较运算
     * @param op 操作码
     * @param a 比较结果（0或1）
     * @param b 左操作数（RK）
     * @param c 右操作数（RK）
     */
    void compare(OpCode op, i32 a, i32 b, i32 c);
    
    // =====================================================================
    // 跳转控制
    // =====================================================================

    /**
     * @brief 执行跳转
     * @param offset 跳转偏移量
     */
    void doJump(i32 offset);

    // =====================================================================
    // 函数调用机制
    // =====================================================================

    /**
     * @brief 准备函数调用
     * @param funcIndex 函数在栈上的索引
     * @param nArgs 参数数量（0表示到栈顶）
     * @param nResults 期望的返回值数量（-1表示多返回值）
     * @return true表示Lua函数，false表示C函数
     */
    bool precall(i32 funcIndex, i32 nArgs, i32 nResults);

    /**
     * @brief 处理函数返回
     * @param firstResult 第一个返回值的索引
     * @param nResults 期望的返回值数量
     */
    void postcall(i32 firstResult, i32 nResults);

    // =====================================================================
    // 当前函数上下文
    // =====================================================================

    Function* currentFunc_;  ///< 当前执行的函数（用于访问upvalues）
};

} // namespace Lua

