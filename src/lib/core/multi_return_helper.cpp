#include "multi_return_helper.hpp"
#include "../../vm/state.hpp"
#include <stdexcept>

namespace Lua {

    CallResult MultiReturnHelper::createTwoValues(const Value& first, const Value& second) {
        Vec<Value> values;
        values.push_back(first);
        values.push_back(second);
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createThreeValues(const Value& first, const Value& second, const Value& third) {
        Vec<Value> values;
        values.push_back(first);
        values.push_back(second);
        values.push_back(third);
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createMultipleValues(const Vec<Value>& values) {
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createPCallSuccess(const Value& result) {
        Vec<Value> values;
        values.push_back(Value(true));  // Success flag
        values.push_back(result);       // The actual result
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createPCallSuccessMultiple(const Vec<Value>& results) {
        Vec<Value> values;
        values.push_back(Value(true));  // Success flag
        
        // Add all result values
        for (const auto& result : results) {
            values.push_back(result);
        }
        
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createPCallError(const Str& errorMessage) {
        Vec<Value> values;
        values.push_back(Value(false));           // Error flag
        values.push_back(Value(errorMessage));    // Error message
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createLoadError(const Str& errorMessage) {
        Vec<Value> values;
        values.push_back(Value());                // nil (load failed)
        values.push_back(Value(errorMessage));    // Error message
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createLoadSuccess(const Value& loadedFunction) {
        return CallResult(loadedFunction);  // Single function value
    }

    i32 MultiReturnHelper::pushMultipleValues(State* state, const Vec<Value>& values) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        for (const auto& value : values) {
            state->push(value);
        }

        return static_cast<i32>(values.size());
    }

    i32 MultiReturnHelper::returnCallResult(State* state, const CallResult& result) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        // Push all values from the CallResult to the stack
        for (size_t i = 0; i < result.count; ++i) {
            state->push(result.getValue(i));
        }

        return static_cast<i32>(result.count);
    }

    CallResult MultiReturnHelper::createMathTwoValues(f64 first, f64 second) {
        Vec<Value> values;
        values.push_back(Value(first));
        values.push_back(Value(second));
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createMathTwoValues(f64 first, i32 second) {
        Vec<Value> values;
        values.push_back(Value(first));
        values.push_back(Value(static_cast<f64>(second)));  // Convert to f64 for Lua
        return CallResult(values);
    }

    CallResult MultiReturnHelper::createNilResult() {
        return CallResult(Value());  // Single nil value
    }

    CallResult MultiReturnHelper::createSingleValue(const Value& value) {
        return CallResult(value);
    }

} // namespace Lua
