#pragma once

#include "../core/lib_module.hpp"
#include "../core/lib_registry.hpp"
#include "../../vm/call_result.hpp"

namespace Lua {
    // Forward declarations
    class Table;
    class Userdata;

/**
 * @brief Base library implementation using the new framework
 *
 * Provides essential Lua core functions:
 * - print: Output function for displaying values
 * - type: Type checking function
 * - tostring: String conversion function
 * - tonumber: Number conversion function
 * - error: Error throwing function
 *
 * This implementation follows the simplified standard library framework
 * design principles for better maintainability and performance.
 */
class BaseLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "base"; }

    /**
     * @brief Register all base library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(LuaState* state) override;

    /**
     * @brief Initialize the base library with global constants
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(LuaState* state) override;

    // Basic function declarations
    /**
     * @brief Print function implementation
     * @param state Lua state containing arguments
     * @param nargs Number of arguments on stack
     * @return nil value
     * @throws std::invalid_argument if state is null or nargs is negative
     */
    static Value print(LuaState* state, i32 nargs);

    /**
     * @brief Type checking function
     * @param state Lua state containing value to check
     * @param nargs Number of arguments (should be 1)
     * @return String value representing the type
     * @throws std::invalid_argument if state is null
     */
    static Value type(LuaState* state, i32 nargs);

    /**
     * @brief String conversion function
     * @param state Lua state containing value to convert
     * @param nargs Number of arguments (should be 1)
     * @return String representation of the value
     * @throws std::invalid_argument if state is null
     */
    static Value tostring(LuaState* state, i32 nargs);

    /**
     * @brief Number conversion function
     * @param state Lua state containing value to convert
     * @param nargs Number of arguments (should be 1)
     * @return Number value or nil if conversion fails
     * @throws std::invalid_argument if state is null
     */
    static Value tonumber(LuaState* state, i32 nargs);

    /**
     * @brief Error throwing function
     * @param state Lua state containing error message
     * @param nargs Number of arguments
     * @return nil value (function doesn't return normally)
     * @throws std::invalid_argument if state is null
     */
    static Value error(LuaState* state, i32 nargs);

    /**
     * @brief Protected call function (pcall) - Lua 5.1 standard
     * @param state Lua state containing function and arguments on stack
     * @return Number of return values pushed to stack: (true, result...) or (false, error_message)
     * @throws std::invalid_argument if state is null
     */
    static i32 pcall(LuaState* state);

    // Table operation functions (simplified implementations)
    static Value pairs(LuaState* state, i32 nargs);
    static Value ipairs(LuaState* state, i32 nargs);
    static Value next(LuaState* state, i32 nargs);

    // Multi-return iterator functions
    static i32 pairsMulti(LuaState* state);
    static i32 ipairsMulti(LuaState* state);
    static i32 nextMulti(LuaState* state);

    // Metatable operation functions (simplified implementations)
    static Value getmetatable(LuaState* state, i32 nargs);
    static Value setmetatable(LuaState* state, i32 nargs);

    // Multi-return versions
    static i32 getmetatableMulti(LuaState* state);
    static i32 setmetatableMulti(LuaState* state);
    static Value rawget(LuaState* state, i32 nargs);
    static Value rawset(LuaState* state, i32 nargs);
    static Value rawlen(LuaState* state, i32 nargs);
    static Value rawequal(LuaState* state, i32 nargs);

    // Other utility functions (simplified implementations)
    //static Value assert(LuaState* state, i32 nargs);
    static Value select(LuaState* state, i32 nargs);
    static Value unpack(LuaState* state, i32 nargs);
};

/**
 * @brief Convenient BaseLib initialization function
 * @param state Lua state to initialize base library for
 * @throws std::invalid_argument if state is null
 */
void initializeBaseLib(LuaState* state);

} // namespace Lua
