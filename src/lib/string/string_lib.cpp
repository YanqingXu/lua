#include "string_lib.hpp"
#include "../core/multi_return_helper.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <regex>
#include <iomanip>

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

    // Register multi-return functions using new mechanism
    LibRegistry::registerTableFunction(state, stringTable, "find", find);
    LibRegistry::registerTableFunction(state, stringTable, "gsub", gsub);

    // Register legacy single-return functions
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "len", len);
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "sub", sub);
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "upper", upper);
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "lower", lower);
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "reverse", reverse);
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "rep", rep);
    LibRegistry::registerTableFunctionLegacy(state, stringTable, "format", format);
}

void StringLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // String library doesn't need special initialization
}

// ===================================================================
// 字符串函数实现
// ===================================================================

Value StringLib::len(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;
    Value strVal = state->get(stackIdx);

    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    return Value(static_cast<double>(str.length()));
}

Value StringLib::sub(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;
    Value strVal = state->get(stackIdx);
    Value startVal = state->get(stackIdx + 1);

    if (!strVal.isString() || !startVal.isNumber()) return Value();

    std::string str = strVal.toString();
    int start = static_cast<int>(startVal.asNumber()) - 1; // Lua索引从1开始
    int end = static_cast<int>(str.length());

    if (nargs >= 3) {
        Value endVal = state->get(stackIdx + 2);
        if (endVal.isNumber()) {
            end = static_cast<int>(endVal.asNumber());
        }
    }

    // 处理负数索引
    if (start < 0) start =  static_cast<int>(str.length() + start + 1);
    if (end < 0) end = static_cast<int>(str.length() + end + 1);

    // 边界检查
    if (start < 0) start = 0;
    if (end > static_cast<int>(str.length())) end = static_cast<int>(str.length());
    if (start >= end) return Value("");

    return Value(str.substr(start, end - start));
}

Value StringLib::upper(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Convert 1-based Lua index to 0-based stack index
    int stackIdx = state->getTop() - nargs;
    Value strVal = state->get(stackIdx);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    return Value(str);
}

Value StringLib::lower(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Convert 1-based Lua index to 0-based stack index
    int stackIdx = state->getTop() - nargs;
    Value strVal = state->get(stackIdx);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    return Value(str);
}

Value StringLib::reverse(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Convert 1-based Lua index to 0-based stack index
    int stackIdx = state->getTop() - nargs;
    Value strVal = state->get(stackIdx);
    if (!strVal.isString()) return Value();

    std::string str = strVal.toString();
    std::reverse(str.begin(), str.end());

    return Value(str);
}

Value StringLib::rep(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    // Convert 1-based Lua index to 0-based stack index
    int stackIdx = state->getTop() - nargs;
    Value strVal = state->get(stackIdx);
    Value countVal = state->get(stackIdx + 1);

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

// New Lua 5.1 standard find implementation (multi-return)
i32 StringLib::find(State* state) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 3) { // Need at least string module + string + pattern
        throw std::invalid_argument("string.find: expected at least 2 arguments (string, pattern)");
    }

    // For string library functions called as string.find(str, pattern):
    // - First argument is the string module itself (skip it)
    // - Second argument is the string to search in
    // - Third argument is the pattern to search for
    Value strVal = state->get(1);      // Skip string module
    Value patternVal = state->get(2);  // Pattern argument

    if (!strVal.isString() || !patternVal.isString()) {
        throw std::invalid_argument("string.find: arguments must be strings");
    }

    Str str = strVal.asString();
    Str pattern = patternVal.asString();

    // Optional start position (default: 1)
    size_t startPos = 1;
    if (nargs >= 4) { // string module + string + pattern + start
        Value startVal = state->get(3); // Fourth argument (start position)
        if (startVal.isNumber()) {
            f64 start = startVal.asNumber();
            if (start >= 1) {
                startPos = static_cast<size_t>(start);
            }
        }
    }

    // Optional plain flag (default: false)
    bool plainSearch = false;
    if (nargs >= 5) { // string module + string + pattern + start + plain
        Value plainVal = state->get(4); // Fifth argument (plain flag)
        if (plainVal.isBoolean()) {
            plainSearch = plainVal.asBoolean();
        }
    }

    // Convert 1-based Lua index to 0-based C++ index
    if (startPos > 0) {
        startPos--;
    }

    // Clear arguments
    state->clearStack();

    // Perform the search
    if (plainSearch || pattern.empty()) {
        // Plain string search
        size_t pos = str.find(pattern, startPos);
        if (pos != Str::npos) {
            // Found - return start and end positions (1-based)
            f64 startResult = static_cast<f64>(pos + 1);
            f64 endResult = static_cast<f64>(pos + pattern.length());
            state->push(Value(startResult));
            state->push(Value(endResult));
            return 2;
        } else {
            // Not found - return nil
            state->push(Value());
            return 1;
        }
    } else {
        // Pattern matching (simplified - fall back to plain search for now)
        // TODO: Implement full Lua pattern matching
        size_t pos = str.find(pattern, startPos);
        if (pos != Str::npos) {
            // Found - return start and end positions (1-based)
            f64 startResult = static_cast<f64>(pos + 1);
            f64 endResult = static_cast<f64>(pos + pattern.length());
            state->push(Value(startResult));
            state->push(Value(endResult));
            return 2;
        } else {
            // Not found - return nil
            state->push(Value());
            return 1;
        }
    }
}





