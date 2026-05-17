/**
 * @file metatable.hpp
 * @brief Lua元方法系统：元表和元方法管理接口
 * 
 * 详细说明：
 * 这个文件实现了Lua的元方法（metamethods）系统，也称为标签方法（tag methods）。
 * 元方法是Lua面向对象编程和操作符重载的核心机制，允许用户自定义表、用户数据
 * 和其他类型的行为。
 * 
 * 系统架构：
 * - 元方法类型定义：17种标准元方法（TMS枚举）
 * - 元方法查找机制：从元表中查找指定的元方法
 * - 元方法调用机制：提供统一的元方法调用接口
 * - 缓存优化：通过标志位避免重复查找
 * 
 * 支持的元方法：
 * - 索引操作：__index, __newindex
 * - 算术运算：__add, __sub, __mul, __div, __mod, __pow, __unm
 * - 比较操作：__eq, __lt, __le
 * - 其他操作：__concat, __len, __call
 * - 特殊方法：__gc, __mode
 * 
 * 参考实现：
 * - lua_c_analysis/src/ltm.h - Lua 5.1.5元方法系统接口
 * - lua_c_analysis/src/ltm.c - Lua 5.1.5元方法系统实现
 * - lua_c_analysis/src/lvm.c - 元方法调用机制
 * 
 * @author Lua C++ Project
 * @date 2025-11-22
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include "vm/vm_constants.hpp"

namespace Lua {

// 前向声明
class Table;
class LuaState;

/**
 * @brief 元方法类型枚举（Tag Method System）
 * 
 * 定义所有支持的元方法类型。枚举顺序与Lua 5.1.5保持一致，
 * 前5个（TM_INDEX到TM_EQ）是"快速"元方法，有特殊优化。
 * 
 * 注意：修改此枚举的顺序需要同步更新 kMetamethodNames 数组
 * 
 * @see lua_c_analysis/src/ltm.h 第146-364行
 */
enum class TMS : u8 {
    // ===== 快速访问元方法（有缓存优化） =====
    
    /// __index: 控制表的索引访问行为（table[key]）
    TM_INDEX = 0,
    
    /// __newindex: 控制表的索引赋值行为（table[key] = value）
    TM_NEWINDEX,
    
    /// __gc: 垃圾回收终结器
    TM_GC,
    
    /// __mode: 弱引用模式（"k", "v", "kv"）
    TM_MODE,
    
    /// __eq: 相等比较运算符（==, ~=）
    TM_EQ,  // 最后一个快速访问元方法
    
    // ===== 算术运算元方法 =====
    
    /// __add: 加法运算符（+）
    TM_ADD,
    
    /// __sub: 减法运算符（-）
    TM_SUB,
    
    /// __mul: 乘法运算符（*）
    TM_MUL,
    
    /// __div: 除法运算符（/）
    TM_DIV,
    
    /// __mod: 取模运算符（%）
    TM_MOD,
    
    /// __pow: 幂运算符（^）
    TM_POW,
    
    /// __unm: 一元负号运算符（-x）
    TM_UNM,
    
    // ===== 其他操作元方法 =====
    
    /// __len: 长度运算符（#）
    TM_LEN,
    
    /// __lt: 小于比较运算符（<）
    TM_LT,
    
    /// __le: 小于等于比较运算符（<=）
    TM_LE,
    
    /// __concat: 字符串连接运算符（..）
    TM_CONCAT,
    
    /// __call: 函数调用运算符（obj(...)）
    TM_CALL,
    
    /// 元方法总数
    TM_N
};

/**
 * @brief 元方法名称字符串数组
 * 
 * 按照TMS枚举顺序定义的元方法名称，用于元表查找。
 * 
 * @see lua_c_analysis/src/ltm.c 第203-208行
 */
extern const char* const kMetamethodNames[static_cast<usize>(TMS::TM_N)];

// =====================================================================
// 元方法查找函数
// =====================================================================

/**
 * @brief 从元表中查找指定的元方法
 * 
 * 这是元方法系统的核心查找函数，从给定的元表中查找指定类型的元方法。
 * 实现了标志位缓存机制，避免重复查找不存在的元方法。
 * 
 * 查找过程：
 * 1. 检查元表是否为nullptr
 * 2. 检查元表的flags标志位（对于快速元方法）
 * 3. 在元表中查找元方法名称对应的值
 * 4. 如果未找到，更新flags标志位
 * 
 * @param metatable 元表指针，可以为nullptr
 * @param event 元方法类型
 * @return 找到的元方法值，如果不存在返回nil
 * 
 * @note 对应C实现的 luaT_gettm()
 * @see lua_c_analysis/src/ltm.c 第282-290行
 */
Value getMetamethod(Table* metatable, TMS event);

/**
 * @brief 根据对象类型查找元方法
 * 
 * 根据给定对象的类型，在相应的元表中查找指定的元方法。
 * 自动处理不同对象类型的元表获取逻辑。
 * 
 * 支持的对象类型：
 * - 表：使用对象自身的元表
 * - 用户数据：使用对象自身的元表
 * - 其他类型：使用 GlobalState 中的全局基础类型元表
 * 
 * @param L Lua状态指针
 * @param obj 要查找元方法的对象
 * @param event 元方法类型
 * @return 找到的元方法值，如果不存在返回nil
 * 
 * @note 对应C实现的 luaT_gettmbyobj()
 * @see lua_c_analysis/src/ltm.c 第368-381行
 */
Value getMetamethodByObject(LuaState* L, const Value& obj, TMS event);

