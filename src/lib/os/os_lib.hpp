#pragma once

#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include "vm/value.hpp"
#include "lib/core/lib_module.hpp"
#include <ctime>

namespace Lua {

/**
 * @brief OS library implementation
 *
 * Provides Lua operating system interface functions:
 * - clock: Get CPU time
 * - date: Format date/time
 * - difftime: Calculate time difference
 * - execute: Execute system command
 * - exit: Exit program
 * - getenv: Get environment variable
 * - remove: Remove file
 * - rename: Rename file
 * - setlocale: Set locale
 * - time: Get current time
 * - tmpname: Generate temporary filename
 *
 * This implementation follows the simplified framework design
 * for better performance and maintainability.
 */
class OSLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "os"; }

    /**
     * @brief Register OS library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(LuaState* state) override;

    /**
     * @brief Initialize the OS library
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(LuaState* state) override;

    // OS function declarations with proper documentation
    /**
     * @brief Get CPU time
     * @param state Lua state
     * @param nargs Number of arguments (should be 0)
     * @return Number representing CPU time in seconds
     * @throws std::invalid_argument if state is null
     */
    static Value clock(LuaState* state, i32 nargs);

    /**
     * @brief Format date/time
     * @param state Lua state containing format string and optional time
     * @param nargs Number of arguments (0 to 2)
     * @return Formatted date string
     * @throws std::invalid_argument if state is null
     */
    static Value date(LuaState* state, i32 nargs);

    /**
     * @brief Calculate time difference
     * @param state Lua state containing two time values
     * @param nargs Number of arguments (should be 2)
     * @return Number representing time difference in seconds
     * @throws std::invalid_argument if state is null
     */
    static Value difftime(LuaState* state, i32 nargs);

    /**
     * @brief Execute system command
     * @param state Lua state containing command string
     * @param nargs Number of arguments (0 or 1)
     * @return Exit code or nil on error
     * @throws std::invalid_argument if state is null
     */
    static Value execute(LuaState* state, i32 nargs);

    /**
     * @brief Exit program
     * @param state Lua state containing optional exit code
     * @param nargs Number of arguments (0 or 1)
     * @return Does not return (exits program)
     * @throws std::invalid_argument if state is null
     */
    static Value exit(LuaState* state, i32 nargs);

    /**
     * @brief Get environment variable
     * @param state Lua state containing variable name
     * @param nargs Number of arguments (should be 1)
     * @return Environment variable value or nil
     * @throws std::invalid_argument if state is null
     */
    static Value getenv(LuaState* state, i32 nargs);

    /**
     * @brief Remove file
     * @param state Lua state containing filename
     * @param nargs Number of arguments (should be 1)
     * @return True on success, nil and error message on failure
     * @throws std::invalid_argument if state is null
     */
    static Value remove(LuaState* state, i32 nargs);

    /**
     * @brief Rename file
     * @param state Lua state containing old and new filenames
     * @param nargs Number of arguments (should be 2)
     * @return True on success, nil and error message on failure
     * @throws std::invalid_argument if state is null
     */
    static Value rename(LuaState* state, i32 nargs);

    /**
     * @brief Set locale
     * @param state Lua state containing locale string and optional category
     * @param nargs Number of arguments (1 or 2)
     * @return Previous locale or nil on error
     * @throws std::invalid_argument if state is null
     */
    static Value setlocale(LuaState* state, i32 nargs);

    /**
     * @brief Get current time
     * @param state Lua state containing optional time table
     * @param nargs Number of arguments (0 or 1)
     * @return Time value as number or table
     * @throws std::invalid_argument if state is null
     */
    static Value time(LuaState* state, i32 nargs);

    /**
     * @brief Generate temporary filename
     * @param state Lua state
     * @param nargs Number of arguments (should be 0)
     * @return Temporary filename string
     * @throws std::invalid_argument if state is null
     */
    static Value tmpname(LuaState* state, i32 nargs);

private:
    /**
     * @brief Convert time_t to Lua time table
     * @param state Lua state
     * @param t Time value
     * @return Table containing time components
     */
    static Value timeToTable(LuaState* state, std::time_t t);

    /**
     * @brief Convert Lua time table to time_t
     * @param state Lua state
     * @param tableVal Table containing time components
     * @return time_t value or -1 on error
     */
    static std::time_t tableToTime(LuaState* state, const Value& tableVal);

    /**
     * @brief Format time using strftime
     * @param format Format string
     * @param t Time value
     * @return Formatted time string
     */
    static Str formatTime(const Str& format, std::time_t t);

    /**
     * @brief Get default date format
     * @return Default format string
     */
    static const char* getDefaultDateFormat();

    /**
     * @brief Validate filename argument
     * @param state Lua state
     * @param argIndex Argument index
     * @return Filename string or empty if invalid
     */
    static Str validateFilename(LuaState* state, i32 argIndex);

    /**
     * @brief Get system error message
     * @param errorCode Error code from errno
     * @return Error message string
     */
    static Str getSystemError(i32 errorCode);
};

/**
 * @brief Convenient OSLib initialization function
 * @param state Lua state
 */
void initializeOSLib(LuaState* state);

} // namespace Lua
