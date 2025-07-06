#include "base_lib.hpp"
#include "lib_base_utils.hpp"
#include "../../common/types.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstdlib>

namespace Lua {

// ===================================================================
// BaseLib Implementation - Following Modern Framework Standards
// ===================================================================

/**
 * Register all BaseLib functions with the function registry
 * @param registry Function registry to register with
 * @param context Library context for configuration
 */
void BaseLib::registerFunctions(Lib::LibFuncRegistry& registry, const Lib::LibContext& context) {
    // Core functions - always registered
    registry.registerFunction(
        Lib::FunctionMetadata("print")
            .withDescription("Outputs arguments to stdout separated by tabs")
            .withArgs(0, -1)
            .withVariadic(),
        [](State* state, i32 nargs) -> Value { return BaseLib::print(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("type")
            .withDescription("Returns the type name of the given value")
            .withArgs(1, 1),
        [](State* state, i32 nargs) -> Value { return BaseLib::type(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("tostring")
            .withDescription("Converts any value to a string representation")
            .withArgs(1, 1),
        [](State* state, i32 nargs) -> Value { return BaseLib::tostring(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("tonumber")
            .withDescription("Converts string to number with optional base")
            .withArgs(1, 2),
        [](State* state, i32 nargs) -> Value { return BaseLib::tonumber(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("error")
            .withDescription("Raises an error with the given message")
            .withArgs(1, 2),
        [](State* state, i32 nargs) -> Value { return BaseLib::error(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("assert")
            .withDescription("Asserts that condition is true, raises error if false")
            .withArgs(1, -1)
            .withVariadic(),
        [](State* state, i32 nargs) -> Value { return BaseLib::assert_func(state, nargs); }
    );

    // Table iteration functions
    if (context.getConfig<bool>("enable_table_ops").value_or(true)) {
        registry.registerFunction(
            Lib::FunctionMetadata("pairs")
                .withDescription("Returns iterator for table key-value pairs")
                .withArgs(1, 1),
            [](State* state, i32 nargs) -> Value { return BaseLib::pairs(state, nargs); }
        );
        
        registry.registerFunction(
            Lib::FunctionMetadata("ipairs")
                .withDescription("Returns iterator for table integer indices")
                .withArgs(1, 1),
            [](State* state, i32 nargs) -> Value { return BaseLib::ipairs(state, nargs); }
        );
        
        registry.registerFunction(
            Lib::FunctionMetadata("next")
                .withDescription("Returns next key-value pair in table")
                .withArgs(1, 2),
            [](State* state, i32 nargs) -> Value { return BaseLib::next(state, nargs); }
        );
    }

    // Metatable operations
    if (context.getConfig<bool>("enable_metatable").value_or(true)) {
        registry.registerFunction(
            Lib::FunctionMetadata("getmetatable")
                .withDescription("Returns the metatable of the given object")
                .withArgs(1, 1),
            [](State* state, i32 nargs) -> Value { return BaseLib::getmetatable(state, nargs); }
        );
        
        registry.registerFunction(
            Lib::FunctionMetadata("setmetatable")
                .withDescription("Sets the metatable for the given table")
                .withArgs(2, 2),
            [](State* state, i32 nargs) -> Value { return BaseLib::setmetatable(state, nargs); }
        );
    }

    // Raw access functions
    registry.registerFunction(
        Lib::FunctionMetadata("rawget")
            .withDescription("Gets table value without invoking metamethods")
            .withArgs(2, 2),
        [](State* state, i32 nargs) -> Value { return BaseLib::rawget(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("rawset")
            .withDescription("Sets table value without invoking metamethods")
            .withArgs(3, 3),
        [](State* state, i32 nargs) -> Value { return BaseLib::rawset(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("rawlen")
            .withDescription("Returns raw length without invoking metamethods")
            .withArgs(1, 1),
        [](State* state, i32 nargs) -> Value { return BaseLib::rawlen(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("rawequal")
            .withDescription("Compares values without invoking metamethods")
            .withArgs(2, 2),
        [](State* state, i32 nargs) -> Value { return BaseLib::rawequal(state, nargs); }
    );

    // Protected call functions
    if (context.getConfig<bool>("enable_pcall").value_or(true)) {
        registry.registerFunction(
            Lib::FunctionMetadata("pcall")
                .withDescription("Calls function in protected mode")
                .withArgs(1, -1)
                .withVariadic(),
            [](State* state, i32 nargs) -> Value { return BaseLib::pcall(state, nargs); }
        );
        
        registry.registerFunction(
            Lib::FunctionMetadata("xpcall")
                .withDescription("Calls function in protected mode with error handler")
                .withArgs(2, -1)
                .withVariadic(),
            [](State* state, i32 nargs) -> Value { return BaseLib::xpcall(state, nargs); }
        );
    }

    // Utility functions
    registry.registerFunction(
        Lib::FunctionMetadata("select")
            .withDescription("Returns selected arguments based on index")
            .withArgs(1, -1)
            .withVariadic(),
        [](State* state, i32 nargs) -> Value { return BaseLib::select(state, nargs); }
    );
    
    registry.registerFunction(
        Lib::FunctionMetadata("unpack")
            .withDescription("Unpacks array elements as separate values")
            .withArgs(1, 3),
        [](State* state, i32 nargs) -> Value { return BaseLib::unpack(state, nargs); }
    );

    // Code loading functions (security sensitive)
    if (context.getConfig<bool>("enable_load").value_or(false)) {
        registry.registerFunction(
            Lib::FunctionMetadata("loadstring")
                .withDescription("Loads Lua code from string")
                .withArgs(1, 2),
            [](State* state, i32 nargs) -> Value { return BaseLib::loadstring(state, nargs); }
        );
        
        registry.registerFunction(
            Lib::FunctionMetadata("load")
                .withDescription("Loads Lua code from reader function")
                .withArgs(1, 3),
            [](State* state, i32 nargs) -> Value { return BaseLib::load(state, nargs); }
        );
    }
}

/**
 * Initialize the BaseLib module
 * @param state Lua state to initialize in
 * @param context Library context for configuration
 */
void BaseLib::initialize(State* state, const Lib::LibContext& context) {
    // Set global constants
    state->setGlobal("_VERSION", Value("Lua 5.1.1 (Modern C++ Implementation)"));
    
    // Output initialization info in debug mode
    if (context.getConfig<bool>("debug_mode").value_or(false)) {
        std::cout << "[BaseLib] Initialized with " << getName() << " v" << getVersion() << std::endl;
    }
}

/**
 * Cleanup the BaseLib module resources
 * @param state Lua state to cleanup from
 * @param context Library context for configuration
 */
void BaseLib::cleanup(State* state, const Lib::LibContext& context) {
    (void)state; // Suppress unused parameter warning
    // Clean up resources if needed
    if (context.getConfig<bool>("debug_mode").value_or(false)) {
        std::cout << "[BaseLib] Cleanup completed" << std::endl;
    }
}

// ===================================================================
// Core Base Function Implementations
// ===================================================================

/**
 * Implementation of print(...) function
 * @param state Lua state
 * @param nargs Number of arguments
 * @return nil
 */
Value BaseLib::print(State* state, i32 nargs) {
    // Output all arguments separated by tabs
    for (i32 i = 1; i <= nargs; ++i) {
        if (i > 1) {
            std::cout << "\t";
        }
        
        Value val = state->get(i);
        std::cout << Lib::BaseLibUtils::toString(val);
    }
    std::cout << std::endl;
    
    return Value(); // Return nil
}

/**
 * Implementation of type(v) function
 * @param state Lua state
 * @param nargs Number of arguments
 * @return Type name as string
 */
Value BaseLib::type(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    // Check argument count
    Lib::ArgUtils::checkArgCount(state, 1, "type");
    
    // Get argument and return type name
    Value val = state->get(1);
    return Value(Lib::BaseLibUtils::getTypeName(val));
}

Value BaseLib::tostring(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    // Check argument count
    Lib::ArgUtils::checkArgCount(state, 1, "tostring");
    
    // Get argument
    Value val = state->get(1);
    
    // Note: Metatable support not yet implemented in VM
    // For now, use direct conversion
    return Value(Lib::BaseLibUtils::toString(val));
}

Value BaseLib::tonumber(State* state, i32 nargs) {
    // Check argument count (1-2 parameters)
    Lib::ArgUtils::checkArgCount(state, 1, 2, "tonumber");
    
    Value value = state->get(1);
    
    // If already a number, return directly
    if (value.isNumber()) {
        return value;
    }
    
    // If string, try to convert
    if (value.isString()) {
        StrView str = value.asString();
        
        // Check base parameter
        i32 base = 10;
        if (nargs >= 2) {
            Value baseVal = Lib::ArgUtils::checkNumber(state, 2, "tonumber");
            base = static_cast<i32>(baseVal.asNumber());
            
            if (base < 2 || base > 36) {
                Lib::ErrorUtils::argumentError(state, 2, "base out of range");
            }
        }
        
        // Perform number conversion
        if (auto result = Lib::BaseLibUtils::stringToNumber(str, base)) {
            return Value(*result);
        }
    }
    
    // Conversion failed, return nil
    return Value();
}

Value BaseLib::error(State* state, i32 nargs) {
    // Check argument count (1-2 parameters)
    Lib::ArgUtils::checkArgCount(state, 1, 2, "error");
    
    Value message = state->get(1);
    i32 level = 1;
    
    if (nargs >= 2) {
        Value levelVal = Lib::ArgUtils::checkNumber(state, 2, "error");
        level = static_cast<i32>(levelVal.asNumber());
    }
    
    // 构造错误消息
    Str errorMsg = Lib::BaseLibUtils::toString(message);
    
    // 抛出错误（这里需要与VM的错误处理机制集成）
    Lib::ErrorUtils::error(state, errorMsg, level);
    
    return Value(); // 永远不会到达这里
}

Value BaseLib::assert_func(State* state, i32 nargs) {
    // 至少需要一个参数
    Lib::ArgUtils::checkArgCount(state, 1, -1, "assert");
    
    Value val = state->get(1);
    
    // 检查第一个参数的真假性
    if (!Lib::BaseLibUtils::isTruthy(val)) {
        Str message = "assertion failed!";
        
        // 如果有第二个参数，用作错误消息
        if (nargs >= 2) {
            Value msgVal = state->get(2);
            message = Lib::BaseLibUtils::toString(msgVal);
        }
        
        Lib::ErrorUtils::error(state, message);
    }
    
    // 返回第一个参数
    return val;
}

// ===================================================================
// 表操作函数实现
// ===================================================================

/**
 * Implementation of pairs(t) function
 * @param state Lua state
 * @param nargs Number of arguments
 * @return Iterator function, table, and initial key
 * @note Current implementation is limited due to VM constraints
 */
Value BaseLib::pairs(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    // Check arguments
    Lib::ArgUtils::checkArgCount(state, 1, "pairs");
    Value table = Lib::ArgUtils::checkTable(state, 1, "pairs");
    
    // Note: Full iterator implementation pending VM support for multiple returns
    // Current implementation returns the table as placeholder
    return table; // Temporary implementation
}

/**
 * Implementation of ipairs(t) function
 * @param state Lua state
 * @param nargs Number of arguments
 * @return Iterator function, table, and initial index
 * @note Current implementation is limited due to VM constraints
 */
Value BaseLib::ipairs(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    // Check arguments
    Lib::ArgUtils::checkArgCount(state, 1, "ipairs");
    Value table = Lib::ArgUtils::checkTable(state, 1, "ipairs");
    
    // Note: Full iterator implementation pending VM support for multiple returns
    // Current implementation returns the table as placeholder
    return table; // Temporary implementation
}

/**
 * Implementation of next(table [, index]) function
 * @param state Lua state
 * @param nargs Number of arguments
 * @return Next key and value, or nil if at end
 * @note Current implementation is limited due to VM constraints
 */
Value BaseLib::next(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    // Check arguments (1-2 parameters)
    Lib::ArgUtils::checkArgCount(state, 1, 2, "next");
    Value table = Lib::ArgUtils::checkTable(state, 1, "next");
    (void)table; // Suppress unused variable warning
    
    // Note: Table iteration pending full VM implementation
    // Current implementation returns nil as placeholder
    return Value(); // Return nil for now
}

// ===================================================================
// 元表操作函数实现
// ===================================================================

Value BaseLib::getmetatable(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    Lib::ArgUtils::checkArgCount(state, 1, "getmetatable");
    Value object = state->get(1);
    
    // Note: Metatable support not yet implemented in VM
    // For tables, we can check if Table class has metatable support
    if (object.isTable()) {
        auto tableObj = object.asTable();
        if (tableObj && tableObj->getMetatable()) {
            // Note: Table* to Value conversion pending metatable system completion
            return Value(); // Return nil for now
        }
    }
    
    return Value(); // Return nil if no metatable
}

Value BaseLib::setmetatable(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    Lib::ArgUtils::checkArgCount(state, 2, "setmetatable");
    Value table = Lib::ArgUtils::checkTable(state, 1, "setmetatable");
    Value metatable = state->get(2);
    
    // Note: Metatable support is limited in current VM
    if (table.isTable()) {
        auto tableObj = table.asTable();
        if (metatable.isNil()) {
            tableObj->setMetatable(nullptr);
        } else if (metatable.isTable()) {
            auto metaTableObj = metatable.asTable();
            tableObj->setMetatable(metaTableObj.get());
        } else {
            Lib::ErrorUtils::typeError(state, 2, "nil or table");
        }
    }
    
    return table; // Return original table
}

// ===================================================================
// 原始访问函数实现
// ===================================================================

Value BaseLib::rawget(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    Lib::ArgUtils::checkArgCount(state, 2, "rawget");
    Value table = Lib::ArgUtils::checkTable(state, 1, "rawget");
    Value index = state->get(2);
    
    // Direct table access, ignoring metamethods
    auto tableObj = table.asTable();
    return tableObj->get(index); // Use regular get() since rawGet() doesn't exist yet
}

Value BaseLib::rawset(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    Lib::ArgUtils::checkArgCount(state, 3, "rawset");
    Value table = Lib::ArgUtils::checkTable(state, 1, "rawset");
    Value index = state->get(2);
    Value value = state->get(3);
    
    // Direct table access, ignoring metamethods
    auto tableObj = table.asTable();
    tableObj->set(index, value); // Use regular set() since rawSet() doesn't exist yet
    
    return table; // Return table
}

Value BaseLib::rawlen(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    Lib::ArgUtils::checkArgCount(state, 1, "rawlen");
    Value object = state->get(1);
    
    if (object.isString()) {
        return Value(static_cast<f64>(object.asString().length()));
    } else if (object.isTable()) {
        auto tableObj = object.asTable();
        return Value(static_cast<f64>(tableObj->length())); // Use length() instead of rawLength()
    } else {
        Lib::ErrorUtils::typeError(state, 1, "string or table");
    }
    
    return Value(); // Should not reach here
}

Value BaseLib::rawequal(State* state, i32 nargs) {
    (void)nargs; // Suppress unused parameter warning
    Lib::ArgUtils::checkArgCount(state, 2, "rawequal");
    Value v1 = state->get(1);
    Value v2 = state->get(2);
    
    // Direct comparison, ignoring metamethods
    bool equal = Lib::BaseLibUtils::rawEqual(v1, v2);
    return Value(equal);
}

// ===================================================================
// 高级函数实现 (存根版本)
// ===================================================================

Value BaseLib::pcall(State* state, i32 nargs) {
    // Simplified pcall implementation
    if (nargs < 1) {
        return Value(false);
    }
    
    try {
        Value func = state->get(1);
        if (!func.isFunction()) {
            state->push(Value(false));
            state->push(Value("attempt to call a non-function value"));
            return Value(); // Return 2 values
        }
        
        // For now, just call the function and assume success
        // In real implementation, this would use protected call
        state->push(Value(true));
        // TODO: Actually call the function and return its results
        return Value(); // Return success + results
    } catch (const std::exception& e) {
        state->push(Value(false));
        state->push(Value(e.what()));
        return Value(); // Return 2 values
    }
}

Value BaseLib::xpcall(State* state, i32 nargs) {
    // Simplified xpcall implementation
    if (nargs < 2) {
        return Value(false);
    }
    
    // Similar to pcall but with error handler
    return pcall(state, nargs - 1); // Skip the error handler for now
}

Value BaseLib::select(State* state, i32 nargs) {
    if (nargs < 1) {
        Lib::ErrorUtils::argumentError(state, 1, "value expected");
        return Value(); // Return nil
    }
    
    Value index = state->get(1);
    if (index.isString() && index.asString() == "#") {
        // Return count of remaining arguments
        return Value(static_cast<double>(nargs - 1));
    } else if (index.isNumber()) {
        int idx = static_cast<int>(index.asNumber());
        if (idx < 1 || idx >= nargs) {
            Lib::ErrorUtils::argumentError(state, 1, "index out of range");
            return Value(); // Return nil
        }
        // Return arguments starting from index
        // For now, just return the indexed argument
        return state->get(idx + 1);
    } else {
        Lib::ErrorUtils::typeError(state, 1, "number or '#'");
        return Value(); // Return nil
    }
}

Value BaseLib::unpack(State* state, int nargs) {
    // Stub implementation for unpack function
    if (nargs < 1) {
        Lib::ErrorUtils::argumentError(state, 1, "table expected");
        return Value();
    }
    
    Value table = Lib::ArgUtils::checkTable(state, 1, "unpack");
    // For now, just return the first element or nil
    // Real implementation would unpack all array elements
    return Value(); // Stub
}

Value BaseLib::loadstring(State* state, int nargs) {
    // Stub implementation for loadstring function
    if (nargs < 1) {
        Lib::ErrorUtils::argumentError(state, 1, "string expected");
        return Value();
    }
    
    Value str = Lib::ArgUtils::checkString(state, 1, "loadstring");
    // For now, just return nil (compilation failed)
    // Real implementation would compile the string to a function
    return Value(); // Stub
}

Value BaseLib::load(State* state, int nargs) {
    // Stub implementation for load function
    if (nargs < 1) {
        Lib::ErrorUtils::argumentError(state, 1, "function or string expected");
        return Value();
    }
    
    // For now, just return nil (compilation failed)
    // Real implementation would load from reader function or string
    return Value(); // Stub
}

} // namespace Lua

// Factory function implementation
std::unique_ptr<Lua::Lib::LibModule> Lua::createBaseLib() {
    return std::make_unique<Lua::BaseLib>();
}
