/**
 * @file stringlib.hpp
 * @brief Lua String Library: String manipulation functions
 * 
 * Detailed Description:
 * This module implements Lua's string library, providing comprehensive string
 * manipulation capabilities including:
 * - Basic operations: len, sub, reverse, rep
 * - Case conversion: upper, lower
 * - Byte operations: byte, char
 * - Pattern matching: find, gsub, match, gmatch
 * - Formatting: format
 * - Advanced: dump
 * 
 * Reference Implementation:
 * - lua_c_analysis/src/lstrlib.c for C implementation
 * - Lua 5.1 Reference Manual for API specifications
 * 
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/lua_state.hpp"

namespace Lua {

class StringLibModule : public LibModule {
public:
    const char* getName() const override { return "string"; }
    
    void registerFunctions(LuaState* L) override;
    
    void initialize(LuaState* L) override;
};

/**
 * @brief Register string library to global environment
 * @param L Lua state pointer
 * 
 * Registers all string library functions in a global 'string' table.
 */
void openStringLib(LuaState* L);

// =====================================================================
// String Library Function Declarations
// =====================================================================

/**
 * @brief string.len(s) - Get string length
 * @param L Lua state pointer
 * @return Number of return values (1: length)
 */
i32 str_len(LuaState* L);

/**
 * @brief string.sub(s, i [, j]) - Extract substring
 * @param L Lua state pointer
 * @return Number of return values (1: substring)
 */
i32 str_sub(LuaState* L);

/**
 * @brief string.upper(s) - Convert to uppercase
 * @param L Lua state pointer
 * @return Number of return values (1: uppercase string)
 */
i32 str_upper(LuaState* L);

/**
 * @brief string.lower(s) - Convert to lowercase
 * @param L Lua state pointer
 * @return Number of return values (1: lowercase string)
 */
i32 str_lower(LuaState* L);

/**
 * @brief string.reverse(s) - Reverse string
 * @param L Lua state pointer
 * @return Number of return values (1: reversed string)
 */
i32 str_reverse(LuaState* L);

/**
 * @brief string.rep(s, n) - Repeat string n times
 * @param L Lua state pointer
 * @return Number of return values (1: repeated string)
 */
i32 str_rep(LuaState* L);

/**
 * @brief string.byte(s [, i [, j]]) - Get byte values
 * @param L Lua state pointer
 * @return Number of return values (variable: byte values)
 */
i32 str_byte(LuaState* L);

/**
 * @brief string.char(...) - Create string from byte values
 * @param L Lua state pointer
 * @return Number of return values (1: string)
 */
i32 str_char(LuaState* L);

/**
 * @brief string.find(s, pattern [, init [, plain]]) - Find pattern in string
 * @param L Lua state pointer
 * @return Number of return values (2-3: start, end, captures)
 */
i32 str_find(LuaState* L);

/**
 * @brief string.gsub(s, pattern, repl [, n]) - Global substitution
 * @param L Lua state pointer
 * @return Number of return values (2: result string, count)
 */
i32 str_gsub(LuaState* L);

/**
 * @brief string.match(s, pattern [, init]) - Match pattern
 * @param L Lua state pointer
 * @return Number of return values (variable: captures or whole match)
 */
i32 str_match(LuaState* L);

/**
 * @brief string.gmatch(s, pattern) - Iterator for pattern matches
 * @param L Lua state pointer
 * @return Number of return values (1: iterator function)
 */
i32 str_gmatch(LuaState* L);

/**
 * @brief string.format(formatstring, ...) - Formatted string
 * @param L Lua state pointer
 * @return Number of return values (1: formatted string)
 */
i32 str_format(LuaState* L);

/**
 * @brief string.dump(function) - Dump function bytecode
 * @param L Lua state pointer
 * @return Number of return values (1: bytecode string)
 */
i32 str_dump(LuaState* L);

} // namespace Lua

