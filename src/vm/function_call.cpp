#include "function_call.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include <algorithm>
#include <iostream>

namespace Lua {
namespace FunctionCall {
    
    // Main call interface
    CallResult call(LuaState* L, Value* func, i32 nargs, i32 nresults) {
        if (!validateCallParameters(L, func, nargs, nresults)) {
            return handleError(L, "Invalid call parameters");
        }
        
        // Try fast path first
        if (canUseFastPath(L, func)) {
            CallResult result = fastCall(L, func, nargs, nresults);
            if (result.isSuccess() || result.isYielded()) {
                return result;
            }
            // Fall through to slow path on error
        }
        
        // Slow path: full precall/postcall processing
        i32 precallResult = precall(L, func, nresults);
        if (precallResult < 0) {
            return handleError(L, "Precall failed");
        }
        
        if (precallResult == 1) {
            // Lua function: VM will execute it
            // For now, just simulate successful execution
            postcall(L, L->index2addr(L->getTop() - nresults), nresults);
            return CallResult(CallStatus::Success, nresults);
        } else {
            // C function: already executed in precall
            return CallResult(CallStatus::Success, nresults);
        }
    }
    
    CallResult pcall(LuaState* L, Value* func, i32 nargs, i32 nresults, i32 errfunc) {
        try {
            return call(L, func, nargs, nresults);
        } catch (const LuaException& e) {
            return handleError(L, e.what(), errfunc);
        } catch (const std::exception& e) {
            return handleError(L, Str("C++ exception: ") + e.what(), errfunc);
        } catch (...) {
            return handleError(L, "Unknown exception", errfunc);
        }
    }
    
    // Precall processing
    i32 precall(LuaState* L, Value* func, i32 nresults) {
        if (func->isFunction()) {
            return precallLua(L, func, nresults);
        } else {
            // Try __call metamethod or treat as error
            return precallMetamethod(L, func, nresults);
        }
    }
    
    i32 precallLua(LuaState* L, Value* func, i32 nresults) {
        // Get function object
        // For now, create a simple implementation
        
        // TODO: Push new call frame (not implemented yet)
        CallInfo* ci = L->getCurrentCI();
        if (!ci) {
            return -1;
        }
        
        // Set up call frame
        ci->func = func;
        ci->base = func + 1;  // Arguments start after function
        ci->top = L->index2addr(L->getTop());
        ci->nresults = nresults;
        ci->setLua();
        ci->setFresh();
        
        // TODO: Set up function-specific information
        // ci->savedpc = function->code;
        
        return 1;  // Lua function
    }
    
    i32 precallC(LuaState* L, Value* func, i32 nresults) {
        // Get C function pointer
        // For now, create a simple implementation
        
        // TODO: Push new call frame (not implemented yet)
        CallInfo* ci = L->getCurrentCI();
        if (!ci) {
            return -1;
        }

        // Set up call frame
        ci->func = func;
        ci->base = func + 1;  // Arguments start after function
        ci->top = L->index2addr(L->getTop());
        ci->nresults = nresults;
        ci->clearFresh();  // C functions are not "fresh" Lua calls
        
        // TODO: Execute C function
        // i32 result = cfunction(L);
        
        // For now, simulate successful C function execution
        postcallC(L, ci->base, nresults);
        
        return 0;  // C function (already executed)
    }
    
    i32 precallMetamethod(LuaState* L, Value* func, i32 nresults) {
        // TODO: Implement __call metamethod handling
        // For now, return error
        return -1;
    }
    
    // Postcall processing
    void postcall(LuaState* L, Value* firstResult, i32 nresults) {
        CallInfo* ci = L->getCurrentCI();
        if (!ci) {
            return;
        }
        
        if (ci->isLua()) {
            postcallLua(L, firstResult, nresults);
        } else {
            postcallC(L, firstResult, nresults);
        }
    }
    
    void postcallLua(LuaState* L, Value* firstResult, i32 nresults) {
        CallInfo* ci = L->getCurrentCI();
        Value* res = ci->func;  // Result storage position
        
        // Adjust number of results
        if (nresults == LUA_MULTRET) {
            nresults = static_cast<i32>(L->index2addr(L->getTop()) - firstResult);
        }

        // Copy results to correct position
        copyResults(L, res, firstResult, nresults);

        // Adjust stack top
        i32 newTop = static_cast<i32>(res - L->index2addr(0)) + nresults;
        L->setTop(newTop);

        // TODO: Pop call frame (not implemented yet)
        // L->popCallInfo();
    }
    
    void postcallC(LuaState* L, Value* firstResult, i32 nresults) {
        // Similar to Lua postcall but simpler
        postcallLua(L, firstResult, nresults);
    }
    
