/**
 * @file lib_registry.cpp
 * @brief Lua函数注册工具实现
 * 
 * 提供统一的函数注册实现，支持静态方法和流式接口两种使用方式。
 * 
 * @author Lua C++ Project
 * @date 2025-12-19
 */

#include "lib/lib_registry.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

// =====================================================================
// 私有辅助函数
// =====================================================================

Function* FunctionRegistrar::createClosure(LuaState* L, LibCFunction func) {
    if (!L || !func) {
        return nullptr;
    }
    Function* closure = new Function(func);
    L->getGlobalState().getGC().registerObject(closure);
    return closure;
}

// =====================================================================
// 静态方法实现：单个函数注册
// =====================================================================

void FunctionRegistrar::registerGlobal(LuaState* L, const char* name, LibCFunction func) {
    if (!L || !name || !func) {
        return;
    }

    Function* closure = createClosure(L, func);
    if (!closure) {
        return;
    }

    L->setGlobal(name, Value(closure));
}

void FunctionRegistrar::registerToTable(LuaState* L, Table* table, const char* name, LibCFunction func) {
    if (!L || !table || !name || !func) {
        return;
    }

    Function* closure = createClosure(L, func);
    if (!closure) {
        return;
    }

    GCString* key = L->getGlobalState().getStringPool().intern(name);
    table->set(Value(key), Value(closure));
}

// =====================================================================
// 静态方法实现：库表创建
// =====================================================================

Table* FunctionRegistrar::createLibTable(LuaState* L, const char* libName) {
    if (!L || !libName) {
        return nullptr;
    }

    Table* table = new Table();
    L->getGlobalState().getGC().registerObject(table);
    L->setGlobal(libName, Value(table));
    return table;
}

} // namespace Lua
