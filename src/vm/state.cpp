#include "state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include "../parser/parser.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/vm.hpp"
#include "../gc/core/garbage_collector.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

namespace Lua {
    State::State() : GCObject(GCObjectType::State, sizeof(State)), top(0), currentVM(nullptr) {
        // Initialize stack space
        stack.resize(LUAI_MAXSTACK);

        // Initialize persistent VM for REPL sessions
        persistentVM = std::make_unique<VM>(this);
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
        if (idx >= 0) {
            abs_idx = idx;  // Direct 0-based indexing
        } else {
            abs_idx = top + idx;  // Index relative to stack top
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
        if (idx >= 0) {
            abs_idx = idx;  // Direct 0-based indexing
        } else {
            abs_idx = top + idx;  // Index relative to stack top
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

    Value* State::getPtr(int idx) {
        // Handle absolute and relative indices
        int abs_idx;
        if (idx > 0) {
            abs_idx = idx - 1;  // Convert 1-based to 0-based
        } else if (idx < 0) {
            abs_idx = top + idx;  // Index relative to stack top
        } else {
            // Index 0 is invalid
            return nullptr;
        }

        // Check bounds
        if (abs_idx < 0 || abs_idx >= top) {
            return nullptr;
        }

        return &stack[abs_idx];
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

    GCRef<Table> State::toTable(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asTable();
    }

    GCRef<Function> State::toFunction(int idx) {
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

    // Function call (Lua 5.1官方设计)
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

            // 使用传统的参数传递方式（保持兼容性）
            // Save current stack top
            int oldTop = top;

            // Push arguments onto stack
            for (size_t i = 0; i < args.size(); ++i) {
                push(args[i]);
            }

            // Call function
            Value result = nativeFn(this, static_cast<int>(args.size()));

            // Restore stack top
            setTop(oldTop);

            return result;
        }

        // For Lua function calls, use VM to execute
        try {
            // Save current state
            int oldTop = top;

            // Lua 5.1 calling convention:
            // Stack layout: [function] [arg1] [arg2] [arg3] ...
            // Register 0 = function, Register 1 = arg1, Register 2 = arg2, etc.

            // Push function onto stack first
            push(Value(function));

            // Push arguments onto stack
            for (const auto& arg : args) {
                push(arg);
            }

            // Create VM instance and execute function
            VM vm(this);
            Value result = vm.execute(function);

            // Restore stack top after VM execution
            setTop(oldTop);

            return result;
        } catch (const LuaException& e) {
            // 只有在特定情况下才返回nil，其他情况应该重新抛出异常
            std::string errorMsg = e.what();

            // 如果是运行时错误（如nil值操作），应该重新抛出
            if (errorMsg.find("attempt to") != std::string::npos) {
                throw; // 重新抛出异常
            }

            // 其他情况下返回nil（类似pcall的行为）
            return Value(nullptr);
        }
    }

    CallResult State::callMultiple(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Native function call - for now, native functions only return single values
        // TODO: Extend native function interface to support multiple return values
        if (function->getType() == Function::Type::Native) {
            auto nativeFn = function->getNative();
            if (!nativeFn) {
                throw LuaException("attempt to call a nil value");
            }

            // Save current stack top
            int oldTop = top;

            // Push arguments onto stack
            for (size_t i = 0; i < args.size(); ++i) {
                push(args[i]);
            }

            // Call function (single return value for now)
            Value result = nativeFn(this, static_cast<int>(args.size()));

            // Restore stack top
            setTop(oldTop);

            return CallResult(result);
        }

        // For Lua function calls, use VM to execute with multiple return values
        try {
            // Save current state
            int oldTop = top;

            // Lua 5.1 calling convention:
            // Stack layout: [function] [arg1] [arg2] [arg3] ...
            // Register 0 = function, Register 1 = arg1, Register 2 = arg2, etc.

            // Push function onto stack first
            push(Value(function));

            // Push arguments onto stack
            for (const auto& arg : args) {
                push(arg);
            }

            // Create VM instance and execute function with multiple return values
            VM vm(this);
            CallResult result = vm.executeMultiple(function);

            // Restore stack top after VM execution
            setTop(oldTop);

            return result;
        } catch (const LuaException& e) {
            // Handle errors similar to single-value call
            std::string errorMsg = e.what();

            if (errorMsg.find("attempt to") != std::string::npos) {
                throw; // Re-throw runtime errors
            }

            // Return nil for other errors
            return CallResult(Value());
        }
    }

    Value State::callSafe(const Value& func, const Vec<Value>& args) {
        if (currentVM) {
            // We are in VM execution context - use in-context call
            if (!func.isFunction()) {
                throw LuaException("attempt to call a non-function value");
            }

            auto function = func.asFunction();
            return currentVM->executeInContext(function, args);
        } else {
            // We are in top-level context - safe to create new VM
            return call(func, args);
        }
    }

    CallResult State::callSafeMultiple(const Value& func, const Vec<Value>& args) {
        if (currentVM) {
            // We are in VM execution context - use in-context call
            if (!func.isFunction()) {
                throw LuaException("attempt to call a non-function value");
            }

            auto function = func.asFunction();
            return currentVM->executeInContextMultiple(function, args);
        } else {
            // We are in top-level context - safe to create new VM
            return callMultiple(func, args);
        }
    }

    // Native function call with arguments already on stack (Lua 5.1 design)
    Value State::callNative(const Value& func, int nargs) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Only handle native functions
        if (function->getType() != Function::Type::Native) {
            throw LuaException("callNative can only call native functions");
        }

        auto nativeFn = function->getNative();
        if (!nativeFn) {
            throw LuaException("attempt to call a nil value");
        }

        // Lua 5.1官方设计：参数已经在栈顶，直接调用
        // Native函数会使用State::get(stackTop - nargs + 1 + i)来访问参数
        Value result = nativeFn(this, nargs);

        return result;
    }

    // Lua function call with arguments already on stack
    Value State::callLua(const Value& func, int nargs) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Only handle Lua functions
        if (function->getType() != Function::Type::Lua) {
            throw LuaException("callLua can only call Lua functions");
        }

        // Lua 5.1官方函数调用实现
        try {
            // 保存当前栈状态
            int oldTop = top;

            

            // Lua 5.1调用约定：
            // 参数已经在栈顶：[arg1] [arg2] [arg3] ...
            // 我们需要重新排列为：[function] [arg1] [arg2] [arg3] ...

            // 1. 先扩展栈顶，为函数腾出空间
            push(Value(nullptr));  // 临时占位

            // 2. 将参数向后移动一位
            for (int i = nargs - 1; i >= 0; i--) {
                Value arg = get(top - nargs - 1 + i);  // 从原位置读取
                set(top - nargs + i, arg);             // 写入新位置
            }

            // 3. 将函数放在最前面
            set(top - nargs - 1, func);

            // 栈布局现在是：[function] [arg1] [arg2] [arg3] ...

            // 4. 创建VM实例并执行函数
            VM vm(this);
            vm.setActualArgsCount(nargs); // 设置实际参数数量
            Value result = vm.execute(function);

            // 5. 恢复栈状态
            setTop(oldTop);

            return result;

        } catch (const LuaException& e) {
            // 只有在特定情况下才返回nil，其他情况应该重新抛出异常
            std::string errorMsg = e.what();

            // 如果是运行时错误（如nil值操作），应该重新抛出
            if (errorMsg.find("attempt to") != std::string::npos) {
                throw; // 重新抛出异常
            }

            // 其他情况下返回nil（类似pcall的行为）
            return Value(nullptr);
        }
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
            GCRef<Function> function = compiler.compile(statements);

            if (!function) {
                return false;
            }

            // 3. Execute bytecode using persistent VM to maintain state continuity
            Value result = persistentVM->execute(function);  // Get return value but don't use it for doString

            return true;
        } catch (const LuaException& e) {
            // Can handle or log errors here
            std::cerr << "Lua error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            // Handle other exceptions that may occur
            std::cerr << "Error executing Lua code: " << e.what() << std::endl;
            return false;
        }
    }

