#include "base_lib.hpp"
#include "../../vm/table.hpp"
#include "../../vm/userdata.hpp"
#include "../../vm/metamethod_manager.hpp"
#include <iostream>
#include <sstream>

namespace Lua {

// ===================================================================
// BaseLib Implementation
// ===================================================================

void BaseLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Register core functions to global environment
    REGISTER_GLOBAL_FUNCTION(state, print, print);
    REGISTER_GLOBAL_FUNCTION(state, type, type);
    REGISTER_GLOBAL_FUNCTION(state, tostring, tostring);
    REGISTER_GLOBAL_FUNCTION(state, tonumber, tonumber);
    REGISTER_GLOBAL_FUNCTION(state, error, error);

    // Register table operation functions
    REGISTER_GLOBAL_FUNCTION(state, pairs, pairs);
    REGISTER_GLOBAL_FUNCTION(state, ipairs, ipairs);
    REGISTER_GLOBAL_FUNCTION(state, next, next);

    // Register metatable operation functions
    REGISTER_GLOBAL_FUNCTION(state, getmetatable, getmetatable);
    REGISTER_GLOBAL_FUNCTION(state, setmetatable, setmetatable);
    REGISTER_GLOBAL_FUNCTION(state, rawget, rawget);
    REGISTER_GLOBAL_FUNCTION(state, rawset, rawset);
    REGISTER_GLOBAL_FUNCTION(state, rawlen, rawlen);
    REGISTER_GLOBAL_FUNCTION(state, rawequal, rawequal);

    // Register other utility functions
    //REGISTER_GLOBAL_FUNCTION(state, assert, assert);
    REGISTER_GLOBAL_FUNCTION(state, select, select);
    REGISTER_GLOBAL_FUNCTION(state, unpack, unpack);
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
    Str result = val.toString();

    return Value(result);
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

    // 简单的错误处理：输出错误信息
    std::cerr << "Lua error: " << message << std::endl;

    return Value(); // nil
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
    (void)nargs; // Not yet implemented
    return Value();
}

Value BaseLib::rawset(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

Value BaseLib::rawlen(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

Value BaseLib::rawequal(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
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
    (void)nargs; // Not yet implemented
    return Value();
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
