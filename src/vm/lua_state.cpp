#include "lua_state.hpp"
#include "global_state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../common/defines.hpp"
#include <algorithm>
#include <cstring>

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
        // Increment call info
        if (++ci_ == end_ci_) {
            reallocCI_(size_ci_ * 2);
        }
        
        // Set up call info
        ci_->func = func;
        ci_->base = func + 1;  // Arguments start after function
        ci_->top = top_;
        ci_->nresults = nresults;
        ci_->tailcalls = 0;
        ci_->savedpc = savedpc_;
        ci_->callstatus = 0;
        
        // Set current base
        base_ = ci_->base;
    }
    
    void LuaState::postcall(Value* firstResult) {
        CallInfo* ci = ci_;
        Value* res = ci->func;  // Results go where function was
        i32 nresults = ci->nresults;
        
        // Move results to proper position
        if (nresults != 0) {
            i32 nres = static_cast<i32>(top_ - firstResult);
            if (nresults > 0) {
                nres = std::min(nres, nresults);
            }
            
            // Copy results
            for (i32 i = 0; i < nres; i++) {
                res[i] = firstResult[i];
            }
            
            // Fill remaining with nil if needed
            while (nres < nresults) {
                res[nres++] = Value();
            }
            
            top_ = res + nres;
        } else {
            top_ = res;
        }
        
        // Restore previous call info
        ci_--;
        if (ci_ >= base_ci_) {
            base_ = ci_->base;
            savedpc_ = ci_->savedpc;
        }
    }
    
    void LuaState::call(i32 nargs, i32 nresults) {
        Value* func = top_ - nargs - 1;
        
        // Prepare call
        precall(func, nresults);
        
        // For now, this is a simplified implementation
        // In a full implementation, this would dispatch to VM execution
        // or call C functions directly
        
        // Simulate function execution result
        top_ = base_ + nresults;
        for (i32 i = 0; i < nresults; i++) {
            *(base_ + i) = Value();  // Return nil values for now
        }
        
        // Post-call cleanup
        postcall(base_);
    }
    
    void LuaState::setGlobal(const String* name, const Value& val) {
        // This is a simplified implementation
        // In a full implementation, this would use the global table
        // For now, we'll throw an exception to indicate it's not implemented
        throw LuaException("setGlobal not fully implemented yet");
    }
    
    Value LuaState::getGlobal(const String* name) {
        // This is a simplified implementation
        // In a full implementation, this would use the global table
        // For now, return nil
        return Value();
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
    
} // namespace Lua