    // Execute Lua code from string and return result (for REPL)
    Value State::doStringWithResult(const Str& code) {
        try {
            // 1. Parse code using our parser
            Parser parser(code);
            auto statements = parser.parse();

            // Check if there are errors in parsing phase
            if (parser.hasError()) {
                throw LuaException("Parse error");
            }

            // 2. Generate bytecode using compiler
            Compiler compiler;
            GCRef<Function> function = compiler.compile(statements);

            if (!function) {
                throw LuaException("Compile error");
            }

            // 3. Execute bytecode using persistent VM to maintain state continuity
            return persistentVM->execute(function);

        } catch (const LuaException& e) {
            throw; // Re-throw Lua exceptions
        } catch (const std::exception& e) {
            throw LuaException("Error executing Lua code: " + std::string(e.what()));
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
            std::cerr << "Error reading file '" << filename << "': " << e.what() << std::endl;
            return false;
        }
    }

    // GCObject virtual function implementations
    void State::markReferences(GarbageCollector* gc) {
        // Mark all values on the stack
        for (const Value& value : stack) {
            value.markReferences(gc);
        }

        // Mark all global variables
        for (const auto& pair : globals) {
            pair.second.markReferences(gc);
        }
    }

    usize State::getSize() const {
        return sizeof(State);
    }

    usize State::getAdditionalSize() const {
        // Calculate additional memory used by vectors and maps
        usize stackSize = stack.capacity() * sizeof(Value);
        usize globalsSize = globals.size() * (sizeof(Str) + sizeof(Value));
        return stackSize + globalsSize;
    }
}
