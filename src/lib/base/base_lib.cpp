#include "base_lib.hpp"
#include "../core/multi_return_helper.hpp"
#include "../../vm/table.hpp"
#include "../../vm/table_impl.hpp"
#include "../../vm/userdata.hpp"
#include "../../vm/metamethod_manager.hpp"
#include "../../vm/core_metamethods.hpp"
#include "../../vm/function.hpp"
#include "../../gc/core/gc_string.hpp"
#include "../../common/types.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Lua {

// ===================================================================
// BaseLib Implementation
// ===================================================================

void BaseLib::registerFunctions(LuaState* state) {
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

    // Register metatable operation functions (multi-return)
    LibRegistry::registerGlobalFunction(state, "getmetatable", getmetatableMulti);
    LibRegistry::registerGlobalFunction(state, "setmetatable", setmetatableMulti);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawget", rawget);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawset", rawset);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawlen", rawlen);
    LibRegistry::registerGlobalFunctionLegacy(state, "rawequal", rawequal);

    // Register other utility functions (legacy)
    //LibRegistry::registerGlobalFunctionLegacy(state, "assert", assert);
    LibRegistry::registerGlobalFunctionLegacy(state, "select", select);
    LibRegistry::registerGlobalFunctionLegacy(state, "unpack", unpack);
}

void BaseLib::initialize(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Set global constants
    auto versionStr = GCString::create("_VERSION");
    auto versionValue = GCString::create("Lua 5.1.1 (Modern C++ Implementation)");
    state->setGlobal(versionStr, Value(versionValue));
}

// ===================================================================
// Basic Function Implementations
// ===================================================================

Value BaseLib::print(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 0) {
        throw std::invalid_argument("Argument count cannot be negative");
    }

    // Lua 5.1 compatible print implementation:
    // Call tostring function for each argument to support __tostring metamethod

    // Get global tostring function
    auto tostringStr = GCString::create("tostring");
    Value tostringFunc = state->getGlobal(tostringStr);

    // Output all arguments separated by tabs
    for (i32 i = 1; i <= nargs; ++i) {
        if (i > 1) {
            std::cout << "\t";
        }

        // Convert 1-based Lua index to 0-based stack index
        // Arguments are at stack[top-nargs] to stack[top-1]
        int stackIdx = state->getTop() - nargs + (i - 1);
        Value val = state->get(stackIdx);

        // Call tostring function if available, otherwise fall back to direct conversion
        if (tostringFunc.isFunction()) {
            try {
                // Call tostring function using the correct method
                Vec<Value> args = {val};
                Value result = state->callFunction(tostringFunc, args);

                // Output the result
                if (result.isString()) {
                    std::cout << result.asString();
                } else {
                    // Fallback if tostring doesn't return a string
                    std::cout << val.toString();
                }
            } catch (...) {
                // If tostring call fails, fall back to direct conversion
                std::cout << val.toString();
            }
        } else {
            // No tostring function available, use direct conversion
            std::cout << val.toString();
        }
    }
    std::cout << std::endl;

    return Value(); // nil
}

