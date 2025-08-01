﻿#include "base_lib.hpp"
#include "../core/multi_return_helper.hpp"
#include "../../vm/table.hpp"
#include "../../vm/table_impl.hpp"
#include "../../vm/userdata.hpp"
#include "../../vm/metamethod_manager.hpp"
#include "../../vm/core_metamethods.hpp"
#include "../../vm/function.hpp"
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

    // Register table operation functions (multi-return)
    LibRegistry::registerGlobalFunction(state, "pairs", pairsMulti);
    LibRegistry::registerGlobalFunction(state, "ipairs", ipairsMulti);
    LibRegistry::registerGlobalFunction(state, "next", nextMulti);

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

    if (nargs < 1) {
        throw std::invalid_argument("pairs: expected 1 argument");
    }

    // Get the table argument from the stack
    int stackBase = state->getTop() - nargs;
    Value tableVal = state->get(stackBase);

    if (!tableVal.isTable()) {
        throw std::invalid_argument("pairs: argument must be a table");
    }

    // Create the pairs iterator function using nextMulti (fixed implementation)
    auto iteratorFunc = Function::createNative([](State* s) -> i32 {
        return nextMulti(s);
    });

    // Push the iterator function, table, and nil (initial key)
    state->push(Value(iteratorFunc));
    state->push(tableVal);
    state->push(Value()); // nil as initial key

    return 3; // Return 3 values: iterator, table, initial_key
}

Value BaseLib::ipairs(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        throw std::invalid_argument("ipairs: expected 1 argument");
    }

    // Get the table argument from the stack
    int stackBase = state->getTop() - nargs;
    Value tableVal = state->get(stackBase);

    if (!tableVal.isTable()) {
        throw std::invalid_argument("ipairs: argument must be a table");
    }

    // Create the ipairs iterator function
    auto iteratorFunc = Function::createNativeLegacy([](State* s, i32 n) -> Value {
        if (n < 2) {
            return Value(); // nil
        }

        int base = s->getTop() - n;
        Value tableVal = s->get(base);
        Value indexVal = s->get(base + 1);

        if (!tableVal.isTable() || !indexVal.isNumber()) {
            return Value(); // nil
        }

        auto table = tableVal.asTable();
        int currentIndex = static_cast<int>(indexVal.asNumber());
        int nextIndex = currentIndex + 1;

        // Check if next index exists in the table
        Value nextValue = table->get(Value(nextIndex));
        if (nextValue.isNil()) {
            return Value(); // End of iteration
        }

        // Push the next index and value
        s->push(Value(nextIndex));
        s->push(nextValue);
        return 2; // Return 2 values
    });

    // Push the iterator function, table, and initial index (0)
    state->push(Value(iteratorFunc));
    state->push(tableVal);
    state->push(Value(0));

    return 3; // Return 3 values: iterator, table, initial_index
}

Value BaseLib::next(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        throw std::invalid_argument("next: expected at least 1 argument");
    }

    // Get arguments from the stack
    int stackBase = state->getTop() - nargs;
    Value tableVal = state->get(stackBase);
    Value keyVal = (nargs >= 2) ? state->get(stackBase + 1) : Value(); // nil if not provided

    if (!tableVal.isTable()) {
        throw std::invalid_argument("next: first argument must be a table");
    }

    auto table = tableVal.asTable();

    // Simplified next implementation
    // For now, we'll implement a basic version that works with the existing Table interface
    // This is a simplified implementation that may not cover all edge cases

    if (keyVal.isNil()) {
        // Start iteration - try to find the first key
        // First check array part (index 1)
        Value firstValue = table->get(Value(1));
        if (!firstValue.isNil()) {
            state->push(Value(1));
            state->push(firstValue);
            return 2;
        }

        // If no array elements, we'd need to iterate hash part
        // For now, return nil (end of iteration)
        return Value(); // nil
    } else if (keyVal.isNumber()) {
        // Continue array iteration
        int currentIndex = static_cast<int>(keyVal.asNumber());
        int nextIndex = currentIndex + 1;

        Value nextValue = table->get(Value(nextIndex));
        if (!nextValue.isNil()) {
            state->push(Value(nextIndex));
            state->push(nextValue);
            return 2;
        }

        // End of array part, would need to check hash part
        // For now, return nil (end of iteration)
        return Value(); // nil
    } else {
        // Hash key iteration not implemented yet
        return Value(); // nil
    }
}

