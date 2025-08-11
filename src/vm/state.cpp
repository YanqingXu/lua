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

    // Unified stack pointer access: 0-based absolute index when idx >= 0,
    // negative idx means relative to current top (top + idx)
    Value* State::getPtr(int idx) {
        int abs_idx;
        if (idx >= 0) {
            abs_idx = idx;  // 0-based absolute indexing
        } else {
            abs_idx = top + idx;  // Index relative to stack top
        }
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
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("attempt to call a nil value");
                }

                // Legacy function call - return single value
                Value result = nativeFnLegacy(this, static_cast<int>(args.size()));
                return result;
            } else {
                // New multi-return function - call and return first value for compatibility
                CallResult callResult = callMultiple(func, args);
                if (callResult.count > 0) {
                    return callResult.getFirst();
                } else {
                    return Value();  // Return nil if no values
                }
            }
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
            std::cerr << "LuaException in call: " << e.what() << std::endl;
            // CRITICAL FIX: Always re-throw LuaException for proper pcall error handling
            // pcall should be the only place that catches and converts exceptions to return values
            throw; // Re-throw all LuaExceptions to allow pcall to handle them properly
        }
    }

    CallResult State::callMultiple(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Native function call with multiple return values support
        if (function->getType() == Function::Type::Native) {
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("attempt to call a nil value");
                }

                // Legacy function call - convert to single return value
                Value result = nativeFnLegacy(this, static_cast<int>(args.size()));
                return CallResult(result);
            } else {
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

                // Call new multi-return function
                i32 returnCount = nativeFn(this);

                // Collect return values from stack
                Vec<Value> results;
                for (i32 i = 0; i < returnCount; ++i) {
                    results.push_back(get(oldTop + i));
                }

                // Restore stack top
                setTop(oldTop);

                return CallResult(results);
            }
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
            vm.setActualArgsCount(static_cast<int>(args.size())); // 设置实际参数数量
            CallResult result = vm.executeMultiple(function);

            // Restore stack top after VM execution
            setTop(oldTop);

            return result;
        } catch (const LuaException& e) {
            std::cerr << "LuaException in callMultiple: " << e.what() << std::endl;
            // CRITICAL FIX: Always re-throw LuaException for proper pcall error handling
            // pcall should be the only place that catches and converts exceptions to return values
            throw; // Re-throw all LuaExceptions to allow pcall to handle them properly
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

        // Check if it's a legacy function
        if (function->isNativeLegacy()) {
            auto nativeFnLegacy = function->getNativeLegacy();
            if (!nativeFnLegacy) {
                throw LuaException("attempt to call a nil value");
            }

            // Legacy function call - return single value
            Value result = nativeFnLegacy(this, nargs);
            return result;
        } else {
            // New multi-return function - call and return first value for compatibility
            i32 returnCount = callNativeMultiple(func, nargs);
            if (returnCount > 0) {
                return get(top - returnCount);  // Return first value
            } else {
                return Value();  // Return nil if no values
            }
        }
    }

    // Native function call with multiple return values (Lua 5.1 standard)
    i32 State::callNativeMultiple(const Value& func, int nargs) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Only handle native functions
        if (function->getType() != Function::Type::Native) {
            throw LuaException("callNativeMultiple can only call native functions");
        }

        // Store the stack position before arguments (0-based absolute index)
        const int stackBase = top - nargs;
        if (stackBase < 0) {
            throw LuaException("invalid argument count for native call");
        }

        i32 returnCount = 0;

        if (function->isNativeLegacy()) {
            auto nativeFnLegacy = function->getNativeLegacy();
            if (!nativeFnLegacy) {
                throw LuaException("attempt to call a nil value");
            }
            // Legacy: returns single Value; replace args with result in place
            Value result = nativeFnLegacy(this, nargs);
            top = stackBase;    // pop args
            push(result);       // push single result
            returnCount = 1;
        } else {
            auto nativeFn = function->getNative();
            if (!nativeFn) {
                throw LuaException("attempt to call a nil value");
            }
            // Lua 5.1 style: arguments are already at [stackBase .. top-1].
            // Native pushes return values on the same State stack and returns count.
            const int oldTop = top;
            returnCount = nativeFn(this);
            if (returnCount < 0) {
                throw LuaException("native function returned invalid return count");
            }
            // Ensure the native respected the protocol; if it pushed fewer values,
            // pad with nil to keep stack shape predictable.
            while (top < oldTop + returnCount) {
                push(Value());
            }
            // Move return segment to start at stackBase
            // 目标：把 [oldTop..oldTop+returnCount-1] 搬到从 stackBase 开始
            int srcStart = oldTop;
            int dstStart = stackBase;
            if (dstStart != srcStart && returnCount > 0) {
                // Simple in-place move when dst before src; use临时缓存，避免覆盖
                Vec<Value> tmp;
                tmp.reserve(returnCount);
                for (int i = 0; i < returnCount; ++i) tmp.push_back(stack[srcStart + i]);
                top = dstStart; // truncate to stackBase
                for (int i = 0; i < returnCount; ++i) push(tmp[i]);
            } else {
                // 如果正好在原位，则不需要移动；确保 top 指向返回段末尾
                top = oldTop + returnCount;
            }
        }

        return returnCount;
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

            

            // Lua 5.1调用约定修复：
            // 当前栈状态：参数已经在栈顶：[arg1] [arg2] [arg3] ...
            // 目标栈状态：[function] [arg1] [arg2] [arg3] ...

            // 关键修复：正确计算参数在栈上的位置
            // 参数位置：从 (top - nargs) 到 (top - 1)

            // 1. 保存参数到临时数组（彻底修复索引计算）
            Vec<Value> args;
            args.reserve(nargs);
            for (int i = 0; i < nargs; ++i) {
                // 彻底修复：参数在栈上的正确位置
                // 栈顶是最后一个参数，栈底是第一个参数
                // 第i个参数的位置：top - nargs + i
                int argIndex = top - nargs + i;

                // 边界检查
                if (argIndex >= 0 && argIndex < top) {
                    args.push_back(get(argIndex));
                } else {
                    args.push_back(Value()); // nil for invalid indices
                }
            }

            // 2. 清理栈顶的参数
            setTop(top - nargs);

            // 3. 按Lua 5.1标准顺序推送：函数 + 参数
            push(func);
            for (int i = 0; i < nargs; ++i) {
                push(args[i]);
            }

            // 验证：栈布局现在是 [function] [arg1] [arg2] [arg3] ...

            // 栈布局现在是：[function] [arg1] [arg2] [arg3] ...

            // 4. 创建VM实例并执行函数
            VM vm(this);
            vm.setActualArgsCount(nargs); // 设置实际参数数量

            // Execute Lua function

            Value result = vm.execute(function);

            // 5. 恢复栈状态
            setTop(oldTop);

            return result;

        } catch (const LuaException& e) {
            std::cerr << "LuaException in callLua: " << e.what() << std::endl;
            // CRITICAL FIX: Always re-throw LuaException for proper pcall error handling
            // pcall should be the only place that catches and converts exceptions to return values
            throw; // Re-throw all LuaExceptions to allow pcall to handle them properly
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
                // Output parsing errors in Lua 5.1 format
                Str formattedErrors = parser.getFormattedErrors();
                if (!formattedErrors.empty()) {
                    std::cerr << formattedErrors << std::endl;
                }
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
                // Output parsing errors in Lua 5.1 format for REPL
                Str formattedErrors = parser.getFormattedErrors();
                if (!formattedErrors.empty()) {
                    std::cerr << formattedErrors << std::endl;
                }
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
            // Re-throw LuaException for specific Lua errors
            throw LuaException("Lua error: " + std::string(e.what()));
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
