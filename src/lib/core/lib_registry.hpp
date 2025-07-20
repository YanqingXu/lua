#pragma once

#include "lib_module.hpp"
#include "../../vm/value.hpp"

namespace Lua {

/**
 * @brief Library function registration helper
 *
 * This utility class provides static methods for registering C functions
 * to the Lua state, either as global functions or as table members.
 * It handles the conversion between LuaCFunction and NativeFn types.
 */
class LibRegistry {
public:
    /**
     * @brief Register C function to global environment (Lua 5.1 standard - multi-return)
     * @param state Lua state to register function to
     * @param name Function name in Lua
     * @param func C function pointer
     * @throws std::invalid_argument if any parameter is null
     */
    static void registerGlobalFunction(State* state, const char* name, LuaCFunction func);

    /**
     * @brief Register C function to specified table (Lua 5.1 standard - multi-return)
     * @param state Lua state containing the table
     * @param table Target table to add function to
     * @param name Function name in the table
     * @param func C function pointer
     * @throws std::invalid_argument if any parameter is null or table is invalid
     */
    static void registerTableFunction(State* state, Value table, const char* name, LuaCFunction func);

    /**
     * @brief Register legacy C function to global environment (single return value)
     * @param state Lua state to register function to
     * @param name Function name in Lua
     * @param func Legacy C function pointer
     * @throws std::invalid_argument if any parameter is null
     */
    static void registerGlobalFunctionLegacy(State* state, const char* name, LuaCFunctionLegacy func);

    /**
     * @brief Register legacy C function to specified table (single return value)
     * @param state Lua state containing the table
     * @param table Target table to add function to
     * @param name Function name in the table
     * @param func Legacy C function pointer
     * @throws std::invalid_argument if any parameter is null or table is invalid
     */
    static void registerTableFunctionLegacy(State* state, Value table, const char* name, LuaCFunctionLegacy func);
    
    /**
     * @brief Create and register library table
     * @param state Lua state to create table in
     * @param libName Library name (will be the global table name)
     * @return Created library table as Value
     * @throws std::invalid_argument if state or libName is null
     */
    static Value createLibTable(State* state, const char* libName);
};

// ===================================================================
// Convenience Macros
// ===================================================================

/**
 * @brief Register global function convenience macro
 * 
 * Usage: REGISTER_GLOBAL_FUNCTION(state, print, BaseLib::print);
 */
#define REGISTER_GLOBAL_FUNCTION(state, name, func) \
    LibRegistry::registerGlobalFunction(state, #name, func)

/**
 * @brief Register table function convenience macro
 * 
 * Usage: REGISTER_TABLE_FUNCTION(state, stringTable, len, StringLib::len);
 */
#define REGISTER_TABLE_FUNCTION(state, table, name, func) \
    LibRegistry::registerTableFunction(state, table, #name, func)

/**
 * @brief Declare Lua C function convenience macro
 * 
 * Usage: LUA_FUNCTION(myFunction) { ... }
 */
#define LUA_FUNCTION(name) \
    static Value name(State* state, i32 nargs)

/**
 * @brief Create library module class convenience macro
 * 
 * Usage: DECLARE_LIB_MODULE(MyLib, "mylib")
 */
#define DECLARE_LIB_MODULE(className, libName) \
    class className : public LibModule { \
    public: \
        const char* getName() const override { return libName; } \
        void registerFunctions(State* state) override; \
        void initialize(State* state) override; \
    }

} // namespace Lua
