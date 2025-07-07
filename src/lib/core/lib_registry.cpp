#include "lib_registry.hpp"
#include "../../vm/function.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// LibRegistry Implementation
// ===================================================================

void LibRegistry::registerGlobalFunction(State* state, const char* name, LuaCFunction func) {
    if (!state || !name || !func) {
        std::cerr << "Error: Invalid parameters for registerGlobalFunction" << std::endl;
        return;
    }

    // Create Native function object and register to global environment
    // Need to convert LuaCFunction to NativeFn
    NativeFn nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNative(nativeFn);
    state->setGlobal(name, Value(cfunction));

    #ifdef DEBUG_LIB_REGISTRATION
    std::cout << "[LibRegistry] Registered global function: " << name << std::endl;
    #endif
}

void LibRegistry::registerTableFunction(State* state, Value table, const char* name, LuaCFunction func) {
    if (!state || !name || !func || !table.isTable()) {
        std::cerr << "Error: Invalid parameters for registerTableFunction" << std::endl;
        return;
    }

    // Create Native function object and add to table
    NativeFn nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNative(nativeFn);
    auto tableRef = table.asTable();
    tableRef->set(Value(name), Value(cfunction));

    #ifdef DEBUG_LIB_REGISTRATION
    std::cout << "[LibRegistry] Registered table function: " << name << std::endl;
    #endif
}

Value LibRegistry::createLibTable(State* state, const char* libName) {
    if (!state || !libName) {
        std::cerr << "Error: Invalid parameters for createLibTable" << std::endl;
        return Value();
    }
    
    // Create new table
    auto table = GCRef<Table>(new Table());
    Value tableValue(table);
    
    // Register to global environment
    state->setGlobal(libName, tableValue);
    
    #ifdef DEBUG_LIB_REGISTRATION
    std::cout << "[LibRegistry] Created library table: " << libName << std::endl;
    #endif
    
    return tableValue;
}

} // namespace Lua
