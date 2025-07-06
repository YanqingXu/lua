#include "lib_base_utils.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

namespace Lua {
namespace Lib {

// === ArgUtils Implementation ===

void ArgUtils::checkArgCount(State* state, int expected, const char* funcName) {
    int actual = state->getTop(); // Use existing getTop method
    if (actual != expected) {
        throw std::runtime_error(std::string(funcName) + " expects " + std::to_string(expected) + " arguments, got " + std::to_string(actual));
    }
}

void ArgUtils::checkArgCount(State* state, int minArgs, int maxArgs, const char* funcName) {
    int actual = state->getTop(); // Use existing getTop method
    if (actual < minArgs || actual > maxArgs) {
        throw std::runtime_error(std::string(funcName) + " expects " + std::to_string(minArgs) + "-" + std::to_string(maxArgs) + " arguments, got " + std::to_string(actual));
    }
}

Value ArgUtils::checkNumber(State* state, int index, const char* funcName) {
    Value val = state->get(index);
    if (!val.isNumber()) {
        throw std::runtime_error(std::string(funcName) + " argument " + std::to_string(index) + " must be a number");
    }
    return val;
}

Value ArgUtils::checkString(State* state, int index, const char* funcName) {
    Value val = state->get(index);
    if (!val.isString()) {
        throw std::runtime_error(std::string(funcName) + " argument " + std::to_string(index) + " must be a string");
    }
    return val;
}

Value ArgUtils::checkTable(State* state, int index, const char* funcName) {
    Value val = state->get(index);
    if (!val.isTable()) {
        throw std::runtime_error(std::string(funcName) + " argument " + std::to_string(index) + " must be a table");
    }
    return val;
}

// === ErrorUtils Implementation ===

void ErrorUtils::error(State* state, const std::string& message, i32 level) {
    (void)state;
    (void)level; // Suppress unused parameter warning
    throw std::runtime_error(message);
}

void ErrorUtils::argumentError(State* state, i32 argIndex, const std::string& message) {
    (void)state;
    throw std::runtime_error("bad argument #" + std::to_string(argIndex) + " (" + message + ")");
}

void ErrorUtils::typeError(State* state, i32 argIndex, const std::string& expectedType) {
    Value val = state->get(argIndex);
    std::string actualType = "unknown";
    
    if (val.isNil()) actualType = "nil";
    else if (val.isBoolean()) actualType = "boolean";
    else if (val.isNumber()) actualType = "number";
    else if (val.isString()) actualType = "string";
    else if (val.isTable()) actualType = "table";
    else if (val.isFunction()) actualType = "function";
    
    throw std::runtime_error("bad argument #" + std::to_string(argIndex) + " (" + expectedType + " expected, got " + actualType + ")");
}

// === BaseLibUtils Implementation ===

std::string BaseLibUtils::toString(const Value& value) {
    if (value.isNil()) {
        return "nil";
    } else if (value.isBoolean()) {
        return value.asBoolean() ? "true" : "false";
    } else if (value.isNumber()) {
        double num = value.asNumber();
        if (std::floor(num) == num && num >= -1e15 && num <= 1e15) {
            // Integer representation
            return std::to_string(static_cast<long long>(num));
        } else {
            // Floating point representation
            std::ostringstream oss;
            oss << std::setprecision(14) << num;
            return oss.str();
        }
    } else if (value.isString()) {
        return value.asString();
    } else if (value.isTable()) {
        return "table: " + std::to_string(reinterpret_cast<uintptr_t>(value.asTable().get()));
    } else if (value.isFunction()) {
        return "function: " + std::to_string(reinterpret_cast<uintptr_t>(value.asFunction().get()));
    } else {
        return "unknown";
    }
}

std::optional<double> BaseLibUtils::stringToNumber(std::string_view str, int base) {
    if (str.empty()) return std::nullopt;
    
    try {
        if (base == 10) {
            // Try to parse as decimal number
            size_t processed = 0;
            double result = std::stod(std::string(str), &processed);
            
            // Check if the entire string was processed
            if (processed == str.length()) {
                return result;
            }
        } else {
            // Handle other bases (simplified)
            // For now, just support base 16
            if (base == 16) {
                size_t processed = 0;
                long long result = std::stoll(std::string(str), &processed, 16);
                if (processed == str.length()) {
                    return static_cast<double>(result);
                }
            }
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }
    
    return std::nullopt;
}

bool BaseLibUtils::isTruthy(const Value& value) {
    // In Lua, only nil and false are falsy
    return !(value.isNil() || (value.isBoolean() && !value.asBoolean()));
}

bool BaseLibUtils::rawEqual(const Value& a, const Value& b) {
    // Raw equality comparison without metamethods
    if (a.type() != b.type()) {
        return false;
    }
    
    switch (a.type()) {
        case ValueType::Nil:
            return true;
        case ValueType::Boolean:
            return a.asBoolean() == b.asBoolean();
        case ValueType::Number:
            return a.asNumber() == b.asNumber();
        case ValueType::String:
            return a.asString() == b.asString();
        case ValueType::Table:
            return a.asTable() == b.asTable();
        case ValueType::Function:
            return a.asFunction() == b.asFunction();
        default:
            return false;
    }
}

std::string BaseLibUtils::getTypeName(const Value& value) {
    switch (value.type()) {
        case ValueType::Nil: return "nil";
        case ValueType::Boolean: return "boolean";
        case ValueType::Number: return "number";
        case ValueType::String: return "string";
        case ValueType::Table: return "table";
        case ValueType::Function: return "function";
        default: return "unknown";
    }
}

size_t BaseLibUtils::rawLength(const Value& value) {
    if (value.isString()) {
        return value.asString().length();
    } else if (value.isTable()) {
        // For now, return 0 as a stub - real implementation would count array elements
        return 0;
    }
    return 0;
}

} // namespace Lib
} // namespace Lua