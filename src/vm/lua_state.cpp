#include "lua_state.hpp"
#include "global_state.hpp"
#include "error_handling.hpp"
#include "debug_hooks.hpp"
#include "table.hpp"
#include "function.hpp"
#include "vm_executor.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../common/defines.hpp"
#include "../parser/parser.hpp"
#include "../compiler/compiler.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "../gc/core/gc_string.hpp"

namespace Lua {
    
    // UpValue implementation
    void UpValue::markReferences(GarbageCollector* gc) {
        if (v && v != &value) {
            // Mark the value if it's on the stack
            // TODO: Implement value marking through GC interface
            // gc->markValue(*v);
        } else {
            // Mark the closed value
            // TODO: Implement value marking through GC interface
            // gc->markValue(value);
        }

        // Mark next upvalue in chain
        if (next) {
            gc->markObject(next);
        }
    }
    
    // LuaState implementation
    LuaState::LuaState(GlobalState* g)
        : GCObject(GCObjectType::State, sizeof(LuaState))
        , G_(g)
        , stack_(nullptr)
        , top_(nullptr)
        , stack_last_(nullptr)
        , stacksize_(0)
        , ci_(nullptr)
        , base_ci_(nullptr)
        , end_ci_(nullptr)
        , size_ci_(0)
        , savedpc_(nullptr)
        , base_(nullptr)
        , openupval_(nullptr)
        , status_(LUA_OK)
        , nCcalls_(0)
        , errfunc_(0)
        , hook_(nullptr)
        , basehookcount_(0)
        , hookcount_(0)
        , hookmask_(0)
        , allowhook_(1)              // Allow hooks by default
        , l_gt_()                    // Initialize global table
        , env_()                     // Initialize environment table
        , gclist_(nullptr)           // Initialize GC list
        , errorJmp_(nullptr)         // Initialize error jump
    {
        initializeStack_();
        initializeCallInfo_();
    }
    
    LuaState::~LuaState() {
        cleanupCallInfo_();
        cleanupStack_();
    }
    
    void LuaState::push(const Value& val) {
        checkstack(1);
        *top_++ = val;
    }
    
    Value LuaState::pop() {
        if (top_ <= stack_) {
            throw LuaException("stack underflow");
        }
        return *--top_;
    }
    
    Value* LuaState::index2addr(i32 idx) {
        if (idx >= 0) {
            // Absolute index from stack base
            Value* addr = stack_ + idx;
            return (addr < top_) ? addr : nullptr;
        } else {
            // Relative index from stack top
            Value* addr = top_ + idx;
            return (addr >= stack_) ? addr : nullptr;
        }
    }
    
    void LuaState::checkstack(i32 n) {
        if (top_ + n > stack_last_) {
            growStack(n);
        }
    }
    
    void LuaState::setTop(i32 idx) {
        Value* newtop;
        if (idx >= 0) {
            newtop = stack_ + idx;
        } else {
            newtop = top_ + idx + 1;
        }
        
        if (newtop < stack_) {
            throw LuaException("invalid stack index");
        }
        
        // Fill with nil if expanding
        while (top_ < newtop) {
            *top_++ = Value();  // nil value
        }
        
        top_ = newtop;
    }
    
    i32 LuaState::getTop() const {
        return static_cast<i32>(top_ - stack_);
    }

    Value* LuaState::getStackBase() const {
        return stack_;
    }

    void LuaState::setTopDirect(Value* newTop) {
        if (newTop < stack_ || newTop > stack_last_) {
            throw LuaException("invalid stack top pointer");
        }
        top_ = newTop;
    }
    
    Value& LuaState::get(i32 idx) {
        Value* addr = index2addr(idx);
        if (!addr) {
            static Value nil;  // Return nil for invalid indices
            return nil;
        }
        return *addr;
    }
    
    void LuaState::set(i32 idx, const Value& val) {
        Value* addr = index2addr(idx);
        if (addr) {
            *addr = val;
        } else {
            throw LuaException("invalid stack index");
        }
    }
    
