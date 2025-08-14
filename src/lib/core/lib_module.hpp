#pragma once

#include "../../common/types.hpp"
#include "../../vm/lua_state.hpp"
#include "../../vm/value.hpp"

namespace Lua {

/**
 * @brief Library module base class
 *
 * This is the base class for all standard library modules in the simplified
 * framework. Each library (base, string, math, etc.) should inherit from
 * this class and implement the required virtual methods.
 *
 * Design principles:
 * - Simple interface with minimal virtual methods
 * - Direct registration to Lua State
 * - Clear separation of concerns
 */

// Forward declarations
class State;

/**
 * @brief Lua C function type definition (Lua 5.1 standard)
 *
 * Represents a C function that can be called from Lua code.
 * Following Lua 5.1 official standard, C functions:
 * - Receive arguments from the stack via State::get()
 * - Push return values to the stack via State::push()
 * - Return the number of values pushed to the stack
 *
 * @param state Lua state containing function arguments on stack
 * @return Number of return values pushed to the stack
 */
using LuaCFunction = i32(*)(LuaState* state);

/**
 * @brief Legacy single-return C function type (for backward compatibility)
 *
 * @param state Lua state containing function arguments
 * @param nargs Number of arguments passed to the function
 * @return Single value returned to Lua
 */
using LuaCFunctionLegacy = Value(*)(LuaState* state, i32 nargs);

/**
 * @brief Library module base class
 *
 * All standard library modules should inherit from this class
 * and implement the required virtual methods for registration
 * and initialization.
 */
class LibModule {
public:
    virtual ~LibModule() = default;

    /**
     * @brief Get module name
     * @return Module name as C string
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Register module functions to LuaState
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    virtual void registerFunctions(LuaState* state) = 0;

    /**
     * @brief Optional initialization function
     * @param state Lua state to initialize
     *
     * Default implementation does nothing. Override if module
     * needs special initialization (e.g., setting constants).
     */
    virtual void initialize(LuaState* state) {
        (void)state; // Default empty implementation
    }
};

} // namespace Lua
