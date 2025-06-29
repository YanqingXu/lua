#include "string_lib.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace Lua {

    StrView StringLib::getName() const noexcept {
        return "string";
    }

    void StringLib::registerFunctions(FunctionRegistry& registry, const LibraryContext& context) {
        // Basic string functions - simplified versions
        LUA_REGISTER_FUNCTION(registry, len, len);
        LUA_REGISTER_FUNCTION(registry, sub, sub);
        LUA_REGISTER_FUNCTION(registry, upper, upper);
        LUA_REGISTER_FUNCTION(registry, lower, lower);
        LUA_REGISTER_FUNCTION(registry, reverse, reverse);
        
        // Pattern matching functions - simplified versions
        LUA_REGISTER_FUNCTION(registry, find, find);
        LUA_REGISTER_FUNCTION(registry, match, match);
        LUA_REGISTER_FUNCTION(registry, gmatch, gmatch);
        LUA_REGISTER_FUNCTION(registry, gsub, gsub);
        
        // Formatting functions - simplified versions
        LUA_REGISTER_FUNCTION(registry, format, format);
        LUA_REGISTER_FUNCTION(registry, rep, rep);
        
        // Character functions - simplified versions
        LUA_REGISTER_FUNCTION(registry, byte, byte_func);
        LUA_REGISTER_FUNCTION(registry, char, char_func);
        
        // Utility functions - simplified versions
        LUA_REGISTER_FUNCTION(registry, trim, trim);
        LUA_REGISTER_FUNCTION(registry, split, split);
        LUA_REGISTER_FUNCTION(registry, join, join);
        LUA_REGISTER_FUNCTION(registry, startswith, startswith);
        LUA_REGISTER_FUNCTION(registry, endswith, endswith);
        LUA_REGISTER_FUNCTION(registry, contains, contains);
    }

    // Simplified function implementations
    Value StringLib::len(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value strVal = state->get(1);
        if (!strVal.isString()) return Value();
        return Value(static_cast<f64>(strVal.asString().length()));
    }

    Value StringLib::sub(State* state, i32 nargs) {
        if (nargs < 2) return Value();
        Value strVal = state->get(1);
        Value startVal = state->get(2);
        if (!strVal.isString() || !startVal.isNumber()) return Value();
        
        Str str = strVal.asString();
        i32 start = static_cast<i32>(startVal.asNumber());
        i32 end = static_cast<i32>(str.length());
        
        if (nargs >= 3) {
            Value endVal = state->get(3);
            if (endVal.isNumber()) {
                end = static_cast<i32>(endVal.asNumber());
            }
        }
        
        // Lua uses 1-based indexing
        start = std::max(1, start) - 1;
        end = std::min(static_cast<i32>(str.length()), end);
        
        if (start >= end || start < 0) return Value(Str(""));
        
        return Value(str.substr(start, end - start));
    }

    Value StringLib::upper(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value strVal = state->get(1);
        if (!strVal.isString()) return Value();
        
        Str str = strVal.asString();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return Value(str);
    }

    Value StringLib::lower(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value strVal = state->get(1);
        if (!strVal.isString()) return Value();
        
        Str str = strVal.asString();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return Value(str);
    }

    Value StringLib::reverse(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value strVal = state->get(1);
        if (!strVal.isString()) return Value();
        
        Str str = strVal.asString();
        std::reverse(str.begin(), str.end());
        return Value(str);
    }

    Value StringLib::find(State* state, i32 nargs) {
        // Simplified implementation - just return nil for now
        // TODO: Implement proper pattern matching
        return Value();
    }

    Value StringLib::match(State* state, i32 nargs) {
        // Simplified implementation - just return nil for now
        // TODO: Implement proper pattern matching
        return Value();
    }

    Value StringLib::gmatch(State* state, i32 nargs) {
        // Simplified implementation - just return nil for now
        // TODO: Implement proper pattern matching iterator
        return Value();
    }

    Value StringLib::gsub(State* state, i32 nargs) {
        // Simplified implementation - just return original string for now
        // TODO: Implement proper pattern substitution
        if (nargs < 1) return Value();
        return state->get(1);
    }

    Value StringLib::format(State* state, i32 nargs) {
        // Simplified implementation - just return format string for now
        // TODO: Implement proper string formatting
        if (nargs < 1) return Value();
        return state->get(1);
    }

    Value StringLib::rep(State* state, i32 nargs) {
        if (nargs < 2) return Value();
        Value strVal = state->get(1);
        Value countVal = state->get(2);
        if (!strVal.isString() || !countVal.isNumber()) return Value();
        
        Str str = strVal.asString();
        i32 count = static_cast<i32>(countVal.asNumber());
        
        if (count <= 0) return Value(Str(""));
        
        std::ostringstream result;
        for (i32 i = 0; i < count; ++i) {
            result << str;
        }
        
        return Value(result.str());
    }

    Value StringLib::byte_func(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value strVal = state->get(1);
        if (!strVal.isString()) return Value();
        
        Str str = strVal.asString();
        if (str.empty()) return Value();
        
        return Value(static_cast<f64>(static_cast<unsigned char>(str[0])));
    }

    Value StringLib::char_func(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value codeVal = state->get(1);
        if (!codeVal.isNumber()) return Value();
        
        i32 code = static_cast<i32>(codeVal.asNumber());
        if (code < 0 || code > 255) return Value();
        
        char ch = static_cast<char>(code);
        return Value(Str(1, ch));
    }

    Value StringLib::trim(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        Value strVal = state->get(1);
        if (!strVal.isString()) return Value();
        
        Str str = strVal.asString();
        
        // Trim leading whitespace
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == Str::npos) return Value(Str(""));
        
        // Trim trailing whitespace
        size_t end = str.find_last_not_of(" \t\n\r\f\v");
        
        return Value(str.substr(start, end - start + 1));
    }

    Value StringLib::split(State* state, i32 nargs) {
        // Simplified implementation - just return original string for now
        // TODO: Implement proper string splitting that returns a table
        if (nargs < 1) return Value();
        return state->get(1);
    }

    Value StringLib::join(State* state, i32 nargs) {
        // Simplified implementation - just return empty string for now
        // TODO: Implement proper table joining
        return Value(Str(""));
    }

    Value StringLib::startswith(State* state, i32 nargs) {
        if (nargs < 2) return Value(false);
        Value strVal = state->get(1);
        Value prefixVal = state->get(2);
        if (!strVal.isString() || !prefixVal.isString()) return Value(false);
        
        Str str = strVal.asString();
        Str prefix = prefixVal.asString();
        
        return Value(str.substr(0, prefix.length()) == prefix);
    }

    Value StringLib::endswith(State* state, i32 nargs) {
        if (nargs < 2) return Value(false);
        Value strVal = state->get(1);
        Value suffixVal = state->get(2);
        if (!strVal.isString() || !suffixVal.isString()) return Value(false);
        
        Str str = strVal.asString();
        Str suffix = suffixVal.asString();
        
        if (suffix.length() > str.length()) return Value(false);
        
        return Value(str.substr(str.length() - suffix.length()) == suffix);
    }

    Value StringLib::contains(State* state, i32 nargs) {
        if (nargs < 2) return Value(false);
        Value strVal = state->get(1);
        Value substrVal = state->get(2);
        if (!strVal.isString() || !substrVal.isString()) return Value(false);
        
        Str str = strVal.asString();
        Str substr = substrVal.asString();
        
        return Value(str.find(substr) != Str::npos);
    }

} // namespace Lua
