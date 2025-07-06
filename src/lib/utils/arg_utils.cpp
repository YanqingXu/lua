#include "arg_utils.hpp"
#include "error_handling.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"

namespace Lua {
namespace Lib {

void ArgUtils::checkArgCount(State* state, int actual, const char* funcName) {
    // Stub implementation - in real version this would check minimum args
    (void)state;
    (void)actual;
    (void)funcName;
    // TODO: Implement argument count validation
}

void ArgUtils::checkArgCount(State* state, int actual, int expected, const char* funcName) {
    if (actual != expected) {
        throw std::runtime_error(std::string(funcName) + " expects " + std::to_string(expected) + " arguments, got " + std::to_string(actual));
    }
}

void ArgUtils::checkArgCount(State* state, int actual, int min, int max, const char* funcName) {
    if (actual < min || actual > max) {
        throw std::runtime_error(std::string(funcName) + " expects " + std::to_string(min) + "-" + std::to_string(max) + " arguments, got " + std::to_string(actual));
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

} // namespace Lib
} // namespace Lua
