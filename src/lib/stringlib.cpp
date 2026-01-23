/**
 * @file stringlib.cpp
 * @brief Lua String Library Implementation
 * 
 * Implements Lua 5.1 string library functions using modern C++.
 * Follows the established pattern from mathlib.cpp and baselib.cpp.
 * 
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#include "lib/stringlib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "vm/global_state.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>

namespace Lua {

// =====================================================================
// Helper Functions
// =====================================================================

/**
 * @brief Get string argument from stack
 * @param L Lua state pointer
 * @param idx Argument index (1-based)
 * @param funcName Function name (for error messages)
 * @return String pointer and length
 */
static inline const char* getStringArg(LuaState* L, i32 idx, const char* funcName, usize* len = nullptr) {
    if (!L->isString(idx)) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.%s' (string expected)", idx, funcName);
        L->error(buffer);
    }
    const char* str = L->toString(idx);
    if (len) {
        *len = std::strlen(str);
    }
    return str;
}

/**
 * @brief Get number argument from stack
 * @param L Lua state pointer
 * @param idx Argument index (1-based)
 * @param funcName Function name (for error messages)
 * @return Number value
 */
static inline f64 getNumberArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isNumber(idx)) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.%s' (number expected)", idx, funcName);
        L->error(buffer);
    }
    return L->toNumber(idx);
}

/**
 * @brief Adjust string position (Lua uses 1-based indexing, supports negative indices)
 * @param pos Position from Lua (1-based, negative from end)
 * @param len String length
 * @return Adjusted 0-based position
 */
static inline usize adjustPosition(i32 pos, usize len) {
    if (pos > 0) {
        return static_cast<usize>(pos - 1);  // Convert 1-based to 0-based
    } else if (pos < 0) {
        // Negative index: count from end
        i32 adjusted = static_cast<i32>(len) + pos + 1;
        return adjusted > 0 ? static_cast<usize>(adjusted - 1) : 0;
    } else {
        // pos == 0 is treated as 1 in Lua
        return 0;
    }
}

// =====================================================================
// Basic String Functions
// =====================================================================

i32 str_len(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.len: missing argument");
    }
    
    usize len;
    getStringArg(L, 1, "len", &len);
    
    L->pushNumber(static_cast<f64>(len));
    return 1;
}

i32 str_sub(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.sub: missing arguments");
    }
    
    usize len;
    const char* s = getStringArg(L, 1, "sub", &len);
    i32 start = static_cast<i32>(getNumberArg(L, 2, "sub"));
    i32 end = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "sub")) : static_cast<i32>(len);
    
    // Adjust positions
    usize startPos = adjustPosition(start, len);
    usize endPos = adjustPosition(end, len);
    
    // Handle edge cases
    if (startPos >= len || endPos < startPos) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }
    
    // Clamp end position
    if (endPos >= len) {
        endPos = len - 1;
    }
    
    // Extract substring
    usize subLen = endPos - startPos + 1;
    Str result(s + startPos, subLen);
    
    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_upper(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.upper: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "upper", &len);

    Str result(s, len);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_lower(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.lower: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "lower", &len);

    Str result(s, len);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_reverse(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.reverse: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "reverse", &len);

    Str result(s, len);
    std::reverse(result.begin(), result.end());

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_rep(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.rep: missing arguments");
    }

    usize len;
    const char* s = getStringArg(L, 1, "rep", &len);
    i32 n = static_cast<i32>(getNumberArg(L, 2, "rep"));

    if (n <= 0) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }

    // Build repeated string
    Str result;
    result.reserve(len * n);
    for (i32 i = 0; i < n; i++) {
        result.append(s, len);
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_byte(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.byte: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "byte", &len);

    i32 start = (L->getTop() >= 2) ? static_cast<i32>(getNumberArg(L, 2, "byte")) : 1;
    i32 end = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "byte")) : start;

    // Adjust positions
    usize startPos = adjustPosition(start, len);
    usize endPos = adjustPosition(end, len);

    // Handle edge cases
    if (startPos >= len || endPos < startPos) {
        return 0;  // Return no values
    }

    // Clamp end position
    if (endPos >= len) {
        endPos = len - 1;
    }

    // Push byte values
    i32 count = 0;
    for (usize i = startPos; i <= endPos; i++) {
        L->pushNumber(static_cast<f64>(static_cast<unsigned char>(s[i])));
        count++;
    }

    return count;
}

i32 str_char(LuaState* L) {
    i32 n = L->getTop();

    Str result;
    result.reserve(n);

    for (i32 i = 1; i <= n; i++) {
        f64 val = getNumberArg(L, i, "char");
        i32 c = static_cast<i32>(val);

        if (c < 0 || c > 255) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.char' (value out of range)", i);
            L->error(buffer);
        }

        result.push_back(static_cast<char>(c));
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

// =====================================================================
// Pattern Matching Functions (Simplified Implementation)
// =====================================================================

/**
 * @brief Simple pattern matching helper (plain text search)
 * @param s Source string
 * @param slen Source length
 * @param pattern Pattern string
 * @param plen Pattern length
 * @param init Starting position (0-based)
 * @return Position of match (0-based) or -1 if not found
 */
static i32 plainFind(const char* s, usize slen, const char* pattern, usize plen, usize init) {
    if (plen == 0) return static_cast<i32>(init);
    if (init + plen > slen) return -1;

    for (usize i = init; i <= slen - plen; i++) {
        if (std::memcmp(s + i, pattern, plen) == 0) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

i32 str_find(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.find: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "find", &slen);
    const char* pattern = getStringArg(L, 2, "find", &plen);

    i32 init = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "find")) : 1;
    bool plain = (L->getTop() >= 4) ? L->toBoolean(4) : false;

    // Adjust init position
    usize initPos = adjustPosition(init, slen);
    if (initPos >= slen) {
        L->pushNil();
        return 1;
    }

    // For now, only implement plain text search
    // Full pattern matching would require a pattern matching engine
    i32 pos = plainFind(s, slen, pattern, plen, initPos);

    if (pos >= 0) {
        L->pushNumber(static_cast<f64>(pos + 1));  // Convert to 1-based
        L->pushNumber(static_cast<f64>(pos + plen));  // End position (1-based)
        return 2;
    } else {
        L->pushNil();
        return 1;
    }
}

