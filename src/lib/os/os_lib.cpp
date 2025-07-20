#include "os_lib.hpp"
#include "lib/core/lib_registry.hpp"
#include "vm/table.hpp"
#include "../../common/defines.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Lua {

void OSLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create os library table
    Value osTable = LibRegistry::createLibTable(state, "os");

    // Register OS functions (all legacy single-return)
    LibRegistry::registerTableFunctionLegacy(state, osTable, "clock", clock);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "date", date);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "difftime", difftime);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "execute", execute);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "exit", exit);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "getenv", getenv);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "remove", remove);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "rename", rename);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "setlocale", setlocale);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "time", time);
    LibRegistry::registerTableFunctionLegacy(state, osTable, "tmpname", tmpname);
}

void OSLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // OS library doesn't need special initialization
    // OS library initialized
}

// ===================================================================
// OS Function Implementations
// ===================================================================

Value OSLib::clock(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning

    std::clock_t c = std::clock();
    if (c == static_cast<std::clock_t>(-1)) {
        return Value(); // nil on error
    }

    f64 seconds = static_cast<f64>(c) / CLOCKS_PER_SEC;
    return Value(seconds);
}

Value OSLib::date(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    Str format = getDefaultDateFormat();
    std::time_t t = std::time(nullptr);

    if (nargs >= 1) {
        Value formatVal = state->get(1);
        if (formatVal.isString()) {
            format = formatVal.toString();
        }
    }

    if (nargs >= 2) {
        Value timeVal = state->get(2);
        if (timeVal.isNumber()) {
            t = static_cast<std::time_t>(timeVal.asNumber());
        }
    }

    // Special case for "*t" format - return table
    if (format == "*t") {
        return timeToTable(state, t);
    }

    return Value(formatTime(format, t));
}

Value OSLib::difftime(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    Value t1Val = state->get(1);
    Value t2Val = state->get(2);

    if (!t1Val.isNumber() || !t2Val.isNumber()) {
        return Value(); // nil - invalid arguments
    }

    std::time_t t1 = static_cast<std::time_t>(t1Val.asNumber());
    std::time_t t2 = static_cast<std::time_t>(t2Val.asNumber());

    f64 diff = std::difftime(t1, t2);
    return Value(diff);
}

Value OSLib::execute(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        // No command - return exit status of last command
        return Value(0); // Simplified: always return 0
    }

    Value commandVal = state->get(1);
    if (!commandVal.isString()) {
        return Value(); // nil - invalid command
    }

    Str command = commandVal.toString();
    i32 result = std::system(command.c_str());
    
    if (result == -1) {
        return Value(); // nil on error
    }

    return Value(static_cast<f64>(result));
}

Value OSLib::exit(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    i32 exitCode = 0;

    if (nargs >= 1) {
        Value codeVal = state->get(1);
        if (codeVal.isNumber()) {
            exitCode = static_cast<i32>(codeVal.asNumber());
        }
    }

    std::exit(exitCode);
    // This function does not return
}

Value OSLib::getenv(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    Value varVal = state->get(1);
    if (!varVal.isString()) {
        return Value(); // nil - invalid variable name
    }

    Str varName = varVal.toString();

#ifdef _WIN32
    // Use Windows secure version
    char* value = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&value, &len, varName.c_str());

    if (err == 0 && value != nullptr) {
        Str result(value);
        free(value); // Must free the allocated memory
        return Value(result);
    }

    if (value) {
        free(value);
    }
    return Value(); // nil if not found
#else
    // Use standard version for non-Windows platforms
    const char* value = std::getenv(varName.c_str());

    if (value) {
        return Value(Str(value));
    }

    return Value(); // nil if not found
#endif
}

Value OSLib::remove(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    Str filename = validateFilename(state, 1);
    if (filename.empty()) {
        return Value(); // nil - invalid filename
    }

    if (std::remove(filename.c_str()) == 0) {
        return Value(true); // success
    }

    // Return nil and error message
    return Value(); // nil on error
}

Value OSLib::rename(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        return Value(); // nil - insufficient arguments
    }

    Str oldName = validateFilename(state, 1);
    Str newName = validateFilename(state, 2);

    if (oldName.empty() || newName.empty()) {
        return Value(); // nil - invalid filenames
    }

    if (std::rename(oldName.c_str(), newName.c_str()) == 0) {
        return Value(true); // success
    }

    // Return nil and error message
    return Value(); // nil on error
}

