#include "lib_framework.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace Lua {

    // FunctionRegistry implementation
    void FunctionRegistry::registerFunction(const FunctionMetadata& meta, LibFunction func) {
        Str name(meta.name);
        functions_[name] = std::move(func);
        metadata_[name] = meta;
    }

    void FunctionRegistry::registerFunction(StrView name, LibFunction func) {
        FunctionMetadata meta(name);
        registerFunction(meta, std::move(func));
    }

    Value FunctionRegistry::callFunction(StrView name, State* state, i32 nargs) const {
        auto it = functions_.find(Str(name));
        if (it != functions_.end()) {
            try {
                return it->second(state, nargs);
            } catch (const std::exception& e) {
                // Return nil value for error (since createError doesn't exist)
                // In a real implementation, this would use Lua's error mechanism
				std::cerr << "Error calling function '" << name << "': " << e.what() << std::endl;
                return Value();
            }
        }
        // Return nil value for function not found
        return Value();
    }

    bool FunctionRegistry::hasFunction(StrView name) const noexcept {
        return functions_.find(Str(name)) != functions_.end();
    }

    const FunctionMetadata* FunctionRegistry::getFunctionMetadata(StrView name) const {
        auto it = metadata_.find(Str(name));
        return (it != metadata_.end()) ? &it->second : nullptr;
    }

    std::vector<Str> FunctionRegistry::getFunctionNames() const {
        std::vector<Str> names;
        names.reserve(functions_.size());
        for (const auto& [name, _] : functions_) {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    void FunctionRegistry::clear() {
        functions_.clear();
        metadata_.clear();
    }

    size_t FunctionRegistry::size() const noexcept {
        return functions_.size();
    }

    // ArgUtils implementation
    namespace ArgUtils {
        void checkArgCount(State* state, i32 expected, StrView funcName) {
            i32 actual = state->getTop();
            if (actual != expected) {
                std::ostringstream oss;
                oss << funcName << ": expected " << expected << " arguments, got " << actual;
                ErrorUtils::error(state, oss.str());
            }
        }

        void checkArgCount(State* state, i32 min, i32 max, StrView funcName) {
            i32 actual = state->getTop();
            if (actual < min || (max >= 0 && actual > max)) {
                std::ostringstream oss;
                oss << funcName << ": expected ";
                if (max < 0) {
                    oss << "at least " << min;
                } else if (min == max) {
                    oss << min;
                } else {
                    oss << min << "-" << max;
                }
                oss << " arguments, got " << actual;
                ErrorUtils::error(state, oss.str());
            }
        }

        Value checkNumber(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                ErrorUtils::error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isNumber()) {
                typeError(state, index, "number", funcName);
            }
            return val;
        }

        Value checkString(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                ErrorUtils::error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isString()) {
                typeError(state, index, "string", funcName);
            }
            return val;
        }

        Value checkTable(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                ErrorUtils::error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isTable()) {
                typeError(state, index, "table", funcName);
            }
            return val;
        }

        Value checkFunction(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                ErrorUtils::error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isFunction()) {
                typeError(state, index, "function", funcName);
            }
            return val;
        }

        std::optional<Value> optNumber(State* state, i32 index, f64 defaultValue) {
            if (index > state->getTop()) {
                return Value(defaultValue);
            }
            
            Value val = state->get(index);
            if (val.isNil()) {
                return Value(defaultValue);
            }
            
            if (!val.isNumber()) {
                return std::nullopt; // Type error
            }
            
            return val;
        }

        std::optional<Value> optString(State* state, i32 index, StrView defaultValue) {
            if (index > state->getTop()) {
                return Value(Str(defaultValue));
            }
            
            Value val = state->get(index);
            if (val.isNil()) {
                return Value(Str(defaultValue));
            }
            
            if (!val.isString()) {
                return std::nullopt; // Type error
            }
            
            return val;
        }

        StrView getTypeName(const Value& value) {
            if (value.isNil()) return "nil";
            if (value.isBoolean()) return "boolean";
            if (value.isNumber()) return "number";
            if (value.isString()) return "string";
            if (value.isTable()) return "table";
            if (value.isFunction()) return "function";
            return "unknown";
        }

        void typeError(State* state, i32 index, StrView expected, StrView funcName) {
            Value val = state->get(index);
            std::ostringstream oss;
            oss << funcName << ": argument " << index << " expected " << expected 
                << ", got " << getTypeName(val);
            ErrorUtils::error(state, oss.str());
        }
    }

    // ErrorUtils implementation
    namespace ErrorUtils {
        [[noreturn]] void error(State* state, StrView message) {
            // In a real implementation, this would use Lua's error mechanism
            throw std::runtime_error(Str(message));
        }

        [[noreturn]] void argError(State* state, i32 index, StrView message) {
            std::ostringstream oss;
            oss << "bad argument #" << index << " (" << message << ")";
            error(state, oss.str());
        }

        [[noreturn]] void typeError(State* state, i32 index, StrView expected) {
            Value val = state->get(index);
            std::ostringstream oss;
            oss << "bad argument #" << index << " (" << expected 
                << " expected, got " << ArgUtils::getTypeName(val) << ")";
            error(state, oss.str());
        }
    }

} // namespace Lua

// Additional implementations for BaseLibUtils
namespace Lua::BaseLibUtils {
    Str toString(const Value& value) {
        if (value.isNil()) return "nil";
        if (value.isBoolean()) return value.asBoolean() ? "true" : "false";
        if (value.isNumber()) {
            std::ostringstream oss;
            oss << value.asNumber();
            return oss.str();
        }
        if (value.isString()) return value.asString();
        if (value.isTable()) return "table";
        if (value.isFunction()) return "function";
        return "unknown";
    }

    std::optional<f64> toNumber(StrView str) {
        try {
            size_t pos;
            f64 result = std::stod(Str(str), &pos);
            // Check if entire string was consumed
            if (pos == str.length()) {
                return result;
            }
        } catch (...) {
            // Conversion failed
        }
        return std::nullopt;
    }

    StrView getTypeName(const Value& value) {
        if (value.isNil()) return "nil";
        if (value.isBoolean()) return "boolean";
        if (value.isNumber()) return "number";
        if (value.isString()) return "string";
        if (value.isTable()) return "table";
        if (value.isFunction()) return "function";
        return "unknown";
    }

    bool deepEqual(const Value& a, const Value& b) {
        // Simple equality check - can be enhanced for deep table comparison
        if (a.isNil() && b.isNil()) return true;
        if (a.isBoolean() && b.isBoolean()) return a.asBoolean() == b.asBoolean();
        if (a.isNumber() && b.isNumber()) return a.asNumber() == b.asNumber();
        if (a.isString() && b.isString()) return a.asString() == b.asString();
        // TODO: Implement deep table comparison
        return false;
    }

    i32 getLength(const Value& value) {
        if (value.isString()) {
            return static_cast<i32>(value.asString().length());
        }
        if (value.isTable()) {
            // TODO: Implement table length calculation
            return 0;
        }
        return 0;
    }

    bool isTruthy(const Value& value) {
        // In Lua, only nil and false are falsy
        return !(value.isNil() || (value.isBoolean() && !value.asBoolean()));
    }
}

// Note: Factory implementations are in their respective implementation files
// to avoid circular dependencies and missing includes