i32 str_gsub(LuaState* L) {
    if (L->getTop() < 3) {
        L->error("string.gsub: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "gsub", &slen);
    const char* pattern = getStringArg(L, 2, "gsub", &plen);
    const char* repl = getStringArg(L, 3, "gsub", nullptr);

    i32 maxRepl = (L->getTop() >= 4) ? static_cast<i32>(getNumberArg(L, 4, "gsub")) : -1;

    // Simple implementation: plain text replacement
    Str result;
    result.reserve(slen);

    i32 count = 0;
    usize pos = 0;

    while (pos < slen) {
        if (maxRepl >= 0 && count >= maxRepl) {
            // Reached max replacements, copy rest
            result.append(s + pos, slen - pos);
            break;
        }

        i32 found = plainFind(s, slen, pattern, plen, pos);
        if (found < 0) {
            // No more matches, copy rest
            result.append(s + pos, slen - pos);
            break;
        }

        // Copy text before match
        result.append(s + pos, found - pos);
        // Append replacement
        result.append(repl);
        count++;
        pos = found + plen;
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    L->pushNumber(static_cast<f64>(count));
    return 2;
}

i32 str_match(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.match: missing arguments");
    }

    // Simplified implementation: return the pattern if found
    usize slen, plen;
    const char* s = getStringArg(L, 1, "match", &slen);
    const char* pattern = getStringArg(L, 2, "match", &plen);

    i32 init = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "match")) : 1;
    usize initPos = adjustPosition(init, slen);

    i32 pos = plainFind(s, slen, pattern, plen, initPos);
    if (pos >= 0) {
        Str result(pattern, plen);
        GCString* str = L->getGlobalState().getStringPool().intern(result);
        L->pushString(str);
        return 1;
    } else {
        L->pushNil();
        return 1;
    }
}

i32 str_gmatch(LuaState* L) {
    // TODO: Implement iterator for pattern matches
    // This requires creating a closure that maintains state
    L->error("string.gmatch: not yet implemented");
    return 0;
}

i32 str_format(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.format: missing format string");
    }

    const char* fmt = getStringArg(L, 1, "format", nullptr);

    // Simple implementation: basic printf-style formatting
    Str result;
    result.reserve(256);

    i32 argIdx = 2;
    const char* p = fmt;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '%') {
                result.push_back('%');
                p++;
            } else if (*p == 's') {
                // String argument
                if (argIdx > L->getTop()) {
                    L->error("string.format: not enough arguments");
                }
                const char* arg = getStringArg(L, argIdx++, "format", nullptr);
                result.append(arg);
                p++;
            } else if (*p == 'd' || *p == 'i') {
                // Integer argument
                if (argIdx > L->getTop()) {
                    L->error("string.format: not enough arguments");
                }
                f64 val = getNumberArg(L, argIdx++, "format");
                char buffer[64];
                std::snprintf(buffer, sizeof(buffer), "%d", static_cast<i32>(val));
                result.append(buffer);
                p++;
            } else if (*p == 'f') {
                // Float argument
                if (argIdx > L->getTop()) {
                    L->error("string.format: not enough arguments");
                }
                f64 val = getNumberArg(L, argIdx++, "format");
                char buffer[64];
                std::snprintf(buffer, sizeof(buffer), "%f", val);
                result.append(buffer);
                p++;
            } else {
                // Unsupported format specifier, just copy it
                result.push_back('%');
                result.push_back(*p);
                p++;
            }
        } else {
            result.push_back(*p);
            p++;
        }
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_dump(LuaState* L) {
    // TODO: Implement function bytecode dumping
    // This requires access to the function's bytecode
    L->error("string.dump: not yet implemented");
    return 0;
}

// =====================================================================
// Library Registration
// =====================================================================

void StringLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // Create string table
    Table* stringTable = FunctionRegistrar::createLibTable(L, "string");

    // Register functions using FunctionRegistrar
    FunctionRegistrar(L)
        .addGlobal("len", str_len)
        .addGlobal("sub", str_sub)
        .addGlobal("upper", str_upper)
        .addGlobal("lower", str_lower)
        .addGlobal("reverse", str_reverse)
        .addGlobal("rep", str_rep)
        .addGlobal("byte", str_byte)
        .addGlobal("char", str_char)
        .addGlobal("find", str_find)
        .addGlobal("gsub", str_gsub)
        .addGlobal("match", str_match)
        .addGlobal("gmatch", str_gmatch)
        .addGlobal("format", str_format)
        .addGlobal("dump", str_dump)
        .commitToTable(stringTable);
}

void StringLibModule::initialize(LuaState* L) {
    // No additional initialization needed
    (void)L;
}

void openStringLib(LuaState* L) {
    if (!L) {
        return;
    }

    StringLibModule module;
    module.registerFunctions(L);
    module.initialize(L);
}

} // namespace Lua