// ===================================================================
// Multi-Return Iterator Function Implementations
// ===================================================================

i32 BaseLib::ipairsMulti(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (state->getTop() < 1) {
        throw std::invalid_argument("ipairs: expected 1 argument");
    }

    // Get the table argument
    Value tableVal = state->get(0);

    if (!tableVal.isTable()) {
        throw std::invalid_argument("ipairs: argument must be a table");
    }

    // Create the ipairs iterator function
    auto iteratorFunc = Function::createNative([](State* s) -> i32 {
        if (s->getTop() < 2) {
            return 0; // No return values (end of iteration)
        }

        Value tableVal = s->get(0);
        Value indexVal = s->get(1);

        if (!tableVal.isTable() || !indexVal.isNumber()) {
            return 0; // No return values (end of iteration)
        }

        auto table = tableVal.asTable();
        int currentIndex = static_cast<int>(indexVal.asNumber());
        int nextIndex = currentIndex + 1;

        // Check if next index exists in the table
        Value nextValue = table->get(Value(nextIndex));
        if (nextValue.isNil()) {
            return 0; // End of iteration
        }

        // Clear stack and push return values
        s->clearStack();
        s->push(Value(nextIndex));
        s->push(nextValue);
        return 2; // Return 2 values
    });

    // Clear stack and push return values
    state->clearStack();
    state->push(Value(iteratorFunc));
    state->push(tableVal);
    state->push(Value(0)); // Initial index

    return 3; // Return 3 values: iterator, table, initial_index
}

i32 BaseLib::pairsMulti(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (state->getTop() < 1) {
        throw std::invalid_argument("pairs: expected 1 argument");
    }

    // Get the table argument
    Value tableVal = state->get(0);

    if (!tableVal.isTable()) {
        throw std::invalid_argument("pairs: argument must be a table");
    }

    // Create the pairs iterator function (using next)
    auto iteratorFunc = Function::createNative([](State* s) -> i32 {
        return nextMulti(s);
    });

    // Clear stack and push return values
    state->clearStack();
    state->push(Value(iteratorFunc));
    state->push(tableVal);
    state->push(Value()); // nil as initial key

    return 3; // Return 3 values: iterator, table, initial_key
}

