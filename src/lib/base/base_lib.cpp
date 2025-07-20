#include "base_lib.hpp"
#include "../core/multi_return_helper.hpp"
#include "../../vm/table.hpp"
#include "../../vm/userdata.hpp"
#include "../../vm/metamethod_manager.hpp"
#include "../../vm/core_metamethods.hpp"
#include "../../common/types.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Lua {

// ===================================================================
// BaseLib Implementation
// ===================================================================

void BaseLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Register multi-return functions using new mechanism
    LibRegistry::registerGlobalFunction(state, "pcall", pcall);

    // Register legacy single-return functions
    LibRegistry::registerGlobalFunctionLegacy(state, "print", print);
    LibRegistry::registerGlobalFunctionLegacy(state, "type", type);
    LibRegistry::registerGlobalFunctionLegacy(state, "tostring", tostring);
    LibRegistry::registerGlobalFunctionLegacy(state, "tonumber", tonumber);
    LibRegistry::registerGlobalFunctionLegacy(state, "error", error);

    // Register table operation functions (legacy)
    LibRegistry::registerGlobalFunctionLegacy(state, "pairs", pairs);
    LibRegistry::registerGlobalFunctionLegacy(state, "ipairs", ipairs);
    LibRegistry::registerGlobalFunctionLegacy(state, "next", next);

    // Register metatable operation functions (legacy)
    LibRegistry::registerGlobalFunctionLegacy(state, "getmetatable", getmetatable);
    LibRegistry::registerGlobalFunctionLegacy(state, "setmetatable", setmetatable);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawget", rawget);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawset", rawset);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawlen", rawlen);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawequal", rawequal);

    // Register other utility functions (legacy)
    //LibRegistry::registerGlobalFunctionLegacy(state, "assert", assert);
    LibRegistry::registerGlobalFunctionLegacy(state, "select", select);
    LibRegistry::registerGlobalFunctionLegacy(state, "unpack", unpack);
}

void BaseLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Set global constants
    state->setGlobal("_VERSION", Value("Lua 5.1.1 (Modern C++ Implementation)"));
}

// ===================================================================
// Basic Function Implementations
// ===================================================================

Value BaseLib::print(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 0) {
        throw std::invalid_argument("Argument count cannot be negative");
    }

    // Output all arguments separated by tabs
    for (i32 i = 1; i <= nargs; ++i) {
        if (i > 1) {
            std::cout << "\t";
        }

        // Convert 1-based Lua index to 0-based stack index
        // Arguments are at stack[top-nargs] to stack[top-1]
        int stackIdx = state->getTop() - nargs + (i - 1);
        Value val = state->get(stackIdx);
        std::cout << val.toString();
    }
    std::cout << std::endl;

    return Value(); // nil
}

Value BaseLib::type(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value("nil");
    }

    // Convert 1-based Lua index to 0-based stack index
    int stackIdx = state->getTop() - nargs;
    Value val = state->get(stackIdx);

    if (val.isNil()) return Value("nil");
    if (val.isBoolean()) return Value("boolean");
    if (val.isNumber()) return Value("number");
    if (val.isString()) return Value("string");
    if (val.isTable()) return Value("table");
    if (val.isFunction()) return Value("function");

    return Value("userdata"); // Default case
}

Value BaseLib::tostring(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value("nil");
    }

    // Convert 1-based Lua index to 0-based stack index
    // Arguments are at stack[top-nargs] to stack[top-1]
    int stackIdx = state->getTop() - nargs;
    Value val = state->get(stackIdx);

    // CRITICAL FIX: Use CoreMetaMethods::handleToString to support __tostring metamethod
    // This enables custom objects to define their string representation
    try {
        Value result = CoreMetaMethods::handleToString(state, val);
        return result;
    } catch (const std::exception& e) {
        // Fallback to default string representation if metamethod fails
        std::cerr << "Error in __tostring metamethod: " << e.what() << std::endl;

        Str result = val.toString();
        return Value(result);
    }
}

Value BaseLib::tonumber(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil
    }

    // Convert 1-based Lua index to 0-based stack index
    // Arguments are at stack[top-nargs] to stack[top-1]
    int stackIdx = state->getTop() - nargs;
    Value val = state->get(stackIdx);

    if (val.isNumber()) {
        return val;
    }

    if (val.isString()) {
        try {
            Str str = val.toString();
            f64 num = std::stod(str);
            return Value(num);
        } catch (...) {
            return Value(); // nil if conversion fails
        }
    }

    return Value(); // nil
}

Value BaseLib::error(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    std::string message = "error";

    if (nargs >= 1) {
        // Convert 1-based Lua index to 0-based stack index
        int stackIdx = state->getTop() - nargs;
        Value val = state->get(stackIdx);
        message = val.toString();
    }

    // Throw LuaException to be caught by pcall
    throw LuaException(message);
}

