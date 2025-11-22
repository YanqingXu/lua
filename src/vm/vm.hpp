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
#include "core/metatable.hpp"
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

    // =====================================================================
    // 重构后的辅助函数 - 执行上下文管理
    // =====================================================================

    /**
     * @brief 验证Proto并检查调用深度
     * @param proto 函数原型
     * @param nexeccalls 嵌套调用计数
     */
    void validateAndCheckDepth(Proto* proto, i32 nexeccalls);

    /**
     * @brief 初始化执行上下文（用于函数入口和reentry）
     */
    void initializeExecutionContext();

    // =====================================================================
    // 重构后的辅助函数 - 指令执行
    // =====================================================================

    /**
     * @brief 分发并执行当前指令
     * @return true 继续执行，false 需要跳转到reentry
     */
    bool dispatchInstruction();

    // 基础操作指令
    inline void executeMove(i32 a, i32 b);
    inline void executeLoadK(i32 a, i32 bx);
    inline void executeLoadBool(i32 a, i32 b, i32 c);
    inline void executeLoadNil(i32 a, i32 b);

    // 全局变量操作
    void executeGetGlobal(i32 a, i32 bx);
    void executeSetGlobal(i32 a, i32 bx);

    // 表操作指令
    void executeGetTable(i32 a, i32 b, i32 c);
    void executeSetTable(i32 a, i32 b, i32 c);
    void executeNewTable(i32 a);
    void executeSelf(i32 a, i32 b, i32 c);
    void executeSetList(i32 a, i32 b, i32 c);

    // 一元运算指令
    void executeUnm(i32 a, i32 b);
    void executeNot(i32 a, i32 b);
    void executeLen(i32 a, i32 b);
    void executeConcat(i32 a, i32 b, i32 c);

    // 测试指令
    void executeTest(i32 a, i32 c);
    void executeTestSet(i32 a, i32 b, i32 c);

    // Upvalue操作指令
    void executeGetUpval(i32 a, i32 b);
    void executeSetUpval(i32 a, i32 b);
    void executeClose(i32 a);

    // 函数调用指令
    bool executeCall(i32 a, i32 b, i32 c, i32& nexeccalls);
    bool executeTailCall(i32 a, i32 b);
    bool executeReturn(i32 a, i32 b, i32& nexeccalls);

    // 循环指令
    void executeForLoop(i32 a, i32 sbx);
    void executeForPrep(i32 a, i32 sbx);
    void executeTForLoop(i32 a, i32 c);

    // 其他指令
    void executeClosure(i32 a, i32 bx);
    void executeVararg(i32 a, i32 b);

    // =====================================================================
    // 元方法调用辅助函数
    // =====================================================================

    /**
     * @brief 尝试调用算术运算元方法
     *
     * 当算术运算的操作数不是数字时，尝试调用相应的元方法。
     *
     * @param op 算术运算类型（TM_ADD, TM_SUB等）
     * @param left 左操作数
     * @param right 右操作数
     * @param result 存储结果的位置
     * @return true 如果成功调用元方法，false 如果没有元方法
     */
    bool tryArithMetamethod(TMS op, const Value& left, const Value& right, Value& result);

    /**
     * @brief 尝试调用__index元方法
     *
     * 当表中不存在指定键时，尝试调用__index元方法。
     * 支持元方法链（最多MAXTAGLOOP次）。
     *
     * @param table 表对象
     * @param key 索引键
     * @param result 存储结果的位置
     * @return true 如果成功获取值，false 如果失败
     */
    bool tryIndexMetamethod(const Value& table, const Value& key, Value& result);

    /**
     * @brief 尝试调用__newindex元方法
     *
     * 当给表中不存在的键赋值时，尝试调用__newindex元方法。
     * 支持元方法链（最多MAXTAGLOOP次）。
     *
     * @param table 表对象
     * @param key 索引键
     * @param value 要设置的值
     * @return true 如果成功设置值，false 如果失败
     */
    bool tryNewIndexMetamethod(const Value& table, const Value& key, const Value& value);

    /**
     * @brief 尝试调用__concat元方法
     *
     * 当字符串连接的操作数不是字符串或数字时，尝试调用__concat元方法。
     *
     * @param left 左操作数
     * @param right 右操作数
     * @param result 存储结果的位置
     * @return true 如果成功调用元方法，false 如果没有元方法
     */
    bool tryConcatMetamethod(const Value& left, const Value& right, Value& result);

    /**
     * @brief 尝试调用__len元方法
     *
     * 当对非表、非字符串对象使用#运算符时，尝试调用__len元方法。
     *
     * @param obj 对象
     * @param result 存储结果的位置
     * @return true 如果成功调用元方法，false 如果没有元方法
     */
    bool tryLenMetamethod(const Value& obj, Value& result);

    /**
     * @brief 尝试调用比较元方法
     *
     * 当比较运算的操作数类型不同或不支持直接比较时，尝试调用元方法。
     *
     * @param op 比较运算类型（TM_EQ, TM_LT, TM_LE）
     * @param left 左操作数
     * @param right 右操作数
     * @param result 存储比较结果（布尔值）
     * @return true 如果成功调用元方法，false 如果没有元方法
     */
    bool tryCompareMetamethod(TMS op, const Value& left, const Value& right, bool& result);

    /**
     * @brief 通用元方法调用接口
     *
     * 提供统一的元方法调用机制，处理栈操作和函数调用。
     *
     * @param metamethod 元方法函数
     * @param arg1 第一个参数
     * @param arg2 第二个参数
     * @param result 存储返回值的位置
     */
    void callMetamethod(const Value& metamethod, const Value& arg1,
                       const Value& arg2, Value& result);
};

} // namespace Lua

