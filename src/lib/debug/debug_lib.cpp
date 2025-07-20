#include "debug_lib.hpp"
#include "../core/lib_registry.hpp"
#include "../../vm/table.hpp"
#include "../../common/defines.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

namespace Lua {

void DebugLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create debug library table
    Value debugTable = LibRegistry::createLibTable(state, "debug");

    // Register debug functions (all legacy single-return for now)
    // Note: When debug functions are fully implemented, gethook, getlocal,
    // and getupvalue should be converted to multi-return functions
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "debug", debug);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "getfenv", getfenv);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "gethook", gethook);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "getinfo", getinfo);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "getlocal", getlocal);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "getmetatable", getmetatable);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "getregistry", getregistry);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "getupvalue", getupvalue);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "setfenv", setfenv);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "sethook", sethook);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "setlocal", setlocal);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "setmetatable", setmetatable);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "setupvalue", setupvalue);
    LibRegistry::registerTableFunctionLegacy(state, debugTable, "traceback", traceback);
}

void DebugLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Debug library doesn't need special initialization
    // Debug library initialized
}

// ===================================================================
// Debug Function Implementations (Simplified Versions)
// ===================================================================

Value DebugLib::debug(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // No arguments expected for debug()

    std::cout << "\n=== Lua Debug Mode ===" << std::endl;
    std::cout << "Type 'help' for available commands, 'cont' to continue execution" << std::endl;

    // Interactive debug loop
    std::string input;
    bool continueExecution = false;

    while (!continueExecution) {
        std::cout << "lua_debug> ";
        std::getline(std::cin, input);

        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);

        if (input.empty()) {
            continue;
        }

        // Process debug commands
        if (input == "help" || input == "h") {
            printDebugHelp();
        }
        else if (input == "cont" || input == "c") {
            continueExecution = true;
        }
        else if (input == "stack" || input == "s") {
            printStackInfo(state);
        }
        else if (input == "globals" || input == "g") {
            printGlobals(state);
        }
        else if (input == "quit" || input == "q") {
            std::cout << "Exiting debug mode..." << std::endl;
            continueExecution = true;
        }
        else if (input.substr(0, 5) == "eval ") {
            // Evaluate Lua expression
            std::string expr = input.substr(5);
            evaluateExpression(state, expr);
        }
        else if (input.substr(0, 4) == "set ") {
            // Set variable (simplified)
            std::string assignment = input.substr(4);
            executeAssignment(state, assignment);
        }
        else {
            std::cout << "Unknown command: " << input << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }

    std::cout << "Continuing execution..." << std::endl;
    return Value(); // nil
}

Value DebugLib::getfenv(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning

    // Simplified implementation - return nil
    return Value(); // nil
}

Value DebugLib::gethook(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning

    // Simplified implementation - return nil (no hook set)
    return Value(); // nil
}

Value DebugLib::getinfo(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Simplified implementation - return basic debug info
    return createDebugInfo(state);
}

Value DebugLib::getlocal(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    i32 level = validateLevel(state, 1);
    if (level < 0) {
        return Value(); // nil - invalid level
    }

    // Simplified implementation - return nil (no locals available)
    return Value(); // nil
}

Value DebugLib::getmetatable(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Simplified implementation - return nil (no metatable support yet)
    return Value(); // nil
}

Value DebugLib::getregistry(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning

    // Simplified implementation - return nil (no registry access yet)
    return Value(); // nil
}

Value DebugLib::getupvalue(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    if (!validateFunction(state, 1)) {
        return Value(); // nil - invalid function
    }

    // Simplified implementation - return nil (no upvalue support yet)
    return Value(); // nil
}

Value DebugLib::setfenv(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    // Simplified implementation - return nil (no environment setting yet)
    return Value(); // nil
}

Value DebugLib::sethook(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    // Simplified implementation - do nothing
    return Value(); // nil
}

Value DebugLib::setlocal(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 3) {
        return Value(); // nil - insufficient arguments
    }

    i32 level = validateLevel(state, 1);
    if (level < 0) {
        return Value(); // nil - invalid level
    }

    // Simplified implementation - return nil (no local setting yet)
    return Value(); // nil
}

Value DebugLib::setmetatable(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    // Simplified implementation - return the object unchanged
    return state->get(1);
}

Value DebugLib::setupvalue(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 3) {
        return Value(); // nil - insufficient arguments
    }

    if (!validateFunction(state, 1)) {
        return Value(); // nil - invalid function
    }

    // Simplified implementation - return nil (no upvalue setting yet)
    return Value(); // nil
}

Value DebugLib::traceback(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    Str message = "";
    i32 level = 1;

    if (nargs >= 1) {
        Value msgVal = state->get(1);
        if (msgVal.isString()) {
            message = msgVal.toString();
        }
    }

    if (nargs >= 2) {
        Value levelVal = state->get(2);
        if (levelVal.isNumber()) {
            level = static_cast<i32>(levelVal.asNumber());
        }
    }

    // Build simplified traceback
    std::ostringstream traceback;
    if (!message.empty()) {
        traceback << message << "\n";
    }
    traceback << "stack traceback:\n";

    // Simplified implementation - just show a few levels
    for (i32 i = level; i <= level + 3; ++i) {
        Str functionName = getFunctionName(state, i);
        Str sourceInfo = getSourceInfo(state, i);
        traceback << formatTracebackLine(i, functionName, sourceInfo) << "\n";
    }

    return Value(traceback.str());
}

