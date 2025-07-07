#pragma once

#include "../../common/types.hpp"
#include "../../vm/state.hpp"
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
 * @brief Lua C function type definition
 *
 * Represents a C function that can be called from Lua code.
 * @param state Lua state containing function arguments
 * @param nargs Number of arguments passed to the function
 * @return Value returned to Lua
 */
using LuaCFunction = Value(*)(State* state, i32 nargs);

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
     * @brief Register module functions to State
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    virtual void registerFunctions(State* state) = 0;

    /**
     * @brief Optional initialization function
     * @param state Lua state to initialize
     *
     * Default implementation does nothing. Override if module
     * needs special initialization (e.g., setting constants).
     */
    virtual void initialize(State* state) {
        (void)state; // Default empty implementation
    }
};

} // namespace Lua