// New Lua 5.1 standard gsub implementation (multi-return)
i32 StringLib::gsub(State* state) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 4) { // Need at least string module + string + pattern + replacement
        throw std::invalid_argument("string.gsub: expected at least 3 arguments (string, pattern, replacement)");
    }

    // For string library functions called as string.gsub(str, pattern, replacement):
    // - First argument is the string module itself (skip it)
    // - Second argument is the string to operate on
    // - Third argument is the pattern
    // - Fourth argument is the replacement
    Value strVal = state->get(1);          // Skip string module
    Value patternVal = state->get(2);      // Pattern argument
    Value replacementVal = state->get(3);  // Replacement argument

    if (!strVal.isString() || !patternVal.isString()) {
        throw std::invalid_argument("string.gsub: string and pattern must be strings");
    }

    Str str = strVal.asString();
    Str pattern = patternVal.asString();
    Str replacement;

    // Handle replacement value (can be string, number, function, or table)
    if (replacementVal.isString()) {
        replacement = replacementVal.asString();
    } else if (replacementVal.isNumber()) {
        replacement = std::to_string(replacementVal.asNumber());
    } else {
        // For now, convert other types to string representation
        // TODO: Implement function and table replacement handling
        replacement = ""; // Default empty replacement
    }

    // Optional limit parameter (default: replace all)
    int limit = -1; // -1 means replace all
    if (nargs >= 5) { // string module + string + pattern + replacement + limit
        Value limitVal = state->get(4); // Fifth argument (limit)
        if (limitVal.isNumber()) {
            limit = static_cast<int>(limitVal.asNumber());
        }
    }

    // Perform global substitution
    Str result = str;
    int count = 0;

    if (!pattern.empty()) {
        size_t pos = 0;
        while ((pos = result.find(pattern, pos)) != Str::npos && (limit == -1 || count < limit)) {
            result.replace(pos, pattern.length(), replacement);
            pos += replacement.length();
            count++;
        }
    }

    // Clear arguments and push results
    state->clearStack();
    state->push(Value(result));                    // New string first
    state->push(Value(static_cast<f64>(count)));   // Replacement count second

    // Return 2 values
    return 2;
}



Value StringLib::format(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    if (nargs < 1) {
        throw std::invalid_argument("string.format: expected at least 1 argument (format string)");
    }

    // Get format string
    int stackBase = state->getTop() - nargs;
    Value formatVal = state->get(stackBase);

    if (!formatVal.isString()) {
        throw std::invalid_argument("string.format: first argument must be a string");
    }

    Str formatStr = formatVal.asString();

    // Get arguments for formatting
    Vec<Value> args;
    for (int i = 1; i < nargs; ++i) {
        args.push_back(state->get(stackBase + i));
    }

    // Perform formatting (simplified implementation)
    try {
        Str result = performStringFormat(formatStr, args);
        return Value(result);
    } catch (const std::exception& e) {
        throw std::invalid_argument("string.format: " + std::string(e.what()));
    }
}

// ===================================================================
// Convenient Initialization Functions
// ===================================================================

void initializeStringLib(State* state) {
    StringLib stringLib;
    stringLib.registerFunctions(state);
    stringLib.initialize(state);
}

// ===================================================================
// Helper Functions for Pattern Matching
// ===================================================================

/**
 * @brief Convert Lua pattern to basic regex (simplified conversion)
 * @param luaPattern The Lua pattern string
 * @return Equivalent regex pattern string
 */