Value OSLib::setlocale(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    Value localeVal = state->get(1);
    if (!localeVal.isString()) {
        return Value(); // nil - invalid locale
    }

    Str locale = localeVal.toString();
    i32 category = LC_ALL; // default category

    if (nargs >= 2) {
        Value categoryVal = state->get(2);
        if (categoryVal.isString()) {
            Str catStr = categoryVal.toString();
            // Simplified category mapping
            if (catStr == "collate") category = LC_COLLATE;
            else if (catStr == "ctype") category = LC_CTYPE;
            else if (catStr == "monetary") category = LC_MONETARY;
            else if (catStr == "numeric") category = LC_NUMERIC;
            else if (catStr == "time") category = LC_TIME;
        }
    }

    const char* result = std::setlocale(category, locale.c_str());
    if (result) {
        return Value(Str(result));
    }

    return Value(); // nil on error
}

Value OSLib::time(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs == 0) {
        // Return current time
        std::time_t t = std::time(nullptr);
        return Value(static_cast<f64>(t));
    }

    // Convert time table to time_t
    Value tableVal = state->get(1);
    std::time_t t = tableToTime(state, tableVal);
    
    if (t == static_cast<std::time_t>(-1)) {
        return Value(); // nil on error
    }

    return Value(static_cast<f64>(t));
}

Value OSLib::tmpname(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning

    // Generate a temporary filename
#ifdef _WIN32
    // Use Windows secure version
    char tmpBuffer[L_tmpnam];
    errno_t err = tmpnam_s(tmpBuffer, L_tmpnam);
    if (err == 0) {
        return Value(Str(tmpBuffer));
    }
    return Value(); // nil on error
#else
    // Use standard version for non-Windows platforms
    char tmpBuffer[L_tmpnam];
    if (std::tmpnam(tmpBuffer)) {
        return Value(Str(tmpBuffer));
    }
    return Value(); // nil on error
#endif
}

// ===================================================================
// Helper Functions
// ===================================================================

Value OSLib::timeToTable(State* state, std::time_t t) {
    if (!state) {
        return Value(); // nil
    }

    // In a real implementation, this would create a proper table
    // For now, return nil as we need proper table creation support
    (void)t; // Suppress unused parameter warning
    return Value(); // nil
}

std::time_t OSLib::tableToTime(State* state, const Value& tableVal) {
    if (!state || !tableVal.isTable()) {
        return static_cast<std::time_t>(-1);
    }

    // In a real implementation, this would extract time components from table
    // For now, return error
    return static_cast<std::time_t>(-1);
}

Str OSLib::formatTime(const Str& format, std::time_t t) {
    char buffer[256];

#ifdef _WIN32
    // Use Windows secure version
    std::tm tm_buf;
    errno_t err = localtime_s(&tm_buf, &t);
    if (err != 0) {
        return "";
    }
    std::tm* tm = &tm_buf;
#else
    // Use standard version for non-Windows platforms
    std::tm* tm = std::localtime(&t);
    if (!tm) {
        return "";
    }
#endif

    std::size_t result = std::strftime(buffer, sizeof(buffer), format.c_str(), tm);
    if (result == 0) {
        return "";
    }

    return Str(buffer);
}

const char* OSLib::getDefaultDateFormat() {
    return "%c"; // Default locale-specific date and time
}

Str OSLib::validateFilename(State* state, i32 argIndex) {
    if (!state) {
        return "";
    }

    Value val = state->get(argIndex);
    if (!val.isString()) {
        return "";
    }

    return val.toString();
}

Str OSLib::getSystemError(i32 errorCode) {
#ifdef _WIN32
    // Use Windows secure version
    char buffer[256];
    errno_t err = strerror_s(buffer, sizeof(buffer), errorCode);
    if (err == 0) {
        return Str(buffer);
    }
    return Str("Unknown error");
#else
    // Use standard version for non-Windows platforms
    return Str(std::strerror(errorCode));
#endif
}

void initializeOSLib(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    OSLib osLib;
    osLib.registerFunctions(state);
    osLib.initialize(state);
}

} // namespace Lua