    // Tail call optimization
    CallResult tailcall(LuaState* L, Value* func, i32 nargs) {
        if (!canTailCall(L, func)) {
            return call(L, func, nargs, LUA_MULTRET);
        }
        
        prepareTailCall(L, func, nargs);
        
        CallInfo* ci = L->getCurrentCI();
        ci->setTail();
        ci->tailcalls++;
        
        // Execute the tail call
        return call(L, ci->func, nargs, LUA_MULTRET);
    }
    
    bool canTailCall(LuaState* L, Value* func) {
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->isLua()) {
            return false;
        }
        
        // TODO: Add more sophisticated tail call detection
        return true;
    }
    
    void prepareTailCall(LuaState* L, Value* func, i32 nargs) {
        CallInfo* ci = L->getCurrentCI();
        Value* base = ci->base;
        
        // Move function and arguments to correct position
        for (i32 i = 0; i <= nargs; i++) {
            *(base + i - 1) = *(func + i);
        }
        
        // Update call frame
        ci->func = base - 1;
        ci->top = base + nargs;
    }
    
    // Stack management helpers
    void adjustArguments(LuaState* L, Value* func, i32 nargs, i32 expectedArgs) {
        if (nargs == expectedArgs) {
            return;
        }
        
        Value* argStart = func + 1;
        
        if (nargs < expectedArgs) {
            // Fill missing arguments with nil
            for (i32 i = nargs; i < expectedArgs; i++) {
                *(argStart + i) = Value();  // Default constructor creates nil
            }
            i32 newTop = static_cast<i32>(argStart - L->index2addr(0)) + expectedArgs;
            L->setTop(newTop);
        } else {
            // Truncate extra arguments
            i32 newTop = static_cast<i32>(argStart - L->index2addr(0)) + expectedArgs;
            L->setTop(newTop);
        }
    }
    
    void adjustResults(LuaState* L, Value* firstResult, i32 actualResults, i32 expectedResults) {
        if (expectedResults == LUA_MULTRET) {
            return;  // Keep all results
        }
        
        if (actualResults < expectedResults) {
            // Fill missing results with nil
            for (i32 i = actualResults; i < expectedResults; i++) {
                *(firstResult + i) = Value();  // Default constructor creates nil
            }
        }

        // Set stack top to expected number of results
        i32 newTop = static_cast<i32>(firstResult - L->index2addr(0)) + expectedResults;
        L->setTop(newTop);
    }
    
    void copyResults(LuaState* L, Value* dest, Value* src, i32 nresults) {
        for (i32 i = 0; i < nresults; i++) {
            dest[i] = src[i];
        }
    }
    
    // Error handling
    CallResult handleError(LuaState* L, const Str& errorMsg, i32 errfunc) {
        // TODO: Implement proper error handling with error function
        std::cerr << "Function call error: " << errorMsg << std::endl;
        return CallResult(CallStatus::Error, 0, errorMsg);
    }
    
    Str createErrorMessage(Value* func, const Str& reason) {
        return "Function call failed: " + reason;
    }
    
    // Performance optimization
    CallResult fastCall(LuaState* L, Value* func, i32 nargs, i32 nresults) {
        // TODO: Implement fast path for simple calls
        // For now, fall back to slow path
        return CallResult(CallStatus::Error, 0, "Fast path not implemented");
    }
    
    bool canUseFastPath(LuaState* L, Value* func) {
        // TODO: Implement fast path detection
        return false;
    }
    
    // Integration functions
    CallResult callWithRegisterFile(RegisterFile& rf, i32 funcReg, i32 nargs, i32 nresults) {
        // TODO: Implement RegisterFile integration
        return CallResult(CallStatus::Error, 0, "RegisterFile integration not implemented");
    }
    
    CallResult callWithCallStack(LuaState* L, CallStack& cs, Value* func, i32 nargs, i32 nresults) {
        // TODO: Implement CallStack integration
        return CallResult(CallStatus::Error, 0, "CallStack integration not implemented");
    }
    
    // Debugging and diagnostics
    void dumpCallState(LuaState* L, Value* func, i32 nargs) {
        std::cout << "=== Call State Dump ===" << std::endl;
        std::cout << "Function: " << func << std::endl;
        std::cout << "Arguments: " << nargs << std::endl;
        std::cout << "Stack top: " << L->getTop() << std::endl;
        std::cout << "=======================" << std::endl;
    }
    
    bool validateCallParameters(LuaState* L, Value* func, i32 nargs, i32 nresults) {
        if (!L || !func) {
            return false;
        }
        
        if (nargs < 0 || nresults < -1) {
            return false;
        }
        
        // TODO: Add more validation
        return true;
    }
    
} // namespace FunctionCall
} // namespace Lua