// New Lua 5.1 standard pcall implementation (multi-return)
i32 BaseLib::pcall(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 1) {
        throw std::invalid_argument("pcall: function expected");
    }

    // DEBUG: Removed debug output for cleaner testing

    // Get function from stack (first argument) - now in clean stack environment
    Value func = state->get(0);
    // DEBUG: Removed debug output for cleaner testing

    // Prepare arguments for the function call (skip the function itself)
    Vec<Value> args;
    for (int i = 1; i < nargs; ++i) {
        args.push_back(state->get(i));
    }
    // DEBUG: Removed debug output for cleaner testing

    try {
        // Try to call the function with multiple return values
        CallResult callResult = state->callMultiple(func, args);

        // Clear the stack
        state->clearStack();

        // Push success flag
        state->push(Value(true));

        // Push all result values
        for (size_t i = 0; i < callResult.count; ++i) {
            state->push(callResult.getValue(i));
        }

        // DEBUG: Removed debug output for cleaner testing
        // Return total count (success flag + results)
        return static_cast<i32>(1 + callResult.count);

    } catch (const LuaException& e) {
        // DEBUG: Removed debug output for cleaner testing
        // Clear the stack
        state->clearStack();

        // Push error flag and message
        state->push(Value(false));
        state->push(Value(e.what()));

        // DEBUG: Removed debug output for cleaner testing
        // Return 2 values (false, error_message)
        return 2;

    } catch (const std::exception& e) {
        // DEBUG: Removed debug output for cleaner testing
        // Clear the stack
        state->clearStack();

        // Push error flag and message
        state->push(Value(false));
        state->push(Value(e.what()));

        // DEBUG: Removed debug output for cleaner testing
        // Return 2 values (false, error_message)
        return 2;
    }
}



// ===================================================================
// Table Operation Function Implementations (Simplified Versions)
// ===================================================================

