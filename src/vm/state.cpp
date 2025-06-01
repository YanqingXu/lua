#include "state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include "../parser/parser.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/vm.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace Lua {
    State::State() : top(0) {
        // Initialize stack space
        stack.resize(LUAI_MAXSTACK);
    }
    
    State::~State() {
        // Clean up resources
    }
    
    void State::push(const Value& value) {
        if (top >= LUAI_MAXSTACK) {
            throw LuaException("stack overflow");
        }
        stack[top++] = value;
    }
    
    Value State::pop() {
        if (top <= 0) {
            throw LuaException("stack underflow");
        }
        return stack[--top];
    }
    
    Value& State::get(int idx) {
        static Value nil;  // Static nil value for invalid index returns
        
        // Handle absolute and relative indices
        int abs_idx;
        if (idx > 0) {
            abs_idx = idx - 1;  // Convert 1-based to 0-based
        } else if (idx < 0) {
            abs_idx = top + idx;  // Index relative to stack top
        } else {
            // Index 0 is invalid
            return nil;
        }
        
        // Check if index is within range
        if (abs_idx < 0 || abs_idx >= top) {
            return nil;
        }
        
        return stack[abs_idx];
    }
    
    void State::set(int idx, const Value& value) {
        // Handle absolute and relative indices
        int abs_idx;
        if (idx > 0) {
            abs_idx = idx - 1;  // Convert 1-based to 0-based
        } else if (idx < 0) {
            abs_idx = top + idx;  // Index relative to stack top
        } else {
            // Index 0 is invalid
            return;
        }
        
        // Automatically extend stack to accommodate new index
        if (abs_idx < 0) {
            return; // invalid
        }
        if (abs_idx >= top) {
            // Need to extend top to abs_idx+1
            if (abs_idx >= LUAI_MAXSTACK) {
                throw LuaException("stack overflow");
            }
            top = abs_idx + 1;
        }
        stack[abs_idx] = value;
    }
    
    // Type checking functions
    bool State::isNil(int idx) const {
        if (idx <= 0 || idx > top) return true;
        return stack[idx - 1].isNil();
    }
    
    bool State::isBoolean(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isBoolean();
    }
    
    bool State::isNumber(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isNumber();
    }
    
    bool State::isString(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isString();
    }
    
    bool State::isTable(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isTable();
    }
    
    bool State::isFunction(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isFunction();
    }
    
    // Type conversion functions
    LuaBoolean State::toBoolean(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].asBoolean();
    }
    
    LuaNumber State::toNumber(int idx) const {
        if (idx <= 0 || idx > top) return 0.0;
        return stack[idx - 1].asNumber();
    }
    
    Str State::toString(int idx) const {
        if (idx <= 0 || idx > top) return "";
        return stack[idx - 1].toString();
    }
    
    Ptr<Table> State::toTable(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asTable();
    }
    
    Ptr<Function> State::toFunction(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asFunction();
    }
    
    // Global variable operations
    void State::setGlobal(const Str& name, const Value& value) {
        globals[name] = value;
    }
    
    Value State::getGlobal(const Str& name) {
        auto it = globals.find(name);
        if (it != globals.end()) {
            return it->second;
        }
        return Value(nullptr);  // nil
    }
    
    // Function call
    Value State::call(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }
        
        auto function = func.asFunction();
        
        // Native function call
        if (function->getType() == Function::Type::Native) {
            auto nativeFn = function->getNative();
            if (!nativeFn) {
                throw LuaException("attempt to call a nil value");
            }
            
            // Save current stack top
            int oldTop = top;
            
            // Push arguments onto stack
            for (const auto& arg : args) {
                push(arg);
            }
            
            // Call function
            Value result = nativeFn(this, static_cast<int>(args.size()));
            
            // Restore stack top
            top = oldTop;
            
            return result;
        }
        
        // For Lua function calls, implement in actual VM
        // In simplified version, we don't implement complete Lua function calls
        throw LuaException("Lua function call not implemented");
    }
    
    // Execute Lua code from string
    bool State::doString(const Str& code) {
        try {
            // 1. Parse code using our parser
            Parser parser(code);
            auto statements = parser.parse();
            
            // Check if there are errors in parsing phase
            if (parser.hasError()) {
                return false;
            }
            
            // 2. Generate bytecode using compiler
            Compiler compiler;
            Ptr<Function> function = compiler.compile(statements);
            
            if (!function) {
                return false;
            }
            
            // 3. Execute bytecode using virtual machine
            VM vm(this);
            vm.execute(function);
            
            return true;
        } catch (const LuaException& e) {
            // Can handle or log errors here
            std::cerr << "Lua error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            // Handle other exceptions that may occur
            std::cerr << "Standard error: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Load and execute Lua code from file
    bool State::doFile(const Str& filename) {
        try {
            // 1. Open specified file
            std::ifstream file(filename);
            if (!file.is_open()) {
                return false;
            }
            
            // 2. Read file content to string
            std::stringstream buffer;
            buffer << file.rdbuf();
            
            // 3. Close file
            file.close();
            
            // 4. Call doString to execute the string
            return doString(buffer.str());
        } catch (const std::exception& e) {
            // File operations may throw various exceptions
            std::cerr << "File error: " << e.what() << std::endl;
            return false;
        }
    }
}
