/**
 * @file metatable.cpp
 * @brief Lua元方法系统实现
 *
 * 详细说明：
 * 实现元方法的查找、调用和缓存机制。
 *
 * @author Lua C++ 项目
 * @date 2025-11-22
 */

#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm.hpp"
#include "runtime/runtime_services.hpp"
#include <stdexcept>

namespace Lua {

// =====================================================================
// 元方法查找函数实现
// =====================================================================

/**
 * @brief 从元表中查找指定的元方法
 *
 * 实现标志位缓存机制，避免重复查找不存在的元方法。
 */
Value getMetamethod(Table* metatable, TMS event) {
    return getMetamethod(GlobalState::getInstance(), metatable, event);
}

Value getMetamethod(GlobalState& globalState, Table* metatable, TMS event) {
    // 1. 检查元表是否为空
    if (metatable == nullptr) {
        return Value(); // 返回nil
    }

    // 2. 对于快速元方法（TM_INDEX到TM_EQ），检查flags缓存
    if (event <= TMS::TM_EQ) {
        u8 eventBit = 1u << static_cast<u8>(event);
        if (metatable->getFlags() & eventBit) {
            // 标志位表示该元方法不存在，直接返回nil
            return Value(); // 返回nil
        }
    }

    // 3. 在元表中查找元方法名称对应的值
    StrView name = kMetamethodNames[static_cast<usize>(event)];

    // 使用StringPool获取内部化字符串，确保相同内容的字符串有相同的指针
    StringPool& pool = globalState.getStringPool();
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
        return Value(); // 返回nil
    }

    GlobalState& globalState = (L != nullptr) ? L->getGlobalState() : GlobalState::getInstance();
    return getMetamethod(globalState, metatable, event);
}

// =====================================================================
// 快速元方法访问（带缓存优化）
// =====================================================================

/**
 * @brief 快速元方法访问
 */
Value fastMetamethod(Table* metatable, TMS event) {
    return fastMetamethod(GlobalState::getInstance(), metatable, event);
}

Value fastMetamethod(GlobalState& globalState, Table* metatable, TMS event) {
    // 快速元方法必须 <= TM_EQ
    if (event > TMS::TM_EQ) {
        throw std::runtime_error("fastMetamethod: event must be <= TM_EQ");
    }

    return getMetamethod(globalState, metatable, event);
}

// =====================================================================
// 元方法调用函数实现
// =====================================================================

/**
 * @brief 调用元方法并获取返回值
 */
void callTMWithResult(LuaState* L, Value& result, const Value& metamethod, const Value& arg1, const Value& arg2) {
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

        RuntimeServices services(L->getGlobalState());
        VM::call(services, L, 2, 1);

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
 */
void callTM(LuaState* L, const Value& metamethod, const Value& arg1, const Value& arg2, const Value& arg3) {
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

        RuntimeServices services(L->getGlobalState());
        VM::call(services, L, 3, 0);

        restoreStack();
    } catch (...) {
        restoreStack();
        throw;
    }
}

/**
 * @brief 调用二元运算元方法
 */
bool callBinaryTM(LuaState* L, const Value& p1, const Value& p2, Value& result, TMS event) {
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
 */
Value getComparisonTM(LuaState* L, Table* mt1, Table* mt2, TMS event) {
    GlobalState& globalState = (L != nullptr) ? L->getGlobalState() : GlobalState::getInstance();

    // 1. 获取第一个元表的元方法
    Value tm1 = fastMetamethod(globalState, mt1, event);

    // 2. 如果没有元方法，返回nil
    if (tm1.isNil()) {
        return Value(); // 返回nil
    }

    // 3. 如果两个元表相同，直接返回元方法
    if (mt1 == mt2) {
        return tm1;
    }

    // 4. 获取第二个元表的元方法
    Value tm2 = fastMetamethod(globalState, mt2, event);

    // 5. 如果第二个元方法不存在，返回nil
    if (tm2.isNil()) {
        return Value(); // 返回nil
    }

    // 6. 比较两个元方法是否相同（原始相等性）
    if (tm1 == tm2) { // 使用Value的operator==
        return tm1;
    }

    // 7. 元方法不同，返回nil
    return Value(); // 返回nil
}

/**
 * @brief 调用比较运算元方法
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
    if (!(tm1 == tm2)) { // 使用Value的operator==
        return -1;
    }

    // 5. 调用元方法并获取结果
    Value result;
    callTMWithResult(L, result, tm1, p1, p2);

    // 6. 将结果转换为布尔值
    // false和nil被视为false，其他值被视为true
    if (result.isNil() || (result.isBoolean() && !result.asBoolean())) {
        return 0; // false
    } else {
        return 1; // true
    }
}

} // namespace Lua