Value BaseLib::pairs(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

Value BaseLib::ipairs(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

Value BaseLib::next(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

// ===================================================================
// Metatable Operation Function Implementations (Simplified Versions)
// ===================================================================

Value BaseLib::getmetatable(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        throw std::invalid_argument("getmetatable requires at least 1 argument");
    }

    // Get the object from stack (1-based Lua index to 0-based stack index)
    int stackIdx = state->getTop() - nargs;
    Value obj = state->get(stackIdx);

    // Only tables and userdata can have metatables in our implementation
    try {
        if (obj.isTable()) {
            auto table = obj.asTable();
            if (table) {
                GCRef<Table> metatable = table->getMetatable();
                if (metatable) {
                    // Return the metatable directly
                    return Value(metatable);
                }
            }
        } else if (obj.isUserdata()) {
            auto userdata = obj.asUserdata();
            if (userdata) {
                auto metatable = userdata->getMetatable();
                if (metatable) {
                    return Value(metatable);
                }
            }
        }
    } catch (...) {
        // If there's any error accessing metatable, return nil
    }

    return Value(); // nil if no metatable
}

Value BaseLib::setmetatable(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        throw std::invalid_argument("setmetatable requires 2 arguments");
    }

    // Get arguments from stack
    int stackIdx = state->getTop() - nargs;
    Value table = state->get(stackIdx);
    Value metatable = state->get(stackIdx + 1);

    // First argument must be a table
    if (!table.isTable()) {
        throw std::invalid_argument("setmetatable: first argument must be a table");
    }

    // Second argument must be a table or nil
    if (!metatable.isTable() && !metatable.isNil()) {
        throw std::invalid_argument("setmetatable: second argument must be a table or nil");
    }

    try {
        auto tablePtr = table.asTable();
        if (!tablePtr) {
            throw std::invalid_argument("setmetatable: invalid table");
        }

        if (metatable.isNil()) {
            // Remove metatable
            tablePtr->setMetatable(GCRef<Table>(nullptr));
        } else {
            // Set metatable
            auto metatablePtr = metatable.asTable();
            if (!metatablePtr) {
                throw std::invalid_argument("setmetatable: invalid metatable");
            }
            tablePtr->setMetatable(metatablePtr);
        }
    } catch (const std::exception& e) {
        throw std::invalid_argument(std::string("setmetatable: ") + e.what());
    }

    // Return the table (first argument)
    return table;
}

Value BaseLib::rawget(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    if (nargs < 2) {
        throw std::invalid_argument("rawget: expected at least 2 arguments (table, key)");
    }

    // Get arguments from stack (1-based Lua index to 0-based stack index)
    int stackIdx = state->getTop() - nargs;
    Value table = state->get(stackIdx);
    Value key = state->get(stackIdx + 1);

    // Check if first argument is a table
    if (!table.isTable()) {
        throw std::invalid_argument("rawget: first argument must be a table");
    }

    // Perform raw table access (without metamethods)
    auto tablePtr = table.asTable();
    return tablePtr->get(key);
}

Value BaseLib::rawset(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    if (nargs < 3) {
        throw std::invalid_argument("rawset: expected at least 3 arguments (table, key, value)");
    }

    // Get arguments from stack (1-based Lua index to 0-based stack index)
    int stackIdx = state->getTop() - nargs;
    Value table = state->get(stackIdx);
    Value key = state->get(stackIdx + 1);
    Value value = state->get(stackIdx + 2);

    // Check if first argument is a table
    if (!table.isTable()) {
        throw std::invalid_argument("rawset: first argument must be a table");
    }

    // Perform raw table assignment (without metamethods)
    auto tablePtr = table.asTable();
    tablePtr->set(key, value);

    // Return the table (first argument)
    return table;
}

Value BaseLib::rawlen(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    if (nargs < 1) {
        throw std::invalid_argument("rawlen: expected at least 1 argument");
    }

    // Get argument from stack (1-based Lua index to 0-based stack index)
    int stackIdx = state->getTop() - nargs;
    Value obj = state->get(stackIdx);

    // Check object type and return length
    if (obj.isTable()) {
        auto tablePtr = obj.asTable();
        return Value(static_cast<LuaNumber>(tablePtr->length()));
    } else if (obj.isString()) {
        const Str& str = obj.asString();
        return Value(static_cast<LuaNumber>(str.length()));
    } else {
        throw std::invalid_argument("rawlen: object must be a table or string");
    }
}

Value BaseLib::rawequal(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    if (nargs < 2) {
        throw std::invalid_argument("rawequal: expected at least 2 arguments");
    }

    // Get arguments from stack (1-based Lua index to 0-based stack index)
    int stackIdx = state->getTop() - nargs;
    Value v1 = state->get(stackIdx);
    Value v2 = state->get(stackIdx + 1);

    // Perform raw equality comparison (without metamethods)
    // This is a direct value comparison without calling __eq metamethod
    bool equal = (v1.type() == v2.type());

    if (equal) {
        switch (v1.type()) {
            case ValueType::Nil:
                equal = true;
                break;
            case ValueType::Boolean:
                equal = (v1.asBoolean() == v2.asBoolean());
                break;
            case ValueType::Number:
                equal = (v1.asNumber() == v2.asNumber());
                break;
            case ValueType::String:
                equal = (v1.asString() == v2.asString());
                break;
            case ValueType::Table:
                equal = (v1.asTable() == v2.asTable());
                break;
            case ValueType::Function:
                equal = (v1.asFunction() == v2.asFunction());
                break;
            case ValueType::Userdata:
                equal = (v1.asUserdata() == v2.asUserdata());
                break;
            default:
                equal = false;
                break;
        }
    }

    return Value(equal);
}

// ===================================================================
// Other Utility Function Implementations (Simplified Versions)
// ===================================================================

//Value BaseLib::assert(State* state, i32 nargs) {
//    if (nargs < 1) {
//        std::cerr << "Lua error: assertion failed!" << std::endl;
//        return Value();
//    }
//
//    // Convert 1-based Lua index to 0-based stack index
//    int stackIdx = state->getTop() - nargs;
//    Value val = state->get(stackIdx);
//    if (val.isNil() || (val.isBoolean() && !val.asBoolean())) {
//        std::string message = "assertion failed!";
//        if (nargs >= 2) {
//            // Second argument is at stackIdx + 1
//            Value msgVal = state->get(stackIdx + 1);
//            message = msgVal.toString();
//        }
//        std::cerr << "Lua error: " << message << std::endl;
//    }
//
//    return val;
//}

Value BaseLib::select(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    if (nargs < 1) {
        throw std::invalid_argument("select: expected at least 1 argument");
    }

    // Get the first argument (index or "#")
    int stackIdx = state->getTop() - nargs;
    Value indexVal = state->get(stackIdx);

    // Handle "#" case - return number of remaining arguments
    if (indexVal.isString()) {
        const Str& str = indexVal.asString();
        if (str == "#") {
            return Value(static_cast<LuaNumber>(nargs - 1));
        }
    }

    // Handle numeric index case
    if (!indexVal.isNumber()) {
        throw std::invalid_argument("select: first argument must be a number or '#'");
    }

    i32 index = static_cast<i32>(indexVal.asNumber());

    // Lua uses 1-based indexing
    if (index < 1 || index >= nargs) {
        throw std::invalid_argument("select: index out of range");
    }

    // Return the value at the specified index
    // index is 1-based, so we need to adjust for the stack
    Value result = state->get(stackIdx + index);
    return result;
}

Value BaseLib::unpack(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

// ===================================================================
// Convenient Initialization Functions
// ===================================================================

void initializeBaseLib(State* state) {
    if (!state) {
        return;
    }

    BaseLib baseLib;
    baseLib.registerFunctions(state);
    baseLib.initialize(state);
}

// For backward compatibility, provide old function names
namespace Lua {
    std::unique_ptr<LibModule> createBaseLib() {
        return std::make_unique<BaseLib>();
    }
}

} // namespace Lua