Value BaseLib::type(LuaState* state, i32 nargs) {
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

Value BaseLib::tostring(LuaState* state, i32 nargs) {
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

Value BaseLib::tonumber(LuaState* state, i32 nargs) {
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

    // Get optional base parameter (Lua 5.1 compatible)
    int base = 10; // Default base is 10
    if (nargs >= 2) {
        Value baseVal = state->get(stackIdx + 1);
        if (baseVal.isNumber()) {
            f64 baseNum = baseVal.asNumber();
            base = static_cast<int>(baseNum);

            // Validate base range (Lua 5.1 standard: 2-36)
            if (base < 2 || base > 36) {
                return Value(); // nil for invalid base
            }
        } else {
            return Value(); // nil if base is not a number
        }
    }

    if (val.isNumber()) {
        // If input is already a number and base is 10, return as-is
        if (base == 10) {
            return val;
        } else {
            // For non-decimal bases, convert number to string first, then parse
            // This handles cases like tonumber(123, 16) which should return nil
            // because "123" is not a valid hex representation
            return Value(); // nil - numbers can only be converted with base 10
        }
    }

    if (val.isString()) {
        try {
            Str str = val.toString();

            // Remove leading/trailing whitespace
            size_t start = str.find_first_not_of(" \t\n\r\f\v");
            if (start == Str::npos) {
                return Value(); // nil for empty or whitespace-only string
            }
            size_t end = str.find_last_not_of(" \t\n\r\f\v");
            str = str.substr(start, end - start + 1);

            if (str.empty()) {
                return Value(); // nil for empty string
            }

            // Handle base conversion (Lua 5.1 compatible)
            if (base == 10) {
                // Decimal conversion - use standard library
                f64 num = std::stod(str);
                return Value(num);
            } else {
                // Non-decimal conversion - implement custom parser
                bool negative = false;
                size_t pos = 0;

                // Handle sign
                if (str[0] == '-') {
                    negative = true;
                    pos = 1;
                } else if (str[0] == '+') {
                    pos = 1;
                }

                if (pos >= str.length()) {
                    return Value(); // nil for just a sign
                }

                // Convert using specified base
                f64 result = 0.0;
                for (size_t i = pos; i < str.length(); ++i) {
                    char c = str[i];
                    int digit;

                    if (c >= '0' && c <= '9') {
                        digit = c - '0';
                    } else if (c >= 'A' && c <= 'Z') {
                        digit = c - 'A' + 10;
                    } else if (c >= 'a' && c <= 'z') {
                        digit = c - 'a' + 10;
                    } else {
                        return Value(); // nil for invalid character
                    }

                    if (digit >= base) {
                        return Value(); // nil for digit >= base
                    }

                    result = result * base + digit;
                }

                return Value(negative ? -result : result);
            }
        } catch (...) {
            return Value(); // nil if conversion fails
        }
    }

    return Value(); // nil
}

Value BaseLib::error(LuaState* state, i32 nargs) {
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
i32 BaseLib::pcall(LuaState* state) {
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

Value BaseLib::pairs(LuaState* state, i32 nargs) {
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
    auto iteratorFunc = Function::createNative([](LuaState* s) -> i32 {
        return nextMulti(s);
    });

    // Push the iterator function, table, and nil (initial key)
    state->push(Value(iteratorFunc));
    state->push(tableVal);
    state->push(Value()); // nil as initial key

    return 3; // Return 3 values: iterator, table, initial_key
}

Value BaseLib::ipairs(LuaState* state, i32 nargs) {
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
    auto iteratorFunc = Function::createNativeLegacy([](LuaState* s, i32 n) -> Value {
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

Value BaseLib::next(LuaState* state, i32 nargs) {
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

i32 BaseLib::ipairsMulti(LuaState* state) {
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
    auto iteratorFunc = Function::createNative([](LuaState* s) -> i32 {
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

i32 BaseLib::pairsMulti(LuaState* state) {
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
    auto iteratorFunc = Function::createNative([](LuaState* s) -> i32 {
        return nextMulti(s);
    });

    // Clear stack and push return values
    state->clearStack();
    state->push(Value(iteratorFunc));
    state->push(tableVal);
    state->push(Value()); // nil as initial key

    return 3; // Return 3 values: iterator, table, initial_key
}

i32 BaseLib::nextMulti(LuaState* state) {
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

Value BaseLib::getmetatable(LuaState* state, i32 nargs) {
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

Value BaseLib::setmetatable(LuaState* state, i32 nargs) {
    // ��ǿ�����ϸ�Ĳ�����֤
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        throw std::invalid_argument("setmetatable requires exactly 2 arguments");
    }

    // ��ǿ��ջ�߽���
    int stackTop = state->getTop();
    if (stackTop < nargs) {
        throw std::invalid_argument("setmetatable: insufficient arguments on stack");
    }

    // Get arguments from stack with bounds checking
    int stackIdx = stackTop - nargs;

    // ��ǿ����ȫ�Ĳ�����ȡ
    Value table, metatable;
    try {
        table = state->get(stackIdx);
        metatable = state->get(stackIdx + 1);
    } catch (const std::exception& e) {
        throw std::invalid_argument("setmetatable: failed to get arguments from stack: " + std::string(e.what()));
    }

    // ��ǿ������ϸ�����ͼ��
    if (!table.isTable()) {
        throw std::invalid_argument("setmetatable: first argument must be a table, got " +
                                  std::string(table.getTypeName()));
    }

    if (!metatable.isTable() && !metatable.isNil()) {
        throw std::invalid_argument("setmetatable: second argument must be a table or nil, got " +
                                  std::string(metatable.getTypeName()));
    }

    // ��ǿ����ȫ��Ԫ������
    try {
        auto tablePtr = table.asTable();
        if (!tablePtr) {
            throw std::invalid_argument("setmetatable: failed to get table pointer");
        }

        // ��ǿ����������Ч��
        if (!tablePtr) {
            throw std::invalid_argument("setmetatable: table object is null or corrupted");
        }

        if (metatable.isNil()) {
            // Remove metatable - ��ǿ������
            try {
                tablePtr->setMetatable(GCRef<Table>(nullptr));
            } catch (const std::exception& e) {
                throw std::invalid_argument("setmetatable: failed to remove metatable: " + std::string(e.what()));
            }
        } else {
            // Set metatable - ��ǿ��֤�ʹ�����
            auto metatablePtr = metatable.asTable();
            if (!metatablePtr) {
                throw std::invalid_argument("setmetatable: failed to get metatable pointer");
            }

            // ��ǿ�����Ԫ������Ч��
            if (!metatablePtr) {
                throw std::invalid_argument("setmetatable: metatable object is null or corrupted");
            }

            // ��ǿ����ֹѭ������
            if (tablePtr == metatablePtr) {
                throw std::invalid_argument("setmetatable: cannot set table as its own metatable");
            }

            try {
                tablePtr->setMetatable(metatablePtr);

                // ��ǿ����֤�����Ƿ�ɹ�
                auto verifyMT = tablePtr->getMetatable();
                if (verifyMT != metatablePtr) {
                    throw std::invalid_argument("setmetatable: metatable setting verification failed");
                }
            } catch (const std::exception& e) {
                throw std::invalid_argument("setmetatable: failed to set metatable: " + std::string(e.what()));
            }
        }
    } catch (const std::invalid_argument&) {
        // �����׳����ǵĴ���
        throw;
    } catch (const std::exception& e) {
        throw std::invalid_argument("setmetatable: unexpected error: " + std::string(e.what()));
    }

    // Return the table (first argument) - Lua 5.1 standard behavior
    return table;
}

// ===================================================================
// Multi-return versions of metatable functions
// ===================================================================

i32 BaseLib::getmetatableMulti(LuaState* state) {
    if (!state) {
        state->push(Value("getmetatable: state is null"));
        return 1;
    }

    if (state->getTop() < 1) {
        state->push(Value("getmetatable requires at least 1 argument"));
        return 1;
    }

    // Get the object from stack - arguments start from stack bottom
    // In multi-return functions, arguments are already on stack starting from index 0
    Value obj = state->get(state->getTop() - 1); // Get the last (first argument)

    // Only tables and userdata can have metatables in our implementation
    try {
        if (obj.isTable()) {
            auto table = obj.asTable();
            if (table) {
                GCRef<Table> metatable = table->getMetatable();
                if (metatable) {
                    // Return the metatable
                    state->clearStack();
                    state->push(Value(metatable));
                    return 1;
                }
            }
        } else if (obj.isUserdata()) {
            auto userdata = obj.asUserdata();
            if (userdata) {
                auto metatable = userdata->getMetatable();
                if (metatable) {
                    state->clearStack();
                    state->push(Value(metatable));
                    return 1;
                }
            }
        }
    } catch (const std::exception& e) {
        state->clearStack();
        state->push(Value("getmetatable error: " + std::string(e.what())));
        return 1;
    }

    // Return nil if no metatable
    state->clearStack();
    state->push(Value());
    return 1;
}

i32 BaseLib::setmetatableMulti(LuaState* state) {
    if (!state) {
        return 0;
    }

    if (state->getTop() < 2) {
        state->clearStack();
        state->push(Value("setmetatable requires exactly 2 arguments"));
        return 1;
    }

    // Get arguments from stack - arguments are at the top of stack
    Value table = state->get(state->getTop() - 2);    // First argument
    Value metatable = state->get(state->getTop() - 1); // Second argument

    // Type checking
    if (!table.isTable()) {
        state->clearStack();
        state->push(Value("setmetatable: first argument must be a table, got " +
                         std::string(table.getTypeName())));
        return 1;
    }

    if (!metatable.isTable() && !metatable.isNil()) {
        state->clearStack();
        state->push(Value("setmetatable: second argument must be a table or nil, got " +
                         std::string(metatable.getTypeName())));
        return 1;
    }

    try {
        auto tablePtr = table.asTable();
        if (!tablePtr) {
            state->clearStack();
            state->push(Value("setmetatable: failed to get table pointer"));
            return 1;
        }

        if (metatable.isNil()) {
            // Remove metatable
            tablePtr->setMetatable(GCRef<Table>(nullptr));
        } else {
            // Set metatable
            auto metatablePtr = metatable.asTable();
            if (!metatablePtr) {
                state->clearStack();
                state->push(Value("setmetatable: failed to get metatable pointer"));
                return 1;
            }

            // Prevent circular reference
            if (tablePtr == metatablePtr) {
                state->clearStack();
                state->push(Value("setmetatable: cannot set table as its own metatable"));
                return 1;
            }

            tablePtr->setMetatable(metatablePtr);
        }

        // Return the table (first argument) - Lua 5.1 standard behavior
        state->clearStack();
        state->push(table);
        return 1;

    } catch (const std::exception& e) {
        state->clearStack();
        state->push(Value("setmetatable error: " + std::string(e.what())));
        return 1;
    }
}

Value BaseLib::rawget(LuaState* state, i32 nargs) {
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

Value BaseLib::rawset(LuaState* state, i32 nargs) {
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

Value BaseLib::rawlen(LuaState* state, i32 nargs) {
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

Value BaseLib::rawequal(LuaState* state, i32 nargs) {
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

//Value BaseLib::assert(LuaState* state, i32 nargs) {
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

Value BaseLib::select(LuaState* state, i32 nargs) {
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

Value BaseLib::unpack(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    (void)nargs; // Not yet implemented
    return Value();
}

// ===================================================================
// Convenient Initialization Functions
// ===================================================================

void initializeBaseLib(LuaState* state) {
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
