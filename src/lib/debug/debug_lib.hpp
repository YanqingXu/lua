#pragma once

#include "../../common/types.hpp"
#include "../../vm/lua_state.hpp"
#include "../../vm/value.hpp"
#include "../core/lib_module.hpp"

namespace Lua {

/**
 * @brief Debug library implementation
 *
 * Provides Lua debugging and introspection functions:
 * - debug: Enter interactive debug mode
 * - getfenv: Get function environment
 * - gethook: Get current hook function
 * - getinfo: Get function information
 * - getlocal: Get local variable
 * - getmetatable: Get metatable (debug version)
 * - getregistry: Get registry table
 * - getupvalue: Get upvalue
 * - setfenv: Set function environment
 * - sethook: Set hook function
 * - setlocal: Set local variable
 * - setmetatable: Set metatable (debug version)
 * - setupvalue: Set upvalue
 * - traceback: Get stack traceback
 *
 * This implementation follows the simplified framework design
 * for better performance and maintainability.
 */
class DebugLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "debug"; }

    /**
     * @brief Register debug library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(LuaState* state) override;

    /**
     * @brief Initialize the debug library
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(LuaState* state) override;

    // Debug function declarations with proper documentation
    /**
     * @brief Enter interactive debug mode
     * @param state Lua state
     * @param nargs Number of arguments (should be 0)
     * @return Nil
     * @throws std::invalid_argument if state is null
     */
    static Value debug(LuaState* state, i32 nargs);

    /**
     * @brief Get function environment
     * @param state Lua state containing function or level
     * @param nargs Number of arguments (should be 1)
     * @return Environment table or nil
     * @throws std::invalid_argument if state is null
     */
    static Value getfenv(LuaState* state, i32 nargs);

    /**
     * @brief Get current hook function
     * @param state Lua state
     * @param nargs Number of arguments (should be 0)
     * @return Hook function, mask, and count
     * @throws std::invalid_argument if state is null
     */
    static Value gethook(LuaState* state, i32 nargs);

    /**
     * @brief Get function information
     * @param state Lua state containing function and optional what string
     * @param nargs Number of arguments (1 or 2)
     * @return Information table
     * @throws std::invalid_argument if state is null
     */
    static Value getinfo(LuaState* state, i32 nargs);

    /**
     * @brief Get local variable
     * @param state Lua state containing level and local index
     * @param nargs Number of arguments (should be 2)
     * @return Variable name and value
     * @throws std::invalid_argument if state is null
     */
    static Value getlocal(LuaState* state, i32 nargs);

    /**
     * @brief Get metatable (debug version)
     * @param state Lua state containing object
     * @param nargs Number of arguments (should be 1)
     * @return Metatable or nil
     * @throws std::invalid_argument if state is null
     */
    static Value getmetatable(LuaState* state, i32 nargs);

    /**
     * @brief Get registry table
     * @param state Lua state
     * @param nargs Number of arguments (should be 0)
     * @return Registry table
     * @throws std::invalid_argument if state is null
     */
    static Value getregistry(LuaState* state, i32 nargs);

    /**
     * @brief Get upvalue
     * @param state Lua state containing function and upvalue index
     * @param nargs Number of arguments (should be 2)
     * @return Upvalue name and value
     * @throws std::invalid_argument if state is null
     */
    static Value getupvalue(LuaState* state, i32 nargs);

    /**
     * @brief Set function environment
     * @param state Lua state containing function/level and environment
     * @param nargs Number of arguments (should be 2)
     * @return Previous environment or nil
     * @throws std::invalid_argument if state is null
     */
    static Value setfenv(LuaState* state, i32 nargs);

    /**
     * @brief Set hook function
     * @param state Lua state containing hook function, mask, and count
     * @param nargs Number of arguments (2 or 3)
     * @return Nil
     * @throws std::invalid_argument if state is null
     */
    static Value sethook(LuaState* state, i32 nargs);

    /**
     * @brief Set local variable
     * @param state Lua state containing level, local index, and value
     * @param nargs Number of arguments (should be 3)
     * @return Variable name or nil
     * @throws std::invalid_argument if state is null
     */
    static Value setlocal(LuaState* state, i32 nargs);

    /**
     * @brief Set metatable (debug version)
     * @param state Lua state containing object and metatable
     * @param nargs Number of arguments (should be 2)
     * @return Object
     * @throws std::invalid_argument if state is null
     */
    static Value setmetatable(LuaState* state, i32 nargs);

    /**
     * @brief Set upvalue
     * @param state Lua state containing function, upvalue index, and value
     * @param nargs Number of arguments (should be 3)
     * @return Upvalue name or nil
     * @throws std::invalid_argument if state is null
     */
    static Value setupvalue(LuaState* state, i32 nargs);

    /**
     * @brief Get stack traceback
     * @param state Lua state containing optional message and level
     * @param nargs Number of arguments (0 to 2)
     * @return Traceback string
     * @throws std::invalid_argument if state is null
     */
    static Value traceback(LuaState* state, i32 nargs);

private:
    /**
     * @brief Validate stack level argument
     * @param state Lua state
     * @param argIndex Argument index
     * @return Stack level or -1 if invalid
     */
    static i32 validateLevel(LuaState* state, i32 argIndex);

    /**
     * @brief Validate function argument
     * @param state Lua state
     * @param argIndex Argument index
     * @return True if valid function
     */
    static bool validateFunction(LuaState* state, i32 argIndex);

    /**
     * @brief Create debug info table
     * @param state Lua state
     * @return Debug info table
     */
    static Value createDebugInfo(LuaState* state);

    /**
     * @brief Get function name from debug info
     * @param state Lua state
     * @param level Stack level
     * @return Function name or "?"
     */
    static Str getFunctionName(LuaState* state, i32 level);

    /**
     * @brief Get source information
     * @param state Lua state
     * @param level Stack level
     * @return Source information string
     */
    static Str getSourceInfo(LuaState* state, i32 level);

    /**
     * @brief Format traceback line
     * @param level Stack level
     * @param functionName Function name
     * @param sourceInfo Source information
     * @return Formatted traceback line
     */
    static Str formatTracebackLine(i32 level, const Str& functionName, const Str& sourceInfo);

    // Interactive debug helper functions
    static void printDebugHelp();
    static void printStackInfo(LuaState* state);
    static void printGlobals(LuaState* state);
    static void evaluateExpression(LuaState* state, const Str& expr);
    static void executeAssignment(LuaState* state, const Str& assignment);
};

/**
 * @brief Convenient DebugLib initialization function
 * @param state Lua state
 */
void initializeDebugLib(LuaState* state);

} // namespace Lua
