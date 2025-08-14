#include "lib_registry.hpp"
#include "../../vm/lua_state.hpp"
#include "../../gc/core/gc_string.hpp"
#include "../../vm/function.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include "../../common/defines.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// LibRegistry Implementation
// ===================================================================

void LibRegistry::registerGlobalFunction(LuaState* state, const char* name, LuaCFunction func) {
    if (!state || !name || !func) {
        return;
    }

    // Create Native function object using new multi-return function type
    NativeFn nativeFn = [func](LuaState* s) -> i32 {
        return func(s);
    };

    auto cfunction = Function::createNative(nativeFn);
    // Convert const char* to GCString* for setGlobal
    auto str = GCString::create(name);
    state->setGlobal(str, Value(cfunction));
}

void LibRegistry::registerTableFunction(LuaState* state, Value table, const char* name, LuaCFunction func) {
    if (!state || !name || !func || !table.isTable()) {
        return;
    }

    // Create Native function object using new multi-return function type
    NativeFn nativeFn = [func](LuaState* s) -> i32 {
        return func(s);
    };

    auto cfunction = Function::createNative(nativeFn);
    auto tableRef = table.asTable();
    auto str = GCString::create(name);
    tableRef->set(Value(str->getString()), Value(cfunction));
}

Value LibRegistry::createLibTable(LuaState* state, const char* libName) {
    if (!state || !libName) {
        return Value();
    }

    // Create new table
    auto table = GCRef<Table>(new Table());
    Value tableValue(table);

    // Register to global environment
    auto str = GCString::create(libName);
    state->setGlobal(str, tableValue);

    return tableValue;
}

void LibRegistry::registerGlobalFunctionLegacy(LuaState* state, const char* name, LuaCFunctionLegacy func) {
    if (!state || !name || !func) {
        return;
    }

    // Create Native function object using legacy function type
    NativeFnLegacy nativeFn = [func](LuaState* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNativeLegacy(nativeFn);
    auto str = GCString::create(name);
    Value functionValue(cfunction);
    state->setGlobal(str, functionValue);
}

void LibRegistry::registerTableFunctionLegacy(LuaState* state, Value table, const char* name, LuaCFunctionLegacy func) {
    if (!state || !name || !func || !table.isTable()) {
        return;
    }

    // Create Native function object using legacy function type
    NativeFnLegacy nativeFn = [func](LuaState* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNativeLegacy(nativeFn);
    auto tableRef = table.asTable();
    auto str = GCString::create(name);
    tableRef->set(Value(str->getString()), Value(cfunction));
}

} // namespace Lua