    void LuaState::precall(Value* func, i32 nresults) {
        // 保存当前 savedpc 到当前 CallInfo (L->ci->savedpc = L->savedpc;)
        if (ci_ >= base_ci_) {
            ci_->savedpc = savedpc_;
        }

        // Increment call info (ci = inc_ci(L);)
        if (++ci_ == end_ci_) {
            reallocCI_(size_ci_ * 2);
        }

        // 按照官方实现设置 CallInfo
        // ci->func = func;
        ci_->func = func;

        // 设置 base (base = func + 1; 对于非可变参数函数)
        Value* base = func + 1;

        // 检查函数类型并设置参数
        if (func->isFunction()) {
            auto function = func->asFunction();
            if (function && function->getType() == Function::Type::Lua) {
                ci_->callstatus = CallInfo::CIST_LUA;  // Mark as Lua function call

                // 按照官方实现处理参数数量
                // if (L->top > base + p->numparams) L->top = base + p->numparams;
                u8 numparams = function->getParamCount();
                if (top_ > base + numparams) {
                    top_ = base + numparams;
                }
            } else {
                ci_->callstatus = CallInfo::CIST_FRESH;
            }
        }

        // 设置 CallInfo 的其他字段
        // L->base = ci->base = base;
        base_ = ci_->base = base;
        ci_->top = top_;  // 暂时设置，可能会被调整
        ci_->nresults = nresults;
        ci_->tailcalls = 0;
        ci_->savedpc = nullptr;  // Will be set during execution

        // Debug output removed for cleaner execution
    }
    
    void LuaState::postcall(Value* firstResult) {
        // Following Lua 5.1 luaD_poscall implementation exactly
        CallInfo* ci = ci_;     // Get current CI
        Value* res = ci->func;  // Results go where function was (res == final position of 1st result)
        i32 wanted = ci->nresults;

        // 安全检查：确保不会访问无效的 CallInfo
        if (ci_ <= base_ci_) {
            // This is normal at the end of main program execution
            return;
        }

        // 先减少 ci_ 指针
        ci_--;

        // 关键修复：按照官方实现恢复 base 和 savedpc
        // L->base = (ci - 1)->base;  /* restore base */
        // L->savedpc = (ci - 1)->savedpc;  /* restore savedpc */
        base_ = ci_->base;      // 恢复到前一个 CallInfo 的 base
        savedpc_ = ci_->savedpc; // 恢复到前一个 CallInfo 的 savedpc

        // Move results to correct place (following Lua 5.1 logic)
        // for (i = wanted; i != 0 && firstResult < L->top; i--)
        //   setobjs2s(L, res++, firstResult++);
        i32 i;
        for (i = wanted; i != 0 && firstResult < top_; i--) {
            *res++ = *firstResult++;
        }

        // Fill remaining with nil if needed
        // while (i-- > 0) setnilvalue(res++);
        while (i-- > 0) {
            *res++ = Value();  // nil value
        }

        // Set new top
        // L->top = res;
        top_ = res;
    }
    

    
    void LuaState::setGlobal(const GCString* name, const Value& val) {
        // Use the global table stored in l_gt_ field
        if (l_gt_.isNil()) {
            // Initialize global table if not exists
            // Create a new table using GC allocator
            if (G_) {
                Table* table = new Table();
                auto globalTable = GCRef<Table>(table);
                l_gt_ = Value(globalTable);
            } else {
                // Fallback: create without GC (for testing)
                auto globalTable = GCRef<Table>(new Table());
                l_gt_ = Value(globalTable);
            }

            // Set _G to point to the global table itself (Lua 5.1 standard)
            if (l_gt_.isTable()) {
                auto table = l_gt_.asTable();
                auto gStr = GCString::create("_G");
                table->set(Value(gStr), l_gt_);
            }
        }

        if (l_gt_.isTable()) {
            auto table = l_gt_.asTable();

            Value keyValue(name->getString());
            table->set(keyValue, val);
        }
    }

    Value LuaState::getGlobal(const GCString* name) {
        // Get from the global table stored in l_gt_ field
        if (l_gt_.isTable()) {
            auto table = l_gt_.asTable();
            return table->get(Value(name->getString()));
        }
        return Value(); // Return nil if no global table
    }

    // Type checking operations implementation
    bool LuaState::isNil(i32 idx) {
        Value* val = index2addr(idx);
        return val ? val->isNil() : true;
    }

    bool LuaState::isBoolean(i32 idx) {
        Value* val = index2addr(idx);
        return val ? val->isBoolean() : false;
    }

    bool LuaState::isNumber(i32 idx) {
        Value* val = index2addr(idx);
        return val ? val->isNumber() : false;
    }

    bool LuaState::isString(i32 idx) {
        Value* val = index2addr(idx);
        return val ? val->isString() : false;
    }

    bool LuaState::isFunction(i32 idx) {
        Value* val = index2addr(idx);
        return val ? val->isFunction() : false;
    }
    
    void LuaState::markReferences(GarbageCollector* gc) {
        // Mark all values on the stack
        for (Value* v = stack_; v < top_; v++) {
            // TODO: Implement value marking through GC interface
            // gc->markValue(*v);
        }

        // Mark all CallInfo functions
        for (CallInfo* ci = base_ci_; ci <= ci_; ci++) {
            if (ci->func) {
                // TODO: Implement value marking through GC interface
                // gc->markValue(*ci->func);
            }
        }

        // Mark open upvalues
        for (UpValue* uv = openupval_; uv; uv = uv->next) {
            gc->markObject(uv);
        }
    }
    
    usize LuaState::getSize() const {
        return sizeof(LuaState);
    }
    
    usize LuaState::getAdditionalSize() const {
        usize size = 0;
        
        // Stack size
        size += stacksize_ * sizeof(Value);
        
        // CallInfo array size
        size += size_ci_ * sizeof(CallInfo);
        
        return size;
    }
    
    void LuaState::growStack(i32 n) {
        i32 needed = static_cast<i32>(top_ - stack_) + n;
        i32 newsize = stacksize_;
        
        // Double the size until we have enough space
        while (newsize < needed) {
            newsize *= 2;
        }
        
        // Limit maximum stack size
        if (newsize > LUAI_MAXSTACK) {
            if (needed > LUAI_MAXSTACK) {
                throw LuaException("stack overflow");
            }
            newsize = LUAI_MAXSTACK;
        }
        
        reallocStack_(newsize);
    }
    
    void LuaState::shrinkStack() {
        i32 used = static_cast<i32>(top_ - stack_);
        i32 newsize = used * 2;  // Keep some extra space
        
        if (newsize < stacksize_ / 4 && newsize >= 32) {
            reallocStack_(newsize);
        }
    }
    
    void LuaState::initializeStack_() {
        stacksize_ = 64;  // Initial stack size
        stack_ = static_cast<Value*>(G_->allocate(stacksize_ * sizeof(Value)));
        if (!stack_) {
            throw LuaException("cannot allocate stack");
        }
        
        // Initialize stack values to nil
        for (i32 i = 0; i < stacksize_; i++) {
            stack_[i] = Value();
        }
        
        top_ = stack_;
        stack_last_ = stack_ + stacksize_ - 1;
    }
    
    void LuaState::initializeCallInfo_() {
        size_ci_ = 8;  // Initial CallInfo array size
        base_ci_ = static_cast<CallInfo*>(G_->allocate(size_ci_ * sizeof(CallInfo)));
        if (!base_ci_) {
            throw LuaException("cannot allocate call info");
        }
        
        // Initialize CallInfo array
        for (i32 i = 0; i < size_ci_; i++) {
            base_ci_[i] = CallInfo();
        }
        
        ci_ = base_ci_;
        end_ci_ = base_ci_ + size_ci_;
        
        // Set up base call info
        ci_->func = nullptr;
        ci_->base = stack_;
        ci_->top = stack_;
        ci_->nresults = 0;
        ci_->callstatus = 0;
        
        base_ = stack_;
    }
    
    void LuaState::cleanupStack_() {
        if (stack_) {
            G_->deallocate(stack_);
            stack_ = nullptr;
            top_ = nullptr;
            stack_last_ = nullptr;
            stacksize_ = 0;
        }
    }
    
    void LuaState::cleanupCallInfo_() {
        if (base_ci_) {
            G_->deallocate(base_ci_);
            base_ci_ = nullptr;
            ci_ = nullptr;
            end_ci_ = nullptr;
            size_ci_ = 0;
        }
    }
    
    void LuaState::reallocStack_(i32 newsize) {
        Value* newstack = static_cast<Value*>(
            G_->reallocate(stack_, stacksize_ * sizeof(Value), newsize * sizeof(Value))
        );
        if (!newstack) {
            throw LuaException("cannot reallocate stack");
        }
        
        // Update pointers
        i32 topoffset = static_cast<i32>(top_ - stack_);
        i32 baseoffset = static_cast<i32>(base_ - stack_);
        
        stack_ = newstack;
        top_ = stack_ + topoffset;
        base_ = stack_ + baseoffset;
        stack_last_ = stack_ + newsize - 1;
        stacksize_ = newsize;
        
        // Initialize new stack space to nil
        for (i32 i = topoffset; i < newsize; i++) {
            stack_[i] = Value();
        }
    }
    
    void LuaState::reallocCI_(i32 newsize) {
        CallInfo* newci = static_cast<CallInfo*>(
            G_->reallocate(base_ci_, size_ci_ * sizeof(CallInfo), newsize * sizeof(CallInfo))
        );
        if (!newci) {
            throw LuaException("cannot reallocate call info");
        }
        
        // Update pointers
        i32 cioffset = static_cast<i32>(ci_ - base_ci_);
        
        base_ci_ = newci;
        ci_ = base_ci_ + cioffset;
        end_ci_ = base_ci_ + newsize;
        
        // Initialize new CallInfo space
        for (i32 i = size_ci_; i < newsize; i++) {
            base_ci_[i] = CallInfo();
        }
        
        size_ci_ = newsize;
    }

