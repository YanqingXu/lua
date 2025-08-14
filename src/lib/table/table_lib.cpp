#include "table_lib.hpp"
#include "lib/core/lib_registry.hpp"
#include "vm/table.hpp"
#include "../../common/defines.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace Lua {

void TableLib::registerFunctions(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create table library table
    Value tableTable = LibRegistry::createLibTable(state, "table");

    // Register table manipulation functions (all legacy single-return)
    LibRegistry::registerTableFunctionLegacy(state, tableTable, "insert", insert);
    LibRegistry::registerTableFunctionLegacy(state, tableTable, "remove", remove);
    LibRegistry::registerTableFunctionLegacy(state, tableTable, "sort", sort);
    LibRegistry::registerTableFunctionLegacy(state, tableTable, "concat", concat);
    LibRegistry::registerTableFunctionLegacy(state, tableTable, "getn", getn);
    LibRegistry::registerTableFunctionLegacy(state, tableTable, "maxn", maxn);
}

void TableLib::initialize(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Table library doesn't need special initialization
    // Table library initialized
}

// ===================================================================
// Table Function Implementations
// ===================================================================

Value TableLib::insert(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    // Get arguments using correct stack indexing
    int stackBase = state->getTop() - nargs;

    // For table library functions called as table.insert(arr, value):
    // - First argument is the table module itself (skip it)
    // - Second argument is the table to operate on
    // - Third argument is the value or position

    if (nargs < 2) {
        std::cout << "[DEBUG] table.insert: insufficient arguments" << std::endl;
        return Value();
    }

    // Skip the first argument (table module) and get the actual table
    Value tableVal = state->get(stackBase + 1);

    if (!tableVal.isTable()) {
        return Value(); // nil - invalid table
    }

    auto table = tableVal.asTable();

    if (nargs == 3) {
        // table.insert(table_module, table, value) - insert at end
        Value value = state->get(stackBase + 2);
        i32 length = getTableLength(table);
        table->set(Value(static_cast<f64>(length + 1)), value);
    } else if (nargs == 4) {
        // table.insert(table_module, table, pos, value) - insert at position
        Value posVal = state->get(stackBase + 2);
        Value value = state->get(stackBase + 3);

        if (!posVal.isNumber()) {
            return Value(); // nil - invalid position
        }

        i32 pos = static_cast<i32>(posVal.asNumber());
        i32 length = getTableLength(table);

        // Shift elements to the right
        for (i32 i = length; i >= pos; --i) {
            Value oldVal = table->get(Value(static_cast<f64>(i)));
            table->set(Value(static_cast<f64>(i + 1)), oldVal);
        }

        // Insert new value
        table->set(Value(static_cast<f64>(pos)), value);
    }

    return Value(); // nil
}

Value TableLib::remove(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Get arguments using correct stack indexing
    int stackBase = state->getTop() - nargs;
    Value tableVal = state->get(stackBase);

    if (!tableVal.isTable()) {
        return Value(); // nil - invalid table
    }

    auto table = tableVal.asTable();

    i32 pos;
    if (nargs >= 2) {
        Value posVal = state->get(stackBase + 1);
        if (!posVal.isNumber()) {
            return Value(); // nil - invalid position
        }
        pos = static_cast<i32>(posVal.asNumber());
    } else {
        // Remove from end if no position specified
        pos = getTableLength(table);
    }

    if (pos <= 0) {
        return Value(); // nil - invalid position
    }

    // Get the value to be removed
    Value removedVal = table->get(Value(static_cast<f64>(pos)));

    // Shift elements to the left
    i32 length = getTableLength(table);
    for (i32 i = pos; i < length; ++i) {
        Value nextVal = table->get(Value(static_cast<f64>(i + 1)));
        table->set(Value(static_cast<f64>(i)), nextVal);
    }

    // Remove the last element
    table->set(Value(static_cast<f64>(length)), Value());

    return removedVal;
}

Value TableLib::sort(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    GCRef<Table> table = validateTableArg(state, 1);
    if (!table) {
        return Value(); // nil - invalid table
    }

    // For now, implement a simple sort without custom comparison function
    // This is a simplified implementation
    i32 length = getTableLength(table);

    // Simple bubble sort for demonstration
    for (i32 i = 1; i <= length - 1; ++i) {
        for (i32 j = 1; j <= length - i; ++j) {
            Value val1 = table->get(Value(static_cast<f64>(j)));
            Value val2 = table->get(Value(static_cast<f64>(j + 1)));

            // Simple comparison - numbers first, then strings
            bool shouldSwap = false;
            if (val1.isNumber() && val2.isNumber()) {
                shouldSwap = val1.asNumber() > val2.asNumber();
            } else if (val1.isString() && val2.isString()) {
                shouldSwap = val1.toString() > val2.toString();
            }

            if (shouldSwap) {
                table->set(Value(static_cast<f64>(j)), val2);
                table->set(Value(static_cast<f64>(j + 1)), val1);
            }
        }
    }

    return Value(); // nil
}

Value TableLib::concat(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    GCRef<Table> table = validateTableArg(state, 1);
    if (!table) {
        return Value(); // nil - invalid table
    }

    Str separator = "";
    i32 start = 1;
    i32 end = getTableLength(table);

    // Parse optional arguments
    if (nargs >= 2) {
        Value sepVal = state->get(2);
        if (sepVal.isString()) {
            separator = sepVal.toString();
        }
    }

    if (nargs >= 3) {
        Value startVal = state->get(3);
        if (startVal.isNumber()) {
            start = static_cast<i32>(startVal.asNumber());
        }
    }

    if (nargs >= 4) {
        Value endVal = state->get(4);
        if (endVal.isNumber()) {
            end = static_cast<i32>(endVal.asNumber());
        }
    }

    // Concatenate elements
    std::ostringstream result;
    for (i32 i = start; i <= end; ++i) {
        if (i > start) {
            result << separator;
        }
        Value val = table->get(Value(static_cast<f64>(i)));
        result << val.toString();
    }

    return Value(result.str());
}

Value TableLib::getn(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    GCRef<Table> table = validateTableArg(state, 1);
    if (!table) {
        return Value(); // nil - invalid table
    }

    return Value(static_cast<f64>(getTableLength(table)));
}

Value TableLib::maxn(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    GCRef<Table> table = validateTableArg(state, 1);
    if (!table) {
        return Value(); // nil - invalid table
    }

    // Find maximum numeric index
    // This is a simplified implementation
    i32 maxIndex = 0;

    // In a real implementation, we would iterate through all keys
    // For now, just return the table length
    maxIndex = getTableLength(table);

    return Value(static_cast<f64>(maxIndex));
}

// ===================================================================
// Helper Functions
// ===================================================================

i32 TableLib::getTableLength(GCRef<Table> table) {
    if (!table) {
        return 0;
    }

    // Simple implementation: count consecutive numeric indices starting from 1
    i32 length = 0;
    while (true) {
        Value key = Value(static_cast<f64>(length + 1));
        Value val = table->get(key);
        if (val.isNil()) {
            break;
        }
        ++length;
    }

    return length;
}

GCRef<Table> TableLib::validateTableArg(LuaState* state, i32 argIndex) {
    if (!state) {
        return GCRef<Table>(nullptr);
    }

    Value val = state->get(argIndex);
    if (!val.isTable()) {
        return GCRef<Table>(nullptr);
    }

    return val.asTable();
}

void initializeTableLib(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    TableLib tableLib;
    tableLib.registerFunctions(state);
    tableLib.initialize(state);
}

} // namespace Lua
