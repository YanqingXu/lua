#include "error_utils.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"

namespace Lua {
namespace Lib {

void ErrorUtils::error(State* state, const std::string& message, int level) {
    (void)level; // Suppress unused parameter warning
    throw std::runtime_error(message);
}

void ErrorUtils::argumentError(State* state, int argIndex, const std::string& message) {
    throw std::runtime_error("bad argument #" + std::to_string(argIndex) + " (" + message + ")");
}

void ErrorUtils::typeError(State* state, int argIndex, const std::string& expectedType) {
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

void ErrorUtils::runtimeError(State* state, const std::string& message) {
    throw std::runtime_error("runtime error: " + message);
}

} // namespace Lib
} // namespace Lua