    // High-level execution interface (migrated from State class)
    bool LuaState::doString(const Str& code) {
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

            // 3. Execute bytecode using VMExecutor
            try {
                Vec<Value> args;  // No arguments
                VMExecutor::execute(this, function, args);
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Execution error: " << e.what() << std::endl;
                return false;
            }
        } catch (const LuaException& e) {
            std::cerr << "Lua error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Error executing Lua code: " << e.what() << std::endl;
            return false;
        }
    }

    Value LuaState::doStringWithResult(const Str& code) {
        try {
            // 1. Parse code using our parser
            Parser parser(code);
            auto statements = parser.parse();

            // Check if there are errors in parsing phase
            if (parser.hasError()) {
                return Value(); // Return nil on parse error
            }

            // 2. Generate bytecode using compiler
            Compiler compiler;
            GCRef<Function> function = compiler.compile(statements);

            if (!function) {
                return Value(); // Return nil on compile error
            }

            // 3. Execute bytecode using VMExecutor and return result
            try {
                Vec<Value> args;  // No arguments
                return VMExecutor::execute(this, function, args);
            } catch (const std::exception& e) {
                std::cerr << "Execution error: " << e.what() << std::endl;
                return Value(); // Return nil on execution error
            }
        } catch (const std::exception& e) {
            std::cerr << "Error executing Lua code: " << e.what() << std::endl;
            return Value(); // Return nil on any error
        }
    }

    bool LuaState::doFile(const Str& filename) {
        try {
            // 1. Open file
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
                return false;
            }

            // 2. Read file content
            std::ostringstream buffer;
            buffer << file.rdbuf();

            // 3. Close file
            file.close();

            // 4. Call doString to execute the string
            return doString(buffer.str());
        } catch (const std::exception& e) {
            std::cerr << "Error reading file '" << filename << "': " << e.what() << std::endl;
            return false;
        }
    }

    Value LuaState::callFunction(const Value& func, const Vec<Value>& args) {
        try {
            if (!func.isFunction()) {
                throw LuaException("attempt to call a non-function value");
            }

            auto function = func.asFunction();
            if (!function) {
                throw LuaException("invalid function object");
            }

            // Handle different function types
            if (function->getType() == Function::Type::Lua) {
                // Use VMExecutor for Lua function execution
                return VMExecutor::execute(this, function, args);
            } else if (function->getType() == Function::Type::Native) {
                // Handle native function directly
                if (function->isNativeLegacy()) {
                    // Legacy native function (single return value)
                    auto legacyFn = function->getNativeLegacy();
                    if (legacyFn) {
                        // 保存当前栈状态
                        int oldTop = getTop();

                        // 将参数推入栈中
                        for (const auto& arg : args) {
                            push(arg);
                        }

                        // 调用legacy函数
                        Value result = legacyFn(this, static_cast<i32>(args.size()));

                        // 恢复栈状态
                        setTop(oldTop);

                        return result;
                    }
                } else {
                    // Modern native function (multiple return values)
                    auto nativeFn = function->getNative();
                    if (nativeFn) {
                        // 保存当前栈状态
                        int oldTop = getTop();

                        // 将参数推入栈中
                        for (const auto& arg : args) {
                            push(arg);
                        }

                        // 调用native函数
                        i32 nresults = nativeFn(this);

                        // 获取第一个返回值（如果有的话）
                        Value result;
                        if (nresults > 0) {
                            result = get(-nresults);
                        }

                        // 恢复栈状态
                        setTop(oldTop);

                        return result;
                    }
                }

                throw LuaException("failed to call native function");
            } else {
                throw LuaException("unknown function type");
            }
        } catch (const std::exception& e) {
            std::cerr << "Function call error: " << e.what() << std::endl;
            return Value(); // Return nil on error
        }
    }

    CallResult LuaState::callMultiple(const Value& func, const Vec<Value>& args) {
        try {
            if (!func.isFunction()) {
                return CallResult(Value()); // Return single nil value
            }

            auto function = func.asFunction();
            if (!function) {
                return CallResult(Value()); // Return single nil value
            }

            // For now, use single return value - this can be enhanced later
            Value result = VMExecutor::execute(this, function, args);
            return CallResult(result);
        } catch (const std::exception& e) {
            std::cerr << "Function call error: " << e.what() << std::endl;
            return CallResult(Value()); // Return single nil value on error
        }
    }

    void LuaState::clearStack() {
        setTop(0);
    }

    // Coroutine methods implementation
    LuaCoroutine* LuaState::createCoroutine(GCRef<Function> func) {
        if (!func) {
            return nullptr;
        }

        try {
            // Create a new coroutine using the coroutine manager
            // For now, create a basic coroutine that can be resumed
            auto coroutine = std::make_unique<LuaCoroutine>(nullptr, this);
            LuaCoroutine* ptr = coroutine.get();

            // Store the function to be executed
            // In a full implementation, this would set up the coroutine to execute the function

            // Add to GC management
            if (G_ && G_->getGC()) {
                G_->getGC()->registerObject(ptr);
            }

            // Store coroutine reference (simplified management)
            // In a full implementation, this would be managed by a coroutine manager
            static Vec<UPtr<LuaCoroutine>> coroutines;
            coroutines.push_back(std::move(coroutine));

            return ptr;
        } catch (const std::exception& e) {
            std::cerr << "Error creating coroutine: " << e.what() << std::endl;
            return nullptr;
        }
    }

    CoroutineResult LuaState::resumeCoroutine(LuaCoroutine* coro, const Vec<Value>& args) {
        if (!coro) {
            return CoroutineResult(false, CoroutineStatus::DEAD);
        }

        try {
            // Basic coroutine resume implementation
            // In a full implementation, this would restore the coroutine's execution state
            // and continue from where it yielded

            CoroutineStatus status = coro->getStatus();
            if (status == CoroutineStatus::DEAD) {
                return CoroutineResult(false, CoroutineStatus::DEAD);
            }

            // For now, simulate a simple coroutine that yields some values
            // This is a basic implementation to make the coroutine library functional
            Vec<Value> results;

            if (status == CoroutineStatus::SUSPENDED) {
                // First resume - return the arguments passed to the coroutine
                results = args;
                return CoroutineResult(true, results, CoroutineStatus::SUSPENDED);
            } else {
                // Subsequent resumes - return nil and mark as dead
                results.push_back(Value()); // nil
                return CoroutineResult(true, results, CoroutineStatus::DEAD);
            }

        } catch (const std::exception& e) {
            std::cerr << "Error resuming coroutine: " << e.what() << std::endl;
            return CoroutineResult(false, CoroutineStatus::DEAD);
        }
    }

    CoroutineResult LuaState::yieldFromCoroutine(const Vec<Value>& values) {
        try {
            // Basic yield implementation
            // In a full implementation, this would save the current execution state
            // and return control to the caller

            // For now, just return the yielded values
            return CoroutineResult(true, values, CoroutineStatus::SUSPENDED);

        } catch (const std::exception& e) {
            std::cerr << "Error yielding from coroutine: " << e.what() << std::endl;
            return CoroutineResult(false, CoroutineStatus::DEAD);
        }
    }

    CoroutineStatus LuaState::getCoroutineStatus(LuaCoroutine* coro) {
        if (!coro) {
            return CoroutineStatus::DEAD;
        }

        return coro->getStatus();
    }

    // Helper method implementation
    i32 LuaState::luaIndex2StackIndex(i32 idx) {
        if (idx > 0) {
            // Positive index: 1-based from bottom
            return idx - 1;
        } else if (idx < 0) {
            // Negative index: from top
            return getTop() + idx;
        }
        return -1; // Invalid index (0)
    }

    // Lua 5.1 Compatible Stack Manipulation API Implementation

    void LuaState::pushValue(i32 idx) {
        Value val = get(idx);
        push(val);
    }

    void LuaState::remove(i32 idx) {
        i32 p = luaIndex2StackIndex(idx);
        if (p < 0) return; // Invalid index

        // Shift elements down to fill the gap
        for (i32 i = p; i < getTop() - 1; i++) {
            stack_[i] = stack_[i + 1];
        }
        setTop(getTop() - 1);
    }

    void LuaState::insert(i32 idx) {
        i32 p = luaIndex2StackIndex(idx);
        if (p < 0 || getTop() == 0) return; // Invalid index or empty stack

        Value top_val = stack_[getTop() - 1];

        // Shift elements up to make space
        for (i32 i = getTop() - 1; i > p; i--) {
            stack_[i] = stack_[i - 1];
        }

        stack_[p] = top_val;
    }

    void LuaState::replace(i32 idx) {
        if (getTop() == 0) return; // Empty stack

        Value top_val = stack_[getTop() - 1];
        setTop(getTop() - 1);
        set(idx, top_val);
    }

    // Lua 5.1 Compatible Push Functions Implementation

    void LuaState::pushNil() {
        push(Value());
    }

    void LuaState::pushNumber(f64 n) {
        push(Value(n));
    }

    void LuaState::pushInteger(i64 n) {
        push(Value(static_cast<f64>(n)));
    }

    void LuaState::pushString(const char* s) {
        if (s == nullptr) {
            pushNil();
            return;
        }
        auto str = GCString::create(s);
        push(Value(str));
    }

    void LuaState::pushLString(const char* s, usize len) {
        if (s == nullptr) {
            pushNil();
            return;
        }
        auto str = GCString::create(std::string(s, len));
        push(Value(str));
    }

    void LuaState::pushBoolean(bool b) {
        push(Value(b));
    }

    // Lua 5.1 Compatible Type Conversion Functions Implementation

    f64 LuaState::toNumber(i32 idx) {
        Value val = get(idx);
        if (val.isNumber()) {
            return val.asNumber();
        }
        if (val.isString()) {
            // Try to convert string to number (Lua 5.1 behavior)
            const char* str = val.asString().c_str();
            char* endptr;
            f64 result = std::strtod(str, &endptr);
            if (endptr != str && *endptr == '\0') {
                return result;
            }
        }
        return 0.0; // Return 0 if not convertible
    }

    i64 LuaState::toInteger(i32 idx) {
        f64 n = toNumber(idx);
        return static_cast<i64>(n);
    }

    const char* LuaState::toString(i32 idx) {
        Value val = get(idx);
        if (val.isString()) {
            return val.asString().c_str();
        }
        if (val.isNumber()) {
            // Convert number to string (Lua 5.1 behavior)
            // Note: In a full implementation, this should modify the stack
            static thread_local char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.14g", val.asNumber());
            return buffer;
        }
        return nullptr; // Return nullptr if not convertible
    }

    const char* LuaState::toLString(i32 idx, usize* len) {
        const char* str = toString(idx);
        if (str && len) {
            *len = std::strlen(str);
        }
        return str;
    }

    bool LuaState::toBoolean(i32 idx) {
        Value val = get(idx);
        // In Lua, only nil and false are false, everything else is true
        return !val.isNil() && !(val.isBoolean() && !val.asBoolean());
    }

    // Enhanced Type Checking Implementation

    bool LuaState::isCFunction(i32 idx) {
        Value val = get(idx);
        return val.isFunction() && val.asFunction()->getType() == Function::Type::Native;
    }

    bool LuaState::isUserdata(i32 idx) {
        Value val = get(idx);
        return val.isUserdata();
    }

    i32 LuaState::type(i32 idx) {
        Value val = get(idx);
        if (val.isNil()) return LUA_TNIL;
        if (val.isBoolean()) return LUA_TBOOLEAN;
        if (val.isNumber()) return LUA_TNUMBER;
        if (val.isString()) return LUA_TSTRING;
        if (val.isTable()) return LUA_TTABLE;
        if (val.isFunction()) return LUA_TFUNCTION;
        if (val.isUserdata()) return LUA_TUSERDATA;
        // Note: Thread type not yet implemented in Value class
        return LUA_TNONE;
    }

    const char* LuaState::typeName(i32 tp) {
        switch (tp) {
            case LUA_TNIL: return "nil";
            case LUA_TBOOLEAN: return "boolean";
            case LUA_TLIGHTUSERDATA: return "userdata";
            case LUA_TNUMBER: return "number";
            case LUA_TSTRING: return "string";
            case LUA_TTABLE: return "table";
            case LUA_TFUNCTION: return "function";
            case LUA_TUSERDATA: return "userdata";
            case LUA_TTHREAD: return "thread";
            default: return "no value";
        }
    }

    // Lua 5.1 Compatible Table Operations API Implementation

    void LuaState::getTable(i32 idx) {
        Value table = get(idx);
        if (!table.isTable()) {
            // Push nil if not a table
            pushNil();
            return;
        }

        Value key = get(-1); // Get key from stack top
        setTop(getTop() - 1); // Remove key from stack

        Value result = table.asTable()->get(key);
        push(result);
    }

    void LuaState::setTable(i32 idx) {
        Value table = get(idx);
        if (!table.isTable()) {
            // Pop key and value if not a table
            setTop(getTop() - 2);
            return;
        }

        Value value = get(-1); // Get value from stack top
        Value key = get(-2);   // Get key from stack top-1
        setTop(getTop() - 2);  // Remove key and value from stack

        // 使用带写屏障的set方法
        table.asTable()->setWithBarrier(key, value, this);
    }

    void LuaState::getField(i32 idx, const char* k) {
        Value table = get(idx);
        if (!table.isTable()) {
            pushNil();
            return;
        }

        auto keyStr = GCString::create(k);
        Value key(keyStr);
        Value result = table.asTable()->get(key);
        push(result);
    }

    void LuaState::setField(i32 idx, const char* k) {
        Value table = get(idx);
        if (!table.isTable()) {
            setTop(getTop() - 1); // Pop value
            return;
        }

        Value value = get(-1); // Get value from stack top
        setTop(getTop() - 1);  // Remove value from stack

        auto keyStr = GCString::create(k);
        Value key(keyStr);
        table.asTable()->set(key, value);
    }

    void LuaState::rawGet(i32 idx) {
        // Same as getTable but without metamethod calls
        getTable(idx);
    }

    void LuaState::rawSet(i32 idx) {
        // Same as setTable but without metamethod calls
        setTable(idx);
    }

    void LuaState::rawGetI(i32 idx, i32 n) {
        Value table = get(idx);
        if (!table.isTable()) {
            pushNil();
            return;
        }

        Value key(static_cast<f64>(n));
        Value result = table.asTable()->get(key);
        push(result);
    }

    void LuaState::rawSetI(i32 idx, i32 n) {
        Value table = get(idx);
        if (!table.isTable()) {
            setTop(getTop() - 1); // Pop value
            return;
        }

        Value value = get(-1); // Get value from stack top
        setTop(getTop() - 1);  // Remove value from stack

        Value key(static_cast<f64>(n));
        table.asTable()->set(key, value);
    }

    void LuaState::createTable(i32 narr, i32 nrec) {
        // Create new table with size hints
        (void)narr; // Size hints not used in current implementation
        (void)nrec;

        auto newTable = GCRef<Table>(new Table());
        push(Value(newTable));
    }

    // Lua 5.1 Compatible Function Call API Implementation

    void LuaState::call(i32 nargs, i32 nresults) {
        // Get function from stack
        Value func = get(-(nargs + 1));
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        // Prepare arguments
        Vec<Value> args;
        for (i32 i = 0; i < nargs; ++i) {
            args.push_back(get(-(nargs - i)));
        }

        // Remove function and arguments from stack
        setTop(getTop() - (nargs + 1));

        // Call function
        try {
            CallResult result = callMultiple(func.asFunction(), args);

            // Push results
            if (nresults == LUA_MULTRET) {
                // Push all results
                for (const Value& val : result.values) {
                    push(val);
                }
            } else {
                // Push specified number of results
                for (i32 i = 0; i < nresults; ++i) {
                    if (i < static_cast<i32>(result.values.size())) {
                        push(result.values[i]);
                    } else {
                        pushNil(); // Pad with nil if not enough results
                    }
                }
            }
        } catch (const std::exception& e) {
            throw LuaException(std::string("call error: ") + e.what());
        }
    }

    i32 LuaState::pcall(i32 nargs, i32 nresults, i32 errfunc) {
        try {
            call(nargs, nresults);
            return LUA_OK;
        } catch (const LuaException& e) {
            // Push error message
            pushString(e.what());
            return LUA_ERRRUN;
        } catch (const std::exception& e) {
            // Push error message
            pushString(e.what());
            return LUA_ERRRUN;
        }
    }

    i32 LuaState::cpcall(lua_CFunction func, void* ud) {
        if (!func) {
            return LUA_ERRRUN;
        }

        try {
            // Create a temporary lua_State structure for C function
            // Note: This is a simplified implementation
            struct TempLuaState {
                LuaState* L;
            } cState;
            cState.L = this;

            i32 result = func(reinterpret_cast<lua_State*>(&cState));
            return (result >= 0) ? LUA_OK : LUA_ERRRUN;
        } catch (...) {
            return LUA_ERRRUN;
        }
    }

    // Lua 5.1 Compatible Coroutine API Implementation

    i32 LuaState::yield(i32 nresults) {
        // Mark state as yielded
        status_ = LUA_YIELD;

        // In a full implementation, this would save the current execution state
        // and return control to the caller
        (void)nresults; // Placeholder

        return LUA_YIELD;
    }

    i32 LuaState::resume(i32 narg) {
        if (status_ != LUA_YIELD && status_ != LUA_OK) {
            return status_; // Cannot resume
        }

        // In a full implementation, this would restore execution state
        // and continue from where yield was called
        (void)narg; // Placeholder

        status_ = LUA_OK;
        return LUA_OK;
    }

    i32 LuaState::status() {
        return status_;
    }

    // Lua 5.1 Compatible Metatable API Implementation

    i32 LuaState::getMetatable(i32 objindex) {
        Value obj = get(objindex);

        // Get metatable based on object type
        GCRef<Table> mt = nullptr;
        if (obj.isTable()) {
            mt = obj.asTable()->getMetatable();
        } else if (obj.isUserdata()) {
            // In a full implementation, userdata would have metatables
            mt = nullptr; // Placeholder
        } else {
            // Get basic type metatable from global state
            i32 type = this->type(objindex);
            if (type >= 0 && type < 8 && G_) {
                Table* basicMt = G_->getMetaTable(type);
                if (basicMt) {
                    mt = GCRef<Table>(basicMt);
                }
            }
        }

        if (mt) {
            push(Value(mt));
            return 1;
        } else {
            return 0; // No metatable
        }
    }

    i32 LuaState::setMetatable(i32 objindex) {
        Value obj = get(objindex);
        Value mt = get(-1);
        setTop(getTop() - 1); // Remove metatable from stack

        if (obj.isTable()) {
            if (mt.isNil()) {
                obj.asTable()->setMetatable(GCRef<Table>(nullptr));
            } else if (mt.isTable()) {
                obj.asTable()->setMetatable(mt.asTable());
            } else {
                return 0; // Invalid metatable type
            }
            return 1;
        }

        // For other types, would set in global state
        return 0; // Not implemented for other types yet
    }

    void LuaState::getFenv(i32 idx) {
        Value obj = get(idx);

        if (obj.isFunction()) {
            // In a full implementation, functions would have environments
            // For now, push global table
            if (!l_gt_.isNil()) {
                push(l_gt_);
            } else {
                pushNil();
            }
        } else {
            pushNil(); // No environment for non-functions
        }
    }

    i32 LuaState::setFenv(i32 idx) {
        Value obj = get(idx);
        Value env = get(-1);
        setTop(getTop() - 1); // Remove environment from stack

        if (obj.isFunction() && env.isTable()) {
            // In a full implementation, this would set the function's environment
            // For now, just return success
            return 1;
        }

        return 0; // Failed to set environment
    }

    // Enhanced Error Handling Implementation (Phase 3)

    void LuaState::setErrorJmp(LuaLongJmp* jmp) {
        errorJmp_ = jmp;
    }

    void LuaState::clearErrorJmp() {
        errorJmp_ = nullptr;
    }

    [[noreturn]] void LuaState::throwError(i32 status, const char* msg) {
        // Simple implementation: throw standard exception
        throw LuaRuntimeException(msg ? msg : "unknown error", status);
    }

    i32 LuaState::handleException(const std::exception& e) {
        // Simple implementation: convert to Lua error code
        if (const auto* luaEx = dynamic_cast<const LuaRuntimeException*>(&e)) {
            return luaEx->getErrorCode();
        }
        if (dynamic_cast<const std::bad_alloc*>(&e)) {
            return 4; // LUA_ERRMEM
        }
        return 2; // LUA_ERRRUN (default)
    }

    // Debug Hooks System Implementation (Phase 3 - Week 9)

    void LuaState::setHook(lua_Hook func, i32 mask, i32 count) {
        // Simple implementation: store hook information
        // Full implementation will be added later
        (void)func; (void)mask; (void)count; // Suppress unused parameter warnings
    }

    lua_Hook LuaState::getHook() const {
        // Simple implementation: return nullptr
        return nullptr;
    }

    i32 LuaState::getHookMask() const {
        // Simple implementation: return 0
        return 0;
    }

    i32 LuaState::getHookCount() const {
        // Simple implementation: return 0
        return 0;
    }

    bool LuaState::getInfo(lua_Debug* ar, const char* what) {
        // Simple implementation: fill basic debug info
        if (!ar || !what) {
            return false;
        }

        // Fill basic information
        ar->event = 0;
        ar->name = "unknown";
        ar->namewhat = "global";
        ar->what = "Lua";
        ar->source = "=[C]";
        ar->currentline = 1;
        ar->nups = 0;
        ar->linedefined = -1;
        ar->lastlinedefined = -1;
        // Safe string copy
        const char* src = "=[C]";
        size_t len = strlen(src);
        size_t max_len = sizeof(ar->short_src) - 1;
        if (len > max_len) len = max_len;
        memcpy(ar->short_src, src, len);
        ar->short_src[len] = '\0';
        ar->i_ci = 0;

        return true;
    }

    bool LuaState::getStack(i32 level, lua_Debug* ar) {
        // Simple implementation: basic stack info
        if (!ar || level < 0) {
            return false;
        }

        // Fill basic stack information
        ar->i_ci = level;
        if (level == 0) {
            ar->what = "Lua";
            ar->currentline = 1;
            ar->linedefined = 1;
            ar->lastlinedefined = -1;
            ar->nups = 0;
        }

        return level < 10; // Reasonable limit
    }





} // namespace Lua