i32 BaseLib::nextMulti(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (state->getTop() < 1) {
        throw std::invalid_argument("next: expected at least 1 argument");
    }

    // Get arguments
    Value tableVal = state->get(0);
    Value keyVal = (state->getTop() >= 2) ? state->get(1) : Value(); // nil if not provided

    if (!tableVal.isTable()) {
        throw std::invalid_argument("next: first argument must be a table");
    }

    auto table = tableVal.asTable();

    if (keyVal.isNil()) {
        // Start iteration - first check array part
        for (size_t i = 1; i <= table->getArraySize(); ++i) {
            Value value = table->get(Value(static_cast<LuaNumber>(i)));
            if (!value.isNil()) {
                state->clearStack();
                state->push(Value(static_cast<LuaNumber>(i)));
                state->push(value);
                return 2;
            }
        }

        // If no array elements, check hash part
        Value foundKey, foundValue;
        bool found = false;

        table->forEachHashEntry([&](const Value& key, const Value& value) {
            if (!found && !key.isNil() && !value.isNil()) {
                foundKey = key;
                foundValue = value;
                found = true;
            }
        });

        if (found) {
            state->clearStack();
            state->push(foundKey);
            state->push(foundValue);
            return 2;
        }

        return 0; // No elements
    } else if (keyVal.isNumber()) {
        // Continue array iteration
        LuaNumber currentNum = keyVal.asNumber();
        if (currentNum == std::floor(currentNum) && currentNum >= 1) {
            int currentIndex = static_cast<int>(currentNum);

            // Try next array indices
            for (int nextIndex = currentIndex + 1; nextIndex <= static_cast<int>(table->getArraySize()); ++nextIndex) {
                Value nextValue = table->get(Value(static_cast<LuaNumber>(nextIndex)));
                if (!nextValue.isNil()) {
                    state->clearStack();
                    state->push(Value(static_cast<LuaNumber>(nextIndex)));
                    state->push(nextValue);
                    return 2;
                }
            }

            // Array part exhausted, move to hash part
            Value foundKey, foundValue;
            bool found = false;

            table->forEachHashEntry([&](const Value& key, const Value& value) {
                if (!found && !key.isNil() && !value.isNil()) {
                    foundKey = key;
                    foundValue = value;
                    found = true;
                }
            });

            if (found) {
                state->clearStack();
                state->push(foundKey);
                state->push(foundValue);
                return 2;
            }
        }

        return 0; // End of iteration
    } else {
        // Continue hash iteration - find the next key after current key
        Value foundKey, foundValue;
        bool found = false;
        bool foundCurrent = false;

        table->forEachHashEntry([&](const Value& key, const Value& value) {
            if (!found && !key.isNil() && !value.isNil()) {
                if (foundCurrent) {
                    // This is the next key after current
                    foundKey = key;
                    foundValue = value;
                    found = true;
                } else if (key == keyVal) {
                    // Found current key, next iteration will be the answer
                    foundCurrent = true;
                }
            }
        });

        if (found) {
            state->clearStack();
            state->push(foundKey);
            state->push(foundValue);
            return 2;
        }

        return 0; // End of iteration
    }
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
    // 增强：更严格的参数验证
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        throw std::invalid_argument("setmetatable requires exactly 2 arguments");
    }

    // 增强：栈边界检查
    int stackTop = state->getTop();
    if (stackTop < nargs) {
        throw std::invalid_argument("setmetatable: insufficient arguments on stack");
    }

    // Get arguments from stack with bounds checking
    int stackIdx = stackTop - nargs;

    // 增强：安全的参数获取
    Value table, metatable;
    try {
        table = state->get(stackIdx);
        metatable = state->get(stackIdx + 1);
    } catch (const std::exception& e) {
        throw std::invalid_argument("setmetatable: failed to get arguments from stack: " + std::string(e.what()));
    }

    // 增强：更详细的类型检查
    if (!table.isTable()) {
        throw std::invalid_argument("setmetatable: first argument must be a table, got " +
                                  std::string(table.getTypeName()));
    }

    if (!metatable.isTable() && !metatable.isNil()) {
        throw std::invalid_argument("setmetatable: second argument must be a table or nil, got " +
                                  std::string(metatable.getTypeName()));
    }

    // 增强：安全的元表操作
    try {
        auto tablePtr = table.asTable();
        if (!tablePtr) {
            throw std::invalid_argument("setmetatable: failed to get table pointer");
        }

        // 增强：检查表的有效性
        if (!tablePtr) {
            throw std::invalid_argument("setmetatable: table object is null or corrupted");
        }

        if (metatable.isNil()) {
            // Remove metatable - 增强错误处理
            try {
                tablePtr->setMetatable(GCRef<Table>(nullptr));
            } catch (const std::exception& e) {
                throw std::invalid_argument("setmetatable: failed to remove metatable: " + std::string(e.what()));
            }
        } else {
            // Set metatable - 增强验证和错误处理
            auto metatablePtr = metatable.asTable();
            if (!metatablePtr) {
                throw std::invalid_argument("setmetatable: failed to get metatable pointer");
            }

            // 增强：检查元表的有效性
            if (!metatablePtr) {
                throw std::invalid_argument("setmetatable: metatable object is null or corrupted");
            }

            // 增强：防止循环引用
            if (tablePtr == metatablePtr) {
                throw std::invalid_argument("setmetatable: cannot set table as its own metatable");
            }

            try {
                tablePtr->setMetatable(metatablePtr);

                // 增强：验证设置是否成功
                auto verifyMT = tablePtr->getMetatable();
                if (verifyMT != metatablePtr) {
                    throw std::invalid_argument("setmetatable: metatable setting verification failed");
                }
            } catch (const std::exception& e) {
                throw std::invalid_argument("setmetatable: failed to set metatable: " + std::string(e.what()));
            }
        }
    } catch (const std::invalid_argument&) {
        // 重新抛出我们的错误
        throw;
    } catch (const std::exception& e) {
        throw std::invalid_argument("setmetatable: unexpected error: " + std::string(e.what()));
    }

    // Return the table (first argument) - Lua 5.1 standard behavior
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
