#pragma once

#include "../common/types.hpp"
#include "lua_state.hpp"
#include <exception>
#include <string>
#include <memory>

namespace Lua {
    // Forward declarations
    class LuaState;
    struct LuaLongJmp;
    
    /**
     * @brief Lua 5.1 Compatible Error Handling System
     * 
     * This system provides a C++ exception-based alternative to Lua 5.1's
     * setjmp/longjmp error recovery mechanism while maintaining compatibility
     * with the official Lua 5.1 error handling semantics.
     */
    
    // Note: Lua 5.1 error codes are defined in lua_state.hpp
    // LUA_ERRRUN, LUA_ERRSYNTAX, LUA_ERRMEM, LUA_ERRERR

    // Note: LuaLongJmp structure is defined in lua_state.hpp
    
    /**
     * @brief Enhanced Lua exception class with error context
     */
    class LuaRuntimeException : public std::runtime_error {
    public:
        LuaRuntimeException(const std::string& message, i32 error_code = LUA_ERRRUN)
            : std::runtime_error(message), errorCode_(error_code) {}
        
        i32 getErrorCode() const { return errorCode_; }
        
    private:
        i32 errorCode_;
    };
    
    /**
     * @brief Memory allocation exception (corresponds to LUA_ERRMEM)
     */
    class LuaMemoryException : public LuaRuntimeException {
    public:
        LuaMemoryException(const std::string& message)
            : LuaRuntimeException(message, LUA_ERRMEM) {}
    };
    
    /**
     * @brief Syntax error exception (corresponds to LUA_ERRSYNTAX)
     */
    class LuaSyntaxException : public LuaRuntimeException {
    public:
        LuaSyntaxException(const std::string& message)
            : LuaRuntimeException(message, LUA_ERRSYNTAX) {}
    };
    
    /**
     * @brief Error handler error exception (corresponds to LUA_ERRERR)
     */
    class LuaErrorHandlerException : public LuaRuntimeException {
    public:
        LuaErrorHandlerException(const std::string& message)
            : LuaRuntimeException(message, LUA_ERRERR) {}
    };
    
    /**
     * @brief Main error handling manager class
     * 
     * Provides Lua 5.1 compatible error handling functionality using
     * C++ exceptions as the underlying mechanism.
     */
    class ErrorHandler {
    public:
        ErrorHandler(LuaState* state);
        ~ErrorHandler();
        
        // Lua 5.1 Compatible Error Management
        /**
         * @brief Set error jump point (equivalent to setjmp in official Lua)
         * @param jmp Error jump structure
         */
        void setErrorJmp(LuaLongJmp* jmp);
        
        /**
         * @brief Clear current error jump point
         */
        void clearErrorJmp();
        
        /**
         * @brief Throw error with specified status and message (equivalent to longjmp)
         * @param status Error status code
         * @param msg Error message
         */
        [[noreturn]] void throwError(i32 status, const char* msg);
        
        /**
         * @brief Throw error with string message
         * @param status Error status code  
         * @param msg Error message string
         */
        [[noreturn]] void throwError(i32 status, const std::string& msg);
        
        // Error Recovery and Handling
        /**
         * @brief Handle caught exception and convert to Lua error
         * @param e Caught exception
         * @return i32 Lua error code
         */
        i32 handleException(const std::exception& e);
        
        /**
         * @brief Get current error jump point
         * @return LuaLongJmp* Current error jump or nullptr
         */
        LuaLongJmp* getCurrentErrorJmp() const { return currentErrorJmp_; }
        
        /**
         * @brief Check if error handling is active
         * @return bool True if error jump is set
         */
        bool isErrorHandlingActive() const { return currentErrorJmp_ != nullptr; }
        
        // Error Message Formatting
        /**
         * @brief Format error message with location information
         * @param msg Base error message
         * @param line Line number (optional)
         * @param source Source name (optional)
         * @return std::string Formatted error message
         */
        std::string formatError(const std::string& msg, i32 line = -1, const char* source = nullptr);
        
        /**
         * @brief Push error message to Lua stack
         * @param msg Error message
         */
        void pushErrorMessage(const std::string& msg);
        
    private:
        LuaState* state_;                   // Associated Lua state
        LuaLongJmp* currentErrorJmp_;       // Current error jump point
        
        // Internal helper methods
        void initializeErrorHandling_();
        void cleanupErrorHandling_();
    };
    
    // Global error handling utilities
    /**
     * @brief Create appropriate exception based on error code
     * @param code Error code
     * @param message Error message
     * @return std::unique_ptr<LuaRuntimeException> Created exception
     */
    std::unique_ptr<LuaRuntimeException> createLuaException(i32 code, const std::string& message);
    
    /**
     * @brief Convert C++ exception to Lua error code
     * @param e Exception to convert
     * @return i32 Corresponding Lua error code
     */
    i32 exceptionToLuaError(const std::exception& e);
    
} // namespace Lua
