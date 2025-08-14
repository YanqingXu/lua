#include "error_handling.hpp"
#include "lua_state.hpp"
#include <sstream>
#include <iostream>

namespace Lua {

    // ErrorHandler Implementation
    
    ErrorHandler::ErrorHandler(LuaState* state)
        : state_(state)
        , currentErrorJmp_(nullptr)
    {
        initializeErrorHandling_();
    }
    
    ErrorHandler::~ErrorHandler() {
        cleanupErrorHandling_();
    }
    
    void ErrorHandler::setErrorJmp(LuaLongJmp* jmp) {
        if (jmp) {
            jmp->previous = currentErrorJmp_;  // Stack previous error jump
            currentErrorJmp_ = jmp;
        }
    }
    
    void ErrorHandler::clearErrorJmp() {
        if (currentErrorJmp_) {
            LuaLongJmp* previous = currentErrorJmp_->previous;
            currentErrorJmp_ = previous;  // Restore previous error jump
        }
    }
    
    [[noreturn]] void ErrorHandler::throwError(i32 status, const char* msg) {
        throwError(status, std::string(msg ? msg : "unknown error"));
    }
    
    [[noreturn]] void ErrorHandler::throwError(i32 status, const std::string& msg) {
        // Format error message with context
        std::string formattedMsg = formatError(msg);

        // Push error message to Lua stack for error handlers
        pushErrorMessage(formattedMsg);

        // Create and throw appropriate exception
        auto exception = createLuaException(status, formattedMsg);

        // Store exception in current error jump if available
        if (currentErrorJmp_) {
            try {
                throw *exception;
            } catch (...) {
                currentErrorJmp_->exception = std::current_exception();
                currentErrorJmp_->status = status;
            }
        }

        // Throw the exception
        throw *exception;
    }
    
    i32 ErrorHandler::handleException(const std::exception& e) {
        // Convert exception to Lua error code
        i32 errorCode = exceptionToLuaError(e);
        
        // Push error message to stack
        pushErrorMessage(e.what());
        
        return errorCode;
    }
    
    std::string ErrorHandler::formatError(const std::string& msg, i32 line, const char* source) {
        std::ostringstream oss;
        
        // Add source information if available
        if (source && *source) {
            oss << source;
            if (line > 0) {
                oss << ":" << line;
            }
            oss << ": ";
        } else if (line > 0) {
            oss << "line " << line << ": ";
        }
        
        // Add the main error message
        oss << msg;
        
        return oss.str();
    }
    
    void ErrorHandler::pushErrorMessage(const std::string& msg) {
        if (state_) {
            // Push error message as string to Lua stack
            state_->pushString(msg.c_str());
        }
    }
    
    void ErrorHandler::initializeErrorHandling_() {
        // Initialize error handling system
        // In a full implementation, this might set up signal handlers
        // or other error recovery mechanisms
    }
    
    void ErrorHandler::cleanupErrorHandling_() {
        // Clean up error handling resources
        currentErrorJmp_ = nullptr;
    }
    
    // Global utility functions implementation
    
    std::unique_ptr<LuaRuntimeException> createLuaException(i32 code, const std::string& message) {
        switch (code) {
            case 4: // LUA_ERRMEM
                return std::make_unique<LuaMemoryException>(message);
            case 3: // LUA_ERRSYNTAX
                return std::make_unique<LuaSyntaxException>(message);
            case 5: // LUA_ERRERR
                return std::make_unique<LuaErrorHandlerException>(message);
            case 2: // LUA_ERRRUN
            default:
                return std::make_unique<LuaRuntimeException>(message, code);
        }
    }
    
    i32 exceptionToLuaError(const std::exception& e) {
        // Try to cast to specific Lua exception types
        if (const auto* luaEx = dynamic_cast<const LuaRuntimeException*>(&e)) {
            return luaEx->getErrorCode();
        }
        
        // Check for standard C++ exceptions
        if (dynamic_cast<const std::bad_alloc*>(&e)) {
            return 4; // LUA_ERRMEM
        }

        // Default to runtime error
        return 2; // LUA_ERRRUN
    }

} // namespace Lua
