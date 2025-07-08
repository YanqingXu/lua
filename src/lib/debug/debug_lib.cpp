#include "debug_lib.hpp"
#include "lib/core/lib_registry.hpp"
#include "vm/table.hpp"
#include <iostream>
#include <sstream>

namespace Lua {

void DebugLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create debug library table
    Value debugTable = LibRegistry::createLibTable(state, "debug");

    // Register debug functions
    REGISTER_TABLE_FUNCTION(state, debugTable, debug, debug);
    REGISTER_TABLE_FUNCTION(state, debugTable, getfenv, getfenv);
    REGISTER_TABLE_FUNCTION(state, debugTable, gethook, gethook);
    REGISTER_TABLE_FUNCTION(state, debugTable, getinfo, getinfo);
    REGISTER_TABLE_FUNCTION(state, debugTable, getlocal, getlocal);
    REGISTER_TABLE_FUNCTION(state, debugTable, getmetatable, getmetatable);
    REGISTER_TABLE_FUNCTION(state, debugTable, getregistry, getregistry);
    REGISTER_TABLE_FUNCTION(state, debugTable, getupvalue, getupvalue);
    REGISTER_TABLE_FUNCTION(state, debugTable, setfenv, setfenv);
    REGISTER_TABLE_FUNCTION(state, debugTable, sethook, sethook);
    REGISTER_TABLE_FUNCTION(state, debugTable, setlocal, setlocal);
    REGISTER_TABLE_FUNCTION(state, debugTable, setmetatable, setmetatable);
    REGISTER_TABLE_FUNCTION(state, debugTable, setupvalue, setupvalue);
    REGISTER_TABLE_FUNCTION(state, debugTable, traceback, traceback);
}

void DebugLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Debug library doesn't need special initialization
    std::cout << "[DebugLib] Initialized successfully!" << std::endl;
}

// ===================================================================
// Debug Function Implementations (Simplified Versions)
// ===================================================================

Value DebugLib::debug(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning

    // Simplified implementation - just print a message
    std::cout << "Debug mode not implemented in this simplified version" << std::endl;
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

void initializeDebugLib(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    DebugLib debugLib;
    debugLib.registerFunctions(state);
    debugLib.initialize(state);
}

} // namespace Lua
