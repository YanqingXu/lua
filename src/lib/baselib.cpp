/**
 * @file baselib.cpp
 * @brief Lua基础库实现
 * 
 * @author Lua C++ Project
 * @date 2025-11-13
 */

#include "lib/baselib.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "vm/global_state.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace Lua {

// =====================================================================
// print(...) - 打印任意数量的参数到标准输出
// =====================================================================

i32 luaB_print(LuaState* L) {
    i32 n = L->getTop();  // 参数数量
    
    for (i32 i = 1; i <= n; i++) {
        const char* s = nullptr;
        
        // 尝试将值转换为字符串
        if (L->isString(i)) {
            s = L->toString(i);
        } else if (L->isNumber(i)) {
            s = L->toString(i);
        } else if (L->isBoolean(i)) {
            s = L->toBoolean(i) ? "true" : "false";
        } else if (L->isNil(i)) {
            s = "nil";
        } else if (L->isTable(i)) {
            // 表类型显示地址
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "table: %p", (void*)&L->at(i));
            s = buffer;
        } else if (L->isFunction(i)) {
            // 函数类型显示地址
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "function: %p", (void*)&L->at(i));
            s = buffer;
        } else {
            s = "unknown";
        }
        
        if (i > 1) {
            std::fputs("\t", stdout);
        }
        std::fputs(s, stdout);
    }
    std::fputs("\n", stdout);
    std::fflush(stdout);
    
    return 0;  // 不返回值
}

// =====================================================================
// type(v) - 返回值的类型字符串
// =====================================================================

i32 luaB_type(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("type: missing argument");
    }
    
    i32 t = L->type(1);
    const char* typeName = L->typeName(t);
    
    // 创建字符串并压入栈
    GCString* str = L->getGlobalState().getStringPool().intern(typeName);
    L->pushString(str);
    
    return 1;
}

// =====================================================================
// tostring(v) - 将值转换为字符串
// =====================================================================

i32 luaB_tostring(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("tostring: missing argument");
    }
    
    const char* s = nullptr;
    char buffer[128];
    
    // 检查 __tostring 元方法
    if (L->getMetatable(1)) {
        // 元表在栈顶
        Value mt = L->pop();
        if (mt.isTable()) {
            GCString* tostringKey = L->getGlobalState().getStringPool().intern("__tostring");
            Value tostringMethod = mt.asTable()->get(Value(tostringKey));
            if (tostringMethod.isFunction()) {
                // 调用 __tostring 元方法
                L->pushFunction(tostringMethod.asFunction());
                L->pushValue(1);  // 对象本身作为参数
                // TODO: 这里需要调用函数执行机制
                // 暂时跳过元方法调用，使用默认转换
            }
        }
    }
    
    if (L->isString(1)) {
        s = L->toString(1);
    } else if (L->isNumber(1)) {
        s = L->toString(1);
    } else if (L->isBoolean(1)) {
        s = L->toBoolean(1) ? "true" : "false";
    } else if (L->isNil(1)) {
        s = "nil";
    } else if (L->isTable(1)) {
        std::snprintf(buffer, sizeof(buffer), "table: %p", (void*)&L->at(1));
        s = buffer;
    } else if (L->isFunction(1)) {
        std::snprintf(buffer, sizeof(buffer), "function: %p", (void*)&L->at(1));
        s = buffer;
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s: %p", L->typeName(L->type(1)), (void*)&L->at(1));
        s = buffer;
    }
    
    GCString* str = L->getGlobalState().getStringPool().intern(s);
    L->pushString(str);
    
    return 1;
}

// =====================================================================
// tonumber(e [, base]) - 将值转换为数字
// =====================================================================

