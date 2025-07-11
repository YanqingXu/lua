﻿#include "lib_registry.hpp"
#include "../../vm/function.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include "../../common/defines.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// LibRegistry Implementation
// ===================================================================

void LibRegistry::registerGlobalFunction(State* state, const char* name, LuaCFunction func) {
    if (!state || !name || !func) {
        return;
    }

    // Create Native function object and register to global environment
    // Need to convert LuaCFunction to NativeFn
    NativeFn nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNative(nativeFn);
    state->setGlobal(name, Value(cfunction));
}

void LibRegistry::registerTableFunction(State* state, Value table, const char* name, LuaCFunction func) {
    if (!state || !name || !func || !table.isTable()) {
        return;
    }

    // Create Native function object and add to table
    NativeFn nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNative(nativeFn);
    auto tableRef = table.asTable();
    tableRef->set(Value(name), Value(cfunction));
}

Value LibRegistry::createLibTable(State* state, const char* libName) {
    if (!state || !libName) {
        return Value();
    }

    // Create new table
    auto table = GCRef<Table>(new Table());
    Value tableValue(table);

    // Register to global environment
    state->setGlobal(libName, tableValue);

    return tableValue;
}

} // namespace Lua
