#pragma once

#include "../core/lib_module.hpp"
#include "../core/lib_registry.hpp"

namespace Lua {

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
    void registerFunctions(State* state) override;

    /**
     * @brief Initialize the base library with global constants
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(State* state) override;

    // Basic function declarations
    /**
     * @brief Print function implementation
     * @param state Lua state containing arguments
     * @param nargs Number of arguments on stack
     * @return nil value
     * @throws std::invalid_argument if state is null or nargs is negative
     */
    static Value print(State* state, i32 nargs);

    /**
     * @brief Type checking function
     * @param state Lua state containing value to check
     * @param nargs Number of arguments (should be 1)
     * @return String value representing the type
     * @throws std::invalid_argument if state is null
     */
    static Value type(State* state, i32 nargs);

    /**
     * @brief String conversion function
     * @param state Lua state containing value to convert
     * @param nargs Number of arguments (should be 1)
     * @return String representation of the value
     * @throws std::invalid_argument if state is null
     */
    static Value tostring(State* state, i32 nargs);

    /**
     * @brief Number conversion function
     * @param state Lua state containing value to convert
     * @param nargs Number of arguments (should be 1)
     * @return Number value or nil if conversion fails
     * @throws std::invalid_argument if state is null
     */
    static Value tonumber(State* state, i32 nargs);

    /**
     * @brief Error throwing function
     * @param state Lua state containing error message
     * @param nargs Number of arguments
     * @return nil value (function doesn't return normally)
     * @throws std::invalid_argument if state is null
     */
    static Value error(State* state, i32 nargs);

    // Table operation functions (simplified implementations)
    static Value pairs(State* state, i32 nargs);
    static Value ipairs(State* state, i32 nargs);
    static Value next(State* state, i32 nargs);

    // Metatable operation functions (simplified implementations)
    static Value getmetatable(State* state, i32 nargs);
    static Value setmetatable(State* state, i32 nargs);
    static Value rawget(State* state, i32 nargs);
    static Value rawset(State* state, i32 nargs);
    static Value rawlen(State* state, i32 nargs);
    static Value rawequal(State* state, i32 nargs);

    // Other utility functions (simplified implementations)
    static Value assert(State* state, i32 nargs);
    static Value select(State* state, i32 nargs);
    static Value unpack(State* state, i32 nargs);
};

/**
 * @brief Convenient BaseLib initialization function
 * @param state Lua state to initialize base library for
 * @throws std::invalid_argument if state is null
 */
void initializeBaseLib(State* state);

} // namespace Lua
