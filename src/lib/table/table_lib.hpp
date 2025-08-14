#pragma once

#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include "vm/value.hpp"
#include "vm/table.hpp"
#include "lib/core/lib_module.hpp"

namespace Lua {

/**
 * @brief Table library implementation
 *
 * Provides Lua table manipulation functions:
 * - insert: Insert element into table
 * - remove: Remove element from table
 * - sort: Sort table elements
 * - concat: Concatenate table elements
 * - getn: Get table length (deprecated in Lua 5.1)
 * - maxn: Get maximum numeric index
 *
 * This implementation follows the simplified framework design
 * for better performance and maintainability.
 */
class TableLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "table"; }

    /**
     * @brief Register table library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(LuaState* state) override;

    /**
     * @brief Initialize the table library
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(LuaState* state) override;

    // Table manipulation function declarations with proper documentation
    /**
     * @brief Insert element into table
     * @param state Lua state containing table, position, and value
     * @param nargs Number of arguments (2 or 3)
     * @return Nil value
     * @throws std::invalid_argument if state is null
     */
    static Value insert(LuaState* state, i32 nargs);

    /**
     * @brief Remove element from table
     * @param state Lua state containing table and optional position
     * @param nargs Number of arguments (1 or 2)
     * @return Removed value or nil
     * @throws std::invalid_argument if state is null
     */
    static Value remove(LuaState* state, i32 nargs);

    /**
     * @brief Sort table elements
     * @param state Lua state containing table and optional comparison function
     * @param nargs Number of arguments (1 or 2)
     * @return Nil value
     * @throws std::invalid_argument if state is null
     */
    static Value sort(LuaState* state, i32 nargs);

    /**
     * @brief Concatenate table elements
     * @param state Lua state containing table, separator, start, and end indices
     * @param nargs Number of arguments (1 to 4)
     * @return String value with concatenated elements
     * @throws std::invalid_argument if state is null
     */
    static Value concat(LuaState* state, i32 nargs);

    /**
     * @brief Get table length (deprecated in Lua 5.1)
     * @param state Lua state containing table
     * @param nargs Number of arguments (should be 1)
     * @return Number value representing table length
     * @throws std::invalid_argument if state is null
     */
    static Value getn(LuaState* state, i32 nargs);

    /**
     * @brief Get maximum numeric index
     * @param state Lua state containing table
     * @param nargs Number of arguments (should be 1)
     * @return Number value representing maximum index
     * @throws std::invalid_argument if state is null
     */
    static Value maxn(LuaState* state, i32 nargs);

private:
    /**
     * @brief Helper function to get table length
     * @param table Table to measure
     * @return Length of the table
     */
    static i32 getTableLength(GCRef<Table> table);

    /**
     * @brief Helper function to validate table argument
     * @param state Lua state
     * @param argIndex Argument index (1-based)
     * @return GCRef to table or null GCRef if invalid
     */
    static GCRef<Table> validateTableArg(LuaState* state, i32 argIndex);
};

/**
 * @brief Convenient TableLib initialization function
 * @param state Lua state
 */
void initializeTableLib(LuaState* state);

} // namespace Lua
