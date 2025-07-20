#include "lib_registry.hpp"
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

    // Create Native function object using new multi-return function type
    NativeFn nativeFn = [func](State* s) -> i32 {
        return func(s);
    };

    auto cfunction = Function::createNative(nativeFn);
    state->setGlobal(name, Value(cfunction));
}

void LibRegistry::registerTableFunction(State* state, Value table, const char* name, LuaCFunction func) {
    if (!state || !name || !func || !table.isTable()) {
        return;
    }

    // Create Native function object using new multi-return function type
    NativeFn nativeFn = [func](State* s) -> i32 {
        return func(s);
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

void LibRegistry::registerGlobalFunctionLegacy(State* state, const char* name, LuaCFunctionLegacy func) {
    if (!state || !name || !func) {
        return;
    }

    // Create Native function object using legacy function type
    NativeFnLegacy nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNativeLegacy(nativeFn);
    state->setGlobal(name, Value(cfunction));
}

void LibRegistry::registerTableFunctionLegacy(State* state, Value table, const char* name, LuaCFunctionLegacy func) {
    if (!state || !name || !func || !table.isTable()) {
        return;
    }

    // Create Native function object using legacy function type
    NativeFnLegacy nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNativeLegacy(nativeFn);
    auto tableRef = table.asTable();
    tableRef->set(Value(name), Value(cfunction));
}

} // namespace Lua
