#include "string_lib.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace Lua {

// ===================================================================
// StringLib Implementation
// ===================================================================

void StringLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create string library table
    Value stringTable = LibRegistry::createLibTable(state, "string");

    // Register string functions to string table
    REGISTER_TABLE_FUNCTION(state, stringTable, len, len);
    REGISTER_TABLE_FUNCTION(state, stringTable, sub, sub);
    REGISTER_TABLE_FUNCTION(state, stringTable, upper, upper);
    REGISTER_TABLE_FUNCTION(state, stringTable, lower, lower);
    REGISTER_TABLE_FUNCTION(state, stringTable, reverse, reverse);
    REGISTER_TABLE_FUNCTION(state, stringTable, rep, rep);

    // Pattern matching functions (simplified versions)
    REGISTER_TABLE_FUNCTION(state, stringTable, find, find);
    REGISTER_TABLE_FUNCTION(state, stringTable, match, match);
    REGISTER_TABLE_FUNCTION(state, stringTable, gsub, gsub);

    // Formatting functions
    REGISTER_TABLE_FUNCTION(state, stringTable, format, format);
}

void StringLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // String library doesn't need special initialization
    std::cout << "[StringLib] Initialized successfully!" << std::endl;
}

// ===================================================================
// 字符串函数实现
// ===================================================================

Value StringLib::len(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value strVal = state->get(1);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    return Value(static_cast<double>(str.length()));
}

Value StringLib::sub(State* state, i32 nargs) {
    if (nargs < 2) return Value();

    Value strVal = state->get(1);
    Value startVal = state->get(2);

    if (!strVal.isString() || !startVal.isNumber()) return Value();

    std::string str = strVal.toString();
    int start = static_cast<int>(startVal.asNumber()) - 1; // Lua索引从1开始
    int end = str.length();

    if (nargs >= 3) {
        Value endVal = state->get(3);
        if (endVal.isNumber()) {
            end = static_cast<int>(endVal.asNumber());
        }
    }

    // 处理负数索引
    if (start < 0) start = str.length() + start + 1;
    if (end < 0) end = str.length() + end + 1;

    // 边界检查
    if (start < 0) start = 0;
    if (end > static_cast<int>(str.length())) end = str.length();
    if (start >= end) return Value("");

    return Value(str.substr(start, end - start));
}

Value StringLib::upper(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value strVal = state->get(1);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    return Value(str);
}

Value StringLib::lower(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value strVal = state->get(1);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    return Value(str);
}

Value StringLib::reverse(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value strVal = state->get(1);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    std::reverse(str.begin(), str.end());

    return Value(str);
}

Value StringLib::rep(State* state, i32 nargs) {
    if (nargs < 2) return Value();

    Value strVal = state->get(1);
    Value countVal = state->get(2);

    if (!strVal.isString() || !countVal.isNumber()) return Value();

    std::string str = strVal.toString();
    int count = static_cast<int>(countVal.asNumber());

    if (count <= 0) return Value("");

    std::string result;
    result.reserve(str.length() * count);

    for (int i = 0; i < count; ++i) {
        result += str;
    }

    return Value(result);
}

// ===================================================================
// Pattern Matching Function Implementations (Simplified Versions)
// ===================================================================

Value StringLib::find(State* state, i32 nargs) {
    (void)state; (void)nargs; // Not yet implemented
    std::cout << "string.find() function not yet implemented" << std::endl;
    return Value();
}

Value StringLib::match(State* state, i32 nargs) {
    (void)state; (void)nargs; // Not yet implemented
    std::cout << "string.match() function not yet implemented" << std::endl;
    return Value();
}

Value StringLib::gsub(State* state, i32 nargs) {
    (void)state; (void)nargs; // Not yet implemented
    std::cout << "string.gsub() function not yet implemented" << std::endl;
    return Value();
}

Value StringLib::format(State* state, i32 nargs) {
    (void)state; (void)nargs; // Not yet implemented
    std::cout << "string.format() function not yet implemented" << std::endl;
    return Value();
}

// ===================================================================
// Convenient Initialization Functions
// ===================================================================

void initializeStringLib(State* state) {
    StringLib stringLib;
    stringLib.registerFunctions(state);
    stringLib.initialize(state);
}

// For backward compatibility, provide old function names
namespace Lua {
    std::unique_ptr<LibModule> createStringLib() {
        return std::make_unique<StringLib>();
    }
}

} // namespace Lua