// =====================================================================
// 元方法调用函数
// =====================================================================

/**
 * @brief 调用元方法并获取返回值
 *
 * 调用指定的元方法函数，传递两个参数，并将结果存储到指定位置。
 * 用于需要返回值的元方法调用，如算术运算、索引访问等。
 *
 * 调用过程：
 * 1. 将元方法函数推入栈
 * 2. 将两个参数推入栈
 * 3. 调用函数（1个返回值）
 * 4. 将返回值存储到result
 *
 * @param L Lua状态指针
 * @param result 存储返回值的位置
 * @param metamethod 元方法函数
 * @param arg1 第一个参数
 * @param arg2 第二个参数
 *
 * @note 对应C实现的 callTMres()
 * @see lua_c_analysis/src/lvm.c 第396-409行
 */
void callTMWithResult(LuaState* L, Value& result, const Value& metamethod,
                      const Value& arg1, const Value& arg2);

/**
 * @brief 调用元方法（无返回值）
 *
 * 调用指定的元方法函数，传递三个参数，不处理返回值。
 * 用于副作用操作，如__newindex元方法。
 *
 * 调用过程：
 * 1. 将元方法函数推入栈
 * 2. 将三个参数推入栈
 * 3. 调用函数（0个返回值）
 *
 * @param L Lua状态指针
 * @param metamethod 元方法函数
 * @param arg1 第一个参数
 * @param arg2 第二个参数
 * @param arg3 第三个参数
 *
 * @note 对应C实现的 callTM()
 * @see lua_c_analysis/src/lvm.c 第453-463行
 */
void callTM(LuaState* L, const Value& metamethod, const Value& arg1,
            const Value& arg2, const Value& arg3);

/**
 * @brief 调用二元运算元方法
 *
 * 尝试调用二元运算的元方法（如__add, __sub等）。
 * 先尝试左操作数的元方法，如果不存在则尝试右操作数的元方法。
 *
 * 查找顺序：
 * 1. 查找p1的元方法
 * 2. 如果不存在，查找p2的元方法
 * 3. 如果都不存在，返回false
 * 4. 调用找到的元方法，将结果存储到result
 *
 * @param L Lua状态指针
 * @param p1 左操作数
 * @param p2 右操作数
 * @param result 存储结果的位置
 * @param event 元方法类型（TM_ADD, TM_SUB等）
 * @return true 如果成功调用元方法，false 如果没有找到元方法
 *
 * @note 对应C实现的 call_binTM()
 * @see lua_c_analysis/src/lvm.c 第689-698行
 */
bool callBinaryTM(LuaState* L, const Value& p1, const Value& p2,
                  Value& result, TMS event);

/**
 * @brief 调用比较运算元方法
 *
 * 尝试调用比较运算的元方法（__lt, __le）。
 * 要求两个操作数必须有相同的元方法（对称性检查）。
 *
 * 对称性要求：
 * - 两个操作数必须有相同的元方法
 * - 防止不对称的比较行为
 * - 确保比较结果的一致性
 *
 * @param L Lua状态指针
 * @param p1 左操作数
 * @param p2 右操作数
 * @param event 元方法类型（TM_LT或TM_LE）
 * @return 比较结果：-1表示没有元方法，0表示false，1表示true
 *
 * @note 对应C实现的 call_orderTM()
 * @see lua_c_analysis/src/lvm.c 第789-800行
 */
i32 callOrderTM(LuaState* L, const Value& p1, const Value& p2, TMS event);

/**
 * @brief 获取比较元方法并检查对称性
 *
 * 获取两个元表的比较元方法，并检查它们是否相同。
 * 只有当两个元表有相同的元方法时才返回该元方法。
 *
 * 检查规则：
 * 1. 获取第一个元表的元方法
 * 2. 如果没有元方法，返回nil
 * 3. 如果两个元表相同，直接返回元方法
 * 4. 获取第二个元表的元方法
 * 5. 比较两个元方法是否相同
 *
 * @param L Lua状态指针
 * @param mt1 第一个元表
 * @param mt2 第二个元表
 * @param event 元方法类型
 * @return 找到的元方法（如果对称），否则返回nil
 *
 * @note 对应C实现的 get_compTM()
 * @see lua_c_analysis/src/lvm.c 第738-750行
 */
Value getComparisonTM(LuaState* L, Table* mt1, Table* mt2, TMS event);

// =====================================================================
// 辅助函数
// =====================================================================

/**
 * @brief 快速元方法访问（带缓存优化）
 *
 * 通过标志位快速判断元方法是否存在，避免不必要的表查找。
 * 只对前5个"快速"元方法（TM_INDEX到TM_EQ）有效。
 *
 * 优化机制：
 * - 检查元表的flags字段
 * - 如果标志位表示元方法不存在，直接返回nil
 * - 否则调用getMetamethod进行实际查找
 *
 * @param metatable 元表指针
 * @param event 元方法类型（必须 <= TM_EQ）
 * @return 找到的元方法值，如果不存在返回nil
 *
 * @note 对应C实现的 fasttm() 宏
 * @see lua_c_analysis/src/ltm.h 第434行
 */
Value fastMetamethod(Table* metatable, TMS event);

/**
 * @brief 将TMS枚举转换为字符串名称
 *
 * @param event 元方法类型
 * @return 元方法名称字符串（如"__add"）
 */
inline const char* getMetamethodName(TMS event) {
    return kMetamethodNames[static_cast<usize>(event)];
}

} // namespace Lua