// ===================================================================
// Helper Functions
// ===================================================================

i32 DebugLib::validateLevel(State* state, i32 argIndex) {
    if (!state) {
        return -1;
    }

    Value val = state->get(argIndex);
    if (!val.isNumber()) {
        return -1;
    }

    i32 level = static_cast<i32>(val.asNumber());
    if (level < 0) {
        return -1;
    }

    return level;
}

bool DebugLib::validateFunction(State* state, i32 argIndex) {
    if (!state) {
        return false;
    }

    Value val = state->get(argIndex);
    return val.isFunction();
}

Value DebugLib::createDebugInfo(State* state) {
    if (!state) {
        return Value(); // nil
    }

    // In a real implementation, this would create a proper table with debug info
    // For now, return nil as we need proper table creation support
    return Value(); // nil
}

Str DebugLib::getFunctionName(State* state, i32 level) {
    if (!state) {
        return "?";
    }

    (void)level; // Suppress unused parameter warning

    // Simplified implementation - return generic name
    return "function";
}

Str DebugLib::getSourceInfo(State* state, i32 level) {
    if (!state) {
        return "unknown";
    }

    (void)level; // Suppress unused parameter warning

    // Simplified implementation - return generic source info
    return "[C]";
}

Str DebugLib::formatTracebackLine(i32 level, const Str& functionName, const Str& sourceInfo) {
    std::ostringstream line;
    line << "\t" << sourceInfo << ": in " << functionName;
    if (level > 1) {
        line << " (level " << level << ")";
    }
    return line.str();
}

// ===================================================================
// Debug Helper Functions Implementation
// ===================================================================

void DebugLib::printDebugHelp() {
    std::cout << "\nAvailable debug commands:" << std::endl;
    std::cout << "  help, h      - Show this help message" << std::endl;
    std::cout << "  cont, c      - Continue execution" << std::endl;
    std::cout << "  stack, s     - Show stack information" << std::endl;
    std::cout << "  globals, g   - Show global variables" << std::endl;
    std::cout << "  eval <expr>  - Evaluate Lua expression" << std::endl;
    std::cout << "  set <var>=<val> - Set variable value" << std::endl;
    std::cout << "  quit, q      - Exit debug mode" << std::endl;
    std::cout << std::endl;
}

void DebugLib::printStackInfo(State* state) {
    if (!state) {
        std::cout << "Error: Invalid state" << std::endl;
        return;
    }

    std::cout << "\n=== Stack Information ===" << std::endl;
    std::cout << "Stack size: " << state->getTop() << std::endl;

    // Show top few stack values
    int top = state->getTop();
    int showCount = std::min(top, 10); // Show up to 10 values

    for (int i = 0; i < showCount; ++i) {
        Value val = state->get(i);
        std::cout << "  [" << i << "] " << val.toString() << " (" << val.getTypeName() << ")" << std::endl;
    }

    if (top > showCount) {
        std::cout << "  ... and " << (top - showCount) << " more values" << std::endl;
    }
    std::cout << std::endl;
}

void DebugLib::printGlobals(State* state) {
    if (!state) {
        std::cout << "Error: Invalid state" << std::endl;
        return;
    }

    std::cout << "\n=== Global Variables ===" << std::endl;

    // Get global table
    Value globalTable = state->getGlobal("_G");
    if (globalTable.isTable()) {
        auto table = globalTable.asTable();
        std::cout << "Global table found with " << table->length() << " entries" << std::endl;

        // Show some common globals
        const char* commonGlobals[] = {"print", "type", "tostring", "tonumber", "_VERSION", "math", "string", "table"};
        for (const char* name : commonGlobals) {
            Value val = state->getGlobal(name);
            if (!val.isNil()) {
                std::cout << "  " << name << " = " << val.toString() << " (" << val.getTypeName() << ")" << std::endl;
            }
        }
    } else {
        std::cout << "Global table not available" << std::endl;
    }
    std::cout << std::endl;
}

void DebugLib::evaluateExpression(State* state, const Str& expr) {
    if (!state || expr.empty()) {
        std::cout << "Error: Invalid expression" << std::endl;
        return;
    }

    try {
        // Try to evaluate the expression as a return statement
        std::string code = "return " + expr;
        Value result = state->doStringWithResult(code);

        std::cout << "Result: " << result.toString() << " (" << result.getTypeName() << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error evaluating expression: " << e.what() << std::endl;
    }
}

void DebugLib::executeAssignment(State* state, const Str& assignment) {
    if (!state || assignment.empty()) {
        std::cout << "Error: Invalid assignment" << std::endl;
        return;
    }

    try {
        // Execute the assignment directly
        bool success = state->doString(assignment);
        if (success) {
            std::cout << "Assignment executed successfully" << std::endl;
        } else {
            std::cout << "Assignment failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error executing assignment: " << e.what() << std::endl;
    }
}

void initializeDebugLib(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    DebugLib debugLib;
    debugLib.registerFunctions(state);
    debugLib.initialize(state);
}

} // namespace Lua
