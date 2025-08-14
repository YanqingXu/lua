#include "io_lib.hpp"
#include "lib/core/lib_registry.hpp"
#include "vm/table.hpp"
#include "../../gc/core/gc_string.hpp"
#include "../../common/defines.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

namespace Lua {

// Static member initialization
IOLib::FileHandle* IOLib::defaultInput = nullptr;
IOLib::FileHandle* IOLib::defaultOutput = nullptr;

void IOLib::registerFunctions(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create io library table
    Value ioTable = LibRegistry::createLibTable(state, "io");

    // Register file operation functions (all legacy single-return)
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "open", open);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "close", close);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "read", read);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "write", write);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "flush", flush);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "lines", lines);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "input", input);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "output", output);
    LibRegistry::registerTableFunctionLegacy(state, ioTable, "type", type);
}

void IOLib::initialize(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Initialize default file handles
    defaultInput = new FileHandle(true);   // stdin
    defaultOutput = new FileHandle(true);  // stdout

    // Set standard file handles in io table
    auto ioStr = GCString::create("io");
    Value ioTable = state->getGlobal(ioStr);
    if (ioTable.isTable()) {
        auto table = ioTable.asTable();
        // Note: In a real implementation, we would create proper userdata for these
        // For now, we'll just set them as nil placeholders
        table->set(Value("stdin"), Value());
        table->set(Value("stdout"), Value());
        table->set(Value("stderr"), Value());
    }

    // IO library initialized
}

// ===================================================================
// File Operation Function Implementations
// ===================================================================

Value IOLib::open(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    Value filenameVal = state->get(1);
    if (!filenameVal.isString()) {
        return Value(); // nil - invalid filename
    }

    Str filename = filenameVal.toString();
    Str mode = "r"; // default mode

    if (nargs >= 2) {
        Value modeVal = state->get(2);
        if (modeVal.isString()) {
            mode = modeVal.toString();
        }
    }

    return createFileHandle(state, filename, mode);
}

Value IOLib::close(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    FileHandle* handle = nullptr;
    
    if (nargs >= 1) {
        handle = validateFileHandle(state, 1);
    } else {
        handle = defaultOutput; // close default output if no argument
    }

    if (!handle) {
        return Value(); // nil - invalid handle
    }

    if (handle->isStdio) {
        return Value(); // nil - cannot close stdio
    }

    handle->close();
    return Value(true); // success
}

Value IOLib::read(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Simplified implementation - just read from stdin for now
    (void)nargs; // Suppress unused parameter warning
    
    std::string line;
    if (std::getline(std::cin, line)) {
        return Value(line);
    }
    
    return Value(); // nil on EOF
}

Value IOLib::write(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Simplified implementation - write to stdout
    for (i32 i = 1; i <= nargs; ++i) {
        Value val = state->get(i);
        std::cout << val.toString();
    }

    return Value(true); // success
}

Value IOLib::flush(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning
    
    // Simplified implementation - flush stdout
    std::cout.flush();
    return Value(true); // success
}

Value IOLib::lines(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning
    
    // Simplified implementation - return nil for now
    // In a real implementation, this would return an iterator function
    return Value(); // nil
}

Value IOLib::input(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning
    
    // Simplified implementation - return current default input
    return Value(); // nil for now
}

Value IOLib::output(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    (void)nargs; // Suppress unused parameter warning
    
    // Simplified implementation - return current default output
    return Value(); // nil for now
}

Value IOLib::type(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    FileHandle* handle = validateFileHandle(state, 1);
    if (!handle) {
        return Value(); // nil - not a file handle
    }

    if (handle->isStdio) {
        return Value("file"); // stdio files are still "file" type
    }

    if (handle->isOpen()) {
        return Value("file");
    } else {
        return Value("closed file");
    }
}

// ===================================================================
// Helper Functions
// ===================================================================

IOLib::FileHandle::FileHandle(const Str& fname, const Str& fmode) 
    : filename(fname), mode(fmode), isStdio(false) {
    file = std::make_unique<std::fstream>();
    std::ios_base::openmode openMode = parseMode(fmode);
    file->open(fname, openMode);
}

IOLib::FileHandle::FileHandle(bool stdio) 
    : filename(stdio ? "stdin/stdout" : ""), mode(""), isStdio(stdio) {
    // For stdio, we don't create a file stream
    file = nullptr;
}

bool IOLib::FileHandle::isOpen() const {
    if (isStdio) {
        return true; // stdio is always "open"
    }
    return file && file->is_open();
}

void IOLib::FileHandle::close() {
    if (!isStdio && file && file->is_open()) {
        file->close();
    }
}

IOLib::FileHandle* IOLib::validateFileHandle(LuaState* state, i32 argIndex) {
    if (!state) {
        return nullptr;
    }

    // In a real implementation, this would check for userdata
    // For now, return nullptr as we don't have proper userdata support
    (void)argIndex; // Suppress unused parameter warning
    return nullptr;
}

Value IOLib::createFileHandle(LuaState* state, const Str& filename, const Str& mode) {
    if (!state) {
        return Value(); // nil
    }

    // In a real implementation, this would create userdata
    // For now, return nil as we don't have proper userdata support
    (void)filename; // Suppress unused parameter warning
    (void)mode;     // Suppress unused parameter warning
    return Value(); // nil
}

std::ios_base::openmode IOLib::parseMode(const Str& mode) {
    std::ios_base::openmode flags = std::ios_base::in;
    
    if (mode.find('w') != Str::npos) {
        flags = std::ios_base::out | std::ios_base::trunc;
    } else if (mode.find('a') != Str::npos) {
        flags = std::ios_base::out | std::ios_base::app;
    } else if (mode.find('r') != Str::npos) {
        flags = std::ios_base::in;
        if (mode.find('+') != Str::npos) {
            flags |= std::ios_base::out;
        }
    }
    
    if (mode.find('b') != Str::npos) {
        flags |= std::ios_base::binary;
    }
    
    return flags;
}

Str IOLib::readLine(std::fstream& file) {
    std::string line;
    if (std::getline(file, line)) {
        return line;
    }
    return "";
}

Str IOLib::readAll(std::fstream& file) {
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Str IOLib::readChars(std::fstream& file, i32 count) {
    std::string result;
    result.resize(count);
    file.read(&result[0], count);
    result.resize(file.gcount());
    return result;
}

void initializeIOLib(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    IOLib ioLib;
    ioLib.registerFunctions(state);
    ioLib.initialize(state);
}

} // namespace Lua
