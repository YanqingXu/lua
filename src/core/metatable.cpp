/**
 * @file metatable.cpp
 * @brief Lua元方法系统实现
 * 
 * 详细说明：
 * 实现元方法的查找、调用和缓存机制。
 * 
 * @author Lua C++ Project
 * @date 2025-11-22
 */

#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "vm/lua_state.hpp"
#include "vm/global_state.hpp"
#include "vm/vm.hpp"
#include <stdexcept>

namespace Lua {

// =====================================================================
// 元方法名称数组（按TMS枚举顺序）
// =====================================================================

/**
 * @brief 元方法名称字符串数组
 * 
 * 顺序必须与TMS枚举完全一致。
 * 
 * @see lua_c_analysis/src/ltm.c 第203-208行
 */
const char* const kMetamethodNames[static_cast<usize>(TMS::TM_N)] = {
    "__index",      // TM_INDEX
    "__newindex",   // TM_NEWINDEX
    "__gc",         // TM_GC
    "__mode",       // TM_MODE
    "__eq",         // TM_EQ
    "__add",        // TM_ADD
    "__sub",        // TM_SUB
    "__mul",        // TM_MUL
    "__div",        // TM_DIV
    "__mod",        // TM_MOD
    "__pow",        // TM_POW
    "__unm",        // TM_UNM
    "__len",        // TM_LEN
    "__lt",         // TM_LT
    "__le",         // TM_LE
    "__concat",     // TM_CONCAT
    "__call"        // TM_CALL
};

// =====================================================================
// 元方法查找函数实现
// =====================================================================

/**
 * @brief 从元表中查找指定的元方法
 * 
 * 实现标志位缓存机制，避免重复查找不存在的元方法。
 * 
 * @see lua_c_analysis/src/ltm.c 第282-290行
 */
Value getMetamethod(Table* metatable, TMS event) {
    // 1. 检查元表是否为空
    if (metatable == nullptr) {
        return Value();  // 返回nil
    }

    // 2. 对于快速元方法（TM_INDEX到TM_EQ），检查flags缓存
    if (event <= TMS::TM_EQ) {
        u8 eventBit = 1u << static_cast<u8>(event);
        if (metatable->getFlags() & eventBit) {
            // 标志位表示该元方法不存在，直接返回nil
            return Value();  // 返回nil
        }
    }

    // 3. 在元表中查找元方法名称对应的值
    const char* name = kMetamethodNames[static_cast<usize>(event)];

    // 使用StringPool获取内部化字符串，确保相同内容的字符串有相同的指针
    StringPool& pool = GlobalState::getInstance().getStringPool();
    GCString* nameStr = pool.intern(name);
    Value key = Value(nameStr);
    Value result = metatable->get(key);
    
    // 4. 如果未找到且是快速元方法，更新flags标志位
    if (result.isNil() && event <= TMS::TM_EQ) {
        u8 eventBit = 1u << static_cast<u8>(event);
        metatable->setFlags(metatable->getFlags() | eventBit);
    }
    
    return result;
}

/**
 * @brief 根据对象类型查找元方法
 * 
 * @see lua_c_analysis/src/ltm.c 第368-381行
 */
Value getMetamethodByObject(LuaState* L, const Value& obj, TMS event) {
    Table* metatable = nullptr;
    
    // 根据对象类型获取元表
    switch (obj.getType()) {
        case ValueType::Table: {
            // 表类型：使用对象自身的元表
            Table* table = obj.asTable();
            metatable = table->getMetatable();
            break;
        }
        
        case ValueType::Userdata: {
            // 用户数据类型：使用对象自身的元表
            Userdata* udata = obj.asUserdata();
            metatable = udata->getMetatable();
            break;
        }
        
        default: {
            // 基础类型：使用 GlobalState 中按 ValueType 存放的全局元表。
            if (L == nullptr) {
                return Value();
            }
            metatable = L->getGlobalState().getMetatable(obj.getType());
            break;
        }
    }

    // 如果没有元表，返回nil
    if (metatable == nullptr) {
        return Value();  // 返回nil
    }
    
    // 在元表中查找元方法
    return getMetamethod(metatable, event);
}

// =====================================================================
// 快速元方法访问（带缓存优化）
// =====================================================================

/**
 * @brief 快速元方法访问
 * 
 * @see lua_c_analysis/src/ltm.h 第434行
 */
Value fastMetamethod(Table* metatable, TMS event) {
    // 快速元方法必须 <= TM_EQ
    if (event > TMS::TM_EQ) {
        throw std::runtime_error("fastMetamethod: event must be <= TM_EQ");
    }

    return getMetamethod(metatable, event);
}

// =====================================================================
// 元方法调用函数实现
// =====================================================================

/**
 * @brief 调用元方法并获取返回值
 *
 * @see lua_c_analysis/src/lvm.c 第396-409行 callTMres()
 */
void callTMWithResult(LuaState* L, Value& result, const Value& metamethod,
                      const Value& arg1, const Value& arg2) {
    // 检查元方法是否是函数
    if (!metamethod.isFunction()) {
        throw std::runtime_error("Metamethod is not a function");
    }

    Stack& stack = L->getStack();
    usize savedTop = L->getAbsoluteTop();
    usize savedStackSize = stack.size();
    auto restoreStack = [&]() {
        for (usize i = savedTop; i < savedStackSize && i < stack.size(); i++) {
            stack.at(i) = Value();
        }
        stack.setTop(savedStackSize);
        L->setAbsoluteTop(savedTop);
    };

    try {
        L->pushValue(metamethod);
        L->pushValue(arg1);
        L->pushValue(arg2);

        VM::call(L, 2, 1);

        if (L->getAbsoluteTop() > savedTop) {
            result = stack.at(savedTop);
        } else {
            result = Value();
        }

        restoreStack();
    } catch (...) {
        restoreStack();
        throw;
    }
}

/**
 * @brief 调用元方法（无返回值）
 *
 * @see lua_c_analysis/src/lvm.c 第453-463行
 */
void callTM(LuaState* L, const Value& metamethod, const Value& arg1,
            const Value& arg2, const Value& arg3) {
    // 检查元方法是否是函数
    if (!metamethod.isFunction()) {
        throw std::runtime_error("Metamethod is not a function");
    }

    Stack& stack = L->getStack();
    usize savedTop = L->getAbsoluteTop();
    usize savedStackSize = stack.size();
    auto restoreStack = [&]() {
        for (usize i = savedTop; i < savedStackSize && i < stack.size(); i++) {
            stack.at(i) = Value();
        }
        stack.setTop(savedStackSize);
        L->setAbsoluteTop(savedTop);
    };

    try {
        L->pushValue(metamethod);
        L->pushValue(arg1);
        L->pushValue(arg2);
        L->pushValue(arg3);

        VM::call(L, 3, 0);

        restoreStack();
    } catch (...) {
        restoreStack();
        throw;
    }
}

/**
 * @brief 调用二元运算元方法
 *
 * @see lua_c_analysis/src/lvm.c 第689-698行
 */
bool callBinaryTM(LuaState* L, const Value& p1, const Value& p2,
                  Value& result, TMS event) {
    // 1. 先尝试左操作数的元方法
    Value tm = getMetamethodByObject(L, p1, event);

    // 2. 如果左操作数没有元方法，尝试右操作数
    if (tm.isNil()) {
        tm = getMetamethodByObject(L, p2, event);
    }

    // 3. 如果都没有元方法，返回false
    if (tm.isNil()) {
        return false;
    }

    // 4. 调用找到的元方法
    callTMWithResult(L, result, tm, p1, p2);
    return true;
}

/**
 * @brief 获取比较元方法并检查对称性
 *
 * @see lua_c_analysis/src/lvm.c 第738-750行
 */
Value getComparisonTM(LuaState* L, Table* mt1, Table* mt2, TMS event) {
    // 1. 获取第一个元表的元方法
    Value tm1 = fastMetamethod(mt1, event);

    // 2. 如果没有元方法，返回nil
    if (tm1.isNil()) {
        return Value();  // 返回nil
    }

    // 3. 如果两个元表相同，直接返回元方法
    if (mt1 == mt2) {
        return tm1;
    }

    // 4. 获取第二个元表的元方法
    Value tm2 = fastMetamethod(mt2, event);

    // 5. 如果第二个元方法不存在，返回nil
    if (tm2.isNil()) {
        return Value();  // 返回nil
    }

    // 6. 比较两个元方法是否相同（原始相等性）
    if (tm1 == tm2) {  // 使用Value的operator==
        return tm1;
    }

    // 7. 元方法不同，返回nil
    return Value();  // 返回nil
}

/**
 * @brief 调用比较运算元方法
 *
 * @see lua_c_analysis/src/lvm.c 第789-800行
 */
i32 callOrderTM(LuaState* L, const Value& p1, const Value& p2, TMS event) {
    // 1. 获取第一个操作数的元方法
    Value tm1 = getMetamethodByObject(L, p1, event);

    // 2. 如果没有元方法，返回-1
    if (tm1.isNil()) {
        return -1;
    }

    // 3. 获取第二个操作数的元方法
    Value tm2 = getMetamethodByObject(L, p2, event);

    // 4. 检查两个元方法是否相同（对称性要求）
    if (!(tm1 == tm2)) {  // 使用Value的operator==
        return -1;
    }

    // 5. 调用元方法并获取结果
    Value result;
    callTMWithResult(L, result, tm1, p1, p2);

    // 6. 将结果转换为布尔值
    // false和nil被视为false，其他值被视为true
    if (result.isNil() || (result.isBoolean() && !result.asBoolean())) {
        return 0;  // false
    } else {
        return 1;  // true
    }
}

} // namespace Lua