Str StringLib::convertLuaPatternToRegex(const Str& luaPattern) {
    Str regexPattern;
    regexPattern.reserve(luaPattern.length() * 2);

    for (size_t i = 0; i < luaPattern.length(); ++i) {
        char c = luaPattern[i];

        switch (c) {
            case '.':
                regexPattern += ".";  // Any character (same in regex)
                break;
            case '*':
                regexPattern += ".*"; // Zero or more of any character
                break;
            case '+':
                regexPattern += ".+"; // One or more of any character
                break;
            case '?':
                regexPattern += ".?"; // Zero or one of any character
                break;
            case '^':
                if (i == 0) {
                    regexPattern += "^"; // Start of string
                } else {
                    regexPattern += "\\^"; // Literal ^
                }
                break;
            case '$':
                if (i == luaPattern.length() - 1) {
                    regexPattern += "$"; // End of string
                } else {
                    regexPattern += "\\$"; // Literal $
                }
                break;
            case '%':
                // Lua escape character - handle next character specially
                if (i + 1 < luaPattern.length()) {
                    char next = luaPattern[i + 1];
                    switch (next) {
                        case 'a': regexPattern += "[a-zA-Z]"; break;
                        case 'd': regexPattern += "[0-9]"; break;
                        case 'l': regexPattern += "[a-z]"; break;
                        case 'u': regexPattern += "[A-Z]"; break;
                        case 'w': regexPattern += "[a-zA-Z0-9]"; break;
                        case 's': regexPattern += "\\s"; break;
                        default:
                            // Escape the character
                            regexPattern += "\\";
                            regexPattern += next;
                            break;
                    }
                    i++; // Skip the next character
                } else {
                    regexPattern += "\\%"; // Literal %
                }
                break;
            case '[':
            case ']':
            case '(':
            case ')':
            case '{':
            case '}':
            case '\\':
            case '|':
                // Escape regex special characters
                regexPattern += "\\";
                regexPattern += c;
                break;
            default:
                regexPattern += c;
                break;
        }
    }

    return regexPattern;
}

/**
 * @brief Perform string formatting (simplified implementation)
 * @param formatStr The format string
 * @param args The arguments to format
 * @return Formatted string
 */
Str StringLib::performStringFormat(const Str& formatStr, const Vec<Value>& args) {
    std::ostringstream result;
    size_t argIndex = 0;

    for (size_t i = 0; i < formatStr.length(); ++i) {
        if (formatStr[i] == '%' && i + 1 < formatStr.length()) {
            char specifier = formatStr[i + 1];

            if (specifier == '%') {
                // Literal %
                result << '%';
                i++; // Skip the second %
            } else if (argIndex < args.size()) {
                // Format specifier
                const Value& arg = args[argIndex++];

                switch (specifier) {
                    case 'd':
                    case 'i':
                        // Integer
                        if (arg.isNumber()) {
                            result << static_cast<i64>(arg.asNumber());
                        } else {
                            result << "0";
                        }
                        break;
                    case 'f':
                        // Float
                        if (arg.isNumber()) {
                            result << std::fixed << arg.asNumber();
                        } else {
                            result << "0.000000";
                        }
                        break;
                    case 'g':
                        // General number format
                        if (arg.isNumber()) {
                            result << arg.asNumber();
                        } else {
                            result << "0";
                        }
                        break;
                    case 's':
                        // String
                        result << arg.toString();
                        break;
                    case 'c':
                        // Character
                        if (arg.isNumber()) {
                            char c = static_cast<char>(arg.asNumber());
                            result << c;
                        } else {
                            result << '\0';
                        }
                        break;
                    case 'x':
                        // Hexadecimal (lowercase)
                        if (arg.isNumber()) {
                            result << std::hex << std::nouppercase << static_cast<i64>(arg.asNumber());
                        } else {
                            result << "0";
                        }
                        break;
                    case 'X':
                        // Hexadecimal (uppercase)
                        if (arg.isNumber()) {
                            result << std::hex << std::uppercase << static_cast<i64>(arg.asNumber());
                        } else {
                            result << "0";
                        }
                        break;
                    case 'o':
                        // Octal
                        if (arg.isNumber()) {
                            result << std::oct << static_cast<i64>(arg.asNumber());
                        } else {
                            result << "0";
                        }
                        break;
                    default:
                        // Unknown specifier - output as is
                        result << '%' << specifier;
                        break;
                }
                i++; // Skip the specifier
            } else {
                // No more arguments - output as is
                result << formatStr[i];
            }
        } else {
            // Regular character
            result << formatStr[i];
        }
    }

    return result.str();
}

// For backward compatibility, provide old function names
namespace Lua {
    std::unique_ptr<LibModule> createStringLib() {
        return std::make_unique<StringLib>();
    }
}

} // namespace Lua