i32 luaB_tonumber(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("tonumber: missing argument");
    }
    
    i32 base = 10;
    if (L->getTop() >= 2) {
        if (!L->isNumber(2)) {
            L->error("tonumber: base must be a number");
        }
        base = static_cast<i32>(L->toNumber(2));
        if (base < 2 || base > 36) {
            L->error("tonumber: base out of range");
        }
    }
    
    // 如果已经是数字，直接返回
    if (L->isNumber(1)) {
        L->pushNumber(L->toNumber(1));
        return 1;
    }
    
    // 尝试从字符串转换
    if (L->isString(1)) {
        const char* s = L->toString(1);
        if (!s || *s == '\0') {
            L->pushNil();
            return 1;
        }
        
        // 跳过前导空白
        while (std::isspace(*s)) s++;
        
        // 处理符号
        bool negative = false;
        if (*s == '-') {
            negative = true;
            s++;
        } else if (*s == '+') {
            s++;
        }
        
        // 转换数字
        f64 result = 0.0;
        bool hasDigit = false;
        
        if (base == 10) {
            // 十进制：支持小数点和科学计数法
            char* endptr = nullptr;
            result = std::strtod(s, &endptr);
            if (endptr == s) {
                L->pushNil();
                return 1;
            }
            // 检查是否有非数字字符（跳过尾部空白）
            while (std::isspace(*endptr)) endptr++;
            if (*endptr != '\0') {
                L->pushNil();
                return 1;
            }
            hasDigit = true;
        } else {
            // 其他进制：只支持整数
            while (*s != '\0' && !std::isspace(*s)) {
                i32 digit = -1;
                if (*s >= '0' && *s <= '9') {
                    digit = *s - '0';
                } else if (*s >= 'a' && *s <= 'z') {
                    digit = *s - 'a' + 10;
                } else if (*s >= 'A' && *s <= 'Z') {
                    digit = *s - 'A' + 10;
                } else {
                    break;
                }
                
                if (digit >= base) {
                    break;
                }
                
                result = result * base + digit;
                hasDigit = true;
                s++;
            }
            
            // 检查是否有非数字字符
            while (std::isspace(*s)) s++;
            if (*s != '\0') {
                hasDigit = false;
            }
        }
        
        if (hasDigit) {
            if (negative) result = -result;
            L->pushNumber(result);
            return 1;
        }
    }

    L->pushNil();
    return 1;
}

// =====================================================================
// error(message [, level]) - 抛出错误
// =====================================================================

i32 luaB_error(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("error: missing argument");
    }

    i32 level = 1;
    if (L->getTop() >= 2 && L->isNumber(2)) {
        level = static_cast<i32>(L->toNumber(2));
    }

    // 简化实现：直接抛出错误
    // TODO: 添加位置信息（根据level参数）
    L->setTop(1);  // 只保留错误消息
    return L->error();
}

// =====================================================================
// assert(v [, message]) - 断言
// =====================================================================

i32 luaB_assert(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("assert: missing argument");
    }

    if (!L->toBoolean(1)) {
        // 断言失败
        if (L->getTop() >= 2) {
            // 使用提供的错误消息
            L->setTop(2);
            return L->error();
        } else {
            // 使用默认错误消息
            L->error("assertion failed!");
        }
    }

    // 断言成功，返回所有参数
    return L->getTop();
}

// =====================================================================
// setmetatable(table, metatable) - 设置表的元表
// =====================================================================

i32 luaB_setmetatable(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("setmetatable: missing arguments");
    }

    if (!L->isTable(1)) {
        L->error("setmetatable: first argument must be a table");
    }

    i32 t = L->type(2);
    if (t != 0 && t != 5) {  // 不是nil也不是table
        L->error("setmetatable: second argument must be nil or table");
    }

    // 检查 __metatable 字段（保护机制）
    if (L->getMetatable(1)) {
        // 如果表已经有元表，检查是否受保护
        Value oldMt = L->pop();
        if (oldMt.isTable()) {
            GCString* metatableKey = L->getGlobalState().getStringPool().intern("__metatable");
            Value protectedField = oldMt.asTable()->get(Value(metatableKey));
            if (!protectedField.isNil()) {
                // 元表受保护，不能修改
                L->error("cannot change a protected metatable");
            }
        }
    }

    if (!L->setMetatable(1)) {
        L->error("setmetatable: cannot set metatable");
    }

    L->setTop(1);  // 只保留表
    return 1;
}

// =====================================================================
// getmetatable(object) - 获取对象的元表
// =====================================================================

i32 luaB_getmetatable(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("getmetatable: missing argument");
    }

    if (!L->getMetatable(1)) {
        L->pushNil();
        return 1;
    }

    // 检查 __metatable 字段
    Value mt = L->top();
    if (mt.isTable()) {
        GCString* metatableKey = L->getGlobalState().getStringPool().intern("__metatable");
        Value protectedField = mt.asTable()->get(Value(metatableKey));
        if (!protectedField.isNil()) {
            // 如果设置了 __metatable 字段，返回该字段的值而不是元表本身
            L->pop();
            L->getStack().push(protectedField);
        }
    }

    return 1;
}

// =====================================================================
// 迭代器函数
// =====================================================================

/**
 * @brief next(table [, index])
 *
 * 返回表中的下一个键值对。如果index为nil，返回第一个键值对。
 * 如果index是表中的最后一个键，返回nil。
 *
 * 参考：lua_c_analysis/src/lbaselib.c 中的luaB_next
 */
