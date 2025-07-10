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
        std::cerr << "Error: Invalid parameters for registerTableFunction - ";
        std::cerr << "state=" << (state ? "OK" : "NULL") << ", ";
        std::cerr << "name=" << (name ? name : "NULL") << ", ";
        std::cerr << "func=" << (func ? "OK" : "NULL") << ", ";
        std::cerr << "table.isTable()=" << (table.isTable() ? "true" : "false") << std::endl;
        return;
    }

    std::cout << "[LibRegistry] Registering table function: " << name << std::endl;

    // Create Native function object and add to table
    NativeFn nativeFn = [func](State* s, int n) -> Value {
        return func(s, static_cast<i32>(n));
    };

    auto cfunction = Function::createNative(nativeFn);
    auto tableRef = table.asTable();
    tableRef->set(Value(name), Value(cfunction));

    std::cout << "[LibRegistry] Successfully registered table function: " << name << std::endl;
}

Value LibRegistry::createLibTable(State* state, const char* libName) {
    if (!state || !libName) {
        std::cerr << "Error: Invalid parameters for createLibTable" << std::endl;
        return Value();
    }
    
    std::cout << "[LibRegistry] Creating library table: " << libName << std::endl;
    
    // Create new table
    auto table = GCRef<Table>(new Table());
    Value tableValue(table);
    
    // Register to global environment
    state->setGlobal(libName, tableValue);
    
    std::cout << "[LibRegistry] Successfully created library table: " << libName << std::endl;
    
    return tableValue;
}

} // namespace Lua
