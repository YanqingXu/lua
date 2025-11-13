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
    
    // TODO: 检查__tostring元方法
    
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
    
    if (base == 10) {
        // 标准转换
        if (L->isNumber(1)) {
            L->pushNumber(L->toNumber(1));
            return 1;
        }
    }
    
    // TODO: 实现字符串到数字的转换（支持不同进制）

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

    // TODO: 检查__metatable字段（保护机制）

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
    }

    // TODO: 检查__metatable字段

    return 1;
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
        {nullptr, nullptr}
    };

    for (const FuncReg* reg = funcs; reg->name != nullptr; ++reg) {
        Function* func = new Function(reg->func);
        L->getGlobalState().getGC().registerObject(func);
        L->setGlobal(reg->name, Value(func));
    }
}

} // namespace Lua