static i32 luaB_next(LuaState* L) {
    Stack& stack = L->getStack();

    if (stack.size() < 1) {
        throw std::runtime_error("next: table expected");
    }

    Value tableVal = stack.at(0);
    if (!tableVal.isTable()) {
        throw std::runtime_error("next: table expected");
    }

    Table* table = tableVal.asTable();
    Value key = stack.size() > 1 ? stack.at(1) : Value();  // nil if not provided

    // 获取下一个键值对
    Value nextKey, nextValue;
    if (table->next(key, nextKey, nextValue)) {
        // 找到下一个键值对
        stack.push(nextKey);
        stack.push(nextValue);
        return 2;
    } else {
        // 没有更多元素
        return 0;
    }
}

/**
 * @brief pairs(t)
 *
 * 返回三个值：迭代器函数、表、nil（初始键）
 * 用于泛型for循环遍历表的所有键值对。
 *
 * 参考：lua_c_analysis/src/lbaselib.c 中的luaB_pairs
 */
static i32 luaB_pairs(LuaState* L) {
    Stack& stack = L->getStack();

    if (stack.size() < 1) {
        throw std::runtime_error("pairs: table expected");
    }

    Value tableVal = stack.at(0);
    if (!tableVal.isTable()) {
        throw std::runtime_error("pairs: table expected");
    }

    // 返回：next函数, 表, nil
    GlobalState& gs = L->getGlobalState();

    // 创建next函数对象
    Function* nextFunc = new Function(luaB_next);
    gs.getGC().registerObject(nextFunc);

    stack.push(Value(nextFunc));
    stack.push(tableVal);
    stack.push(Value());  // nil

    return 3;
}

/**
 * @brief ipairs迭代器辅助函数
 * 
 * 接收(table, index)，返回(index+1, value)
 */
static i32 ipairsIter(LuaState* L) {
    Stack& stack = L->getStack();

    if (stack.size() < 2) {
        return 0;
    }

    Value tableVal = stack.at(0);
    Value indexVal = stack.at(1);

    if (!tableVal.isTable() || !indexVal.isNumber()) {
        return 0;
    }

    Table* table = tableVal.asTable();
    i32 index = static_cast<i32>(indexVal.asNumber());
    i32 nextIndex = index + 1;

    // 获取下一个元素
    Value nextValue = table->getArray(nextIndex);
    if (nextValue.isNil()) {
        return 0;  // 结束迭代
    }

    stack.push(Value(static_cast<f64>(nextIndex)));
    stack.push(nextValue);
    return 2;
}

/**
 * @brief ipairs(t)
 *
 * 返回三个值：迭代器函数、表、0（初始索引）
 * 用于泛型for循环遍历表的数组部分。
 *
 * 参考：lua_c_analysis/src/lbaselib.c 中的luaB_ipairs
 */
static i32 luaB_ipairs(LuaState* L) {
    Stack& stack = L->getStack();

    if (stack.size() < 1) {
        throw std::runtime_error("ipairs: table expected");
    }

    Value tableVal = stack.at(0);
    if (!tableVal.isTable()) {
        throw std::runtime_error("ipairs: table expected");
    }

    // 创建ipairs迭代器函数
    GlobalState& gs = L->getGlobalState();

    Function* iterFunc = new Function(ipairsIter);
    gs.getGC().registerObject(iterFunc);

    stack.push(Value(iterFunc));
    stack.push(tableVal);
    stack.push(Value(0.0));  // 初始索引0

    return 3;
}

// =====================================================================
// 注册基础库
// =====================================================================

void openBaseLib(LuaState* L) {
    // 创建函数对象并注册到全局表
    struct FuncReg {
        const char* name;
        CFunction func;
    };

    static const FuncReg funcs[] = {
        {"print", luaB_print},
        {"type", luaB_type},
        {"tostring", luaB_tostring},
        {"tonumber", luaB_tonumber},
        {"error", luaB_error},
        {"assert", luaB_assert},
        {"setmetatable", luaB_setmetatable},
        {"getmetatable", luaB_getmetatable},
        {"next", luaB_next},
        {"pairs", luaB_pairs},
        {"ipairs", luaB_ipairs},
        {nullptr, nullptr}
    };

    for (const FuncReg* reg = funcs; reg->name != nullptr; ++reg) {
        Function* func = new Function(reg->func);
        L->getGlobalState().getGC().registerObject(func);
        L->setGlobal(reg->name, Value(func));
    }
}

} // namespace Lua

