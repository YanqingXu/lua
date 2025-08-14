#pragma once

#include "../common/types.hpp"
#include <functional>
#include <string>

namespace Lua {
    // Forward declarations
    class LuaState;
    struct lua_State;
    
    /**
     * @brief Lua 5.1 Compatible Debug Hooks System
     * 
     * This system provides complete compatibility with Lua 5.1's debugging
     * infrastructure, including hook masks, debug events, and debug information
     * collection as specified in the official Lua 5.1 reference manual.
     */
    
    // Lua 5.1 Debug Hook Masks (official constants)
    constexpr i32 LUA_MASKCALL = 1;     // Hook on function calls
    constexpr i32 LUA_MASKRET = 2;      // Hook on function returns  
    constexpr i32 LUA_MASKLINE = 4;     // Hook on line execution
    constexpr i32 LUA_MASKCOUNT = 8;    // Hook on instruction count
    
    // Lua 5.1 Debug Event Types (official constants)
    constexpr i32 LUA_HOOKCALL = 0;     // Function call event
    constexpr i32 LUA_HOOKRET = 1;      // Function return event
    constexpr i32 LUA_HOOKLINE = 2;     // Line execution event
    constexpr i32 LUA_HOOKCOUNT = 3;    // Instruction count event
    constexpr i32 LUA_HOOKTAILRET = 4;  // Tail return event
    
    /**
     * @brief Lua 5.1 Compatible Debug Information Structure
     * 
     * This structure matches the official lua_Debug structure from Lua 5.1
     * and provides all the debugging information available in the official API.
     */
    struct lua_Debug {
        i32 event;                      // Debug event type (LUA_HOOKCALL, etc.)
        const char* name;               // Function name (if available)
        const char* namewhat;           // Type of name ("global", "local", "method", etc.)
        const char* what;               // Function type ("Lua", "C", "main", "tail")
        const char* source;             // Source file name
        i32 currentline;                // Current line number
        i32 nups;                       // Number of upvalues
        i32 linedefined;                // Line where function is defined
        i32 lastlinedefined;            // Last line of function definition
        char short_src[60];             // Short source description
        
        // Internal fields (not part of public API)
        i32 i_ci;                       // Call info index (internal)
        
        lua_Debug() : event(0), name(nullptr), namewhat(nullptr), what(nullptr),
                     source(nullptr), currentline(-1), nups(0), linedefined(-1),
                     lastlinedefined(-1), i_ci(0) {
            short_src[0] = '\0';
        }
    };
    
    /**
     * @brief Lua 5.1 Compatible Hook Function Type
     * 
     * This typedef matches the official lua_Hook function signature from Lua 5.1.
     */
    typedef void (*lua_Hook) (lua_State *L, lua_Debug *ar);
    
    /**
     * @brief Debug Hook Manager Class
     * 
     * Manages debug hooks and provides the complete Lua 5.1 debugging API
     * including hook registration, event triggering, and debug information collection.
     */
    class DebugHookManager {
    public:
        DebugHookManager(LuaState* state);
        ~DebugHookManager();
        
        // Lua 5.1 Compatible Debug Hook API
        /**
         * @brief Set debug hook function with mask and count
         * @param func Hook function (lua_Hook)
         * @param mask Hook mask (combination of LUA_MASK* constants)
         * @param count Instruction count for LUA_MASKCOUNT
         */
        void setHook(lua_Hook func, i32 mask, i32 count);
        
        /**
         * @brief Get current hook function
         * @return lua_Hook Current hook function or nullptr
         */
        lua_Hook getHook() const { return currentHook_; }
        
        /**
         * @brief Get current hook mask
         * @return i32 Current hook mask
         */
        i32 getHookMask() const { return hookMask_; }
        
        /**
         * @brief Get current hook count
         * @return i32 Current hook count
         */
        i32 getHookCount() const { return hookCount_; }
        
        // Debug Event Triggering
        /**
         * @brief Trigger debug hook for function call
         * @param func Function being called
         */
        void triggerCallHook(const char* funcName = nullptr);
        
        /**
         * @brief Trigger debug hook for function return
         * @param func Function returning
         */
        void triggerReturnHook(const char* funcName = nullptr);
        
        /**
         * @brief Trigger debug hook for line execution
         * @param line Line number being executed
         * @param source Source file name
         */
        void triggerLineHook(i32 line, const char* source = nullptr);
        
        /**
         * @brief Trigger debug hook for instruction count
         */
        void triggerCountHook();
        
        /**
         * @brief Check if hook should be triggered for given mask
         * @param mask Hook mask to check
         * @return bool True if hook should be triggered
         */
        bool shouldTriggerHook(i32 mask) const;
        
        // Debug Information Collection
        /**
         * @brief Get debug information for current execution context
         * @param ar Debug information structure to fill
         * @param what Information to retrieve ("nSltu" format)
         * @return bool True if information was retrieved successfully
         */
        bool getInfo(lua_Debug* ar, const char* what);
        
        /**
         * @brief Get debug information for stack level
         * @param level Stack level (0 = current function)
         * @param ar Debug information structure to fill
         * @return bool True if information was retrieved successfully
         */
        bool getStack(i32 level, lua_Debug* ar);
        
        /**
         * @brief Get local variable information
         * @param ar Debug information structure
         * @param n Local variable index
         * @return const char* Variable name or nullptr
         */
        const char* getLocal(lua_Debug* ar, i32 n);
        
        /**
         * @brief Set local variable value
         * @param ar Debug information structure
         * @param n Local variable index
         * @return const char* Variable name or nullptr
         */
        const char* setLocal(lua_Debug* ar, i32 n);
        
        /**
         * @brief Get upvalue information
         * @param funcindex Function index on stack
         * @param n Upvalue index
         * @return const char* Upvalue name or nullptr
         */
        const char* getUpvalue(i32 funcindex, i32 n);
        
        /**
         * @brief Set upvalue value
         * @param funcindex Function index on stack
         * @param n Upvalue index
         * @return const char* Upvalue name or nullptr
         */
        const char* setUpvalue(i32 funcindex, i32 n);
        
        // Hook State Management
        /**
         * @brief Check if any hooks are active
         * @return bool True if hooks are active
         */
        bool isHookActive() const { return currentHook_ != nullptr; }
        
        /**
         * @brief Clear all hooks
         */
        void clearHooks();
        
        /**
         * @brief Update instruction counter for count hooks
         */
        void updateInstructionCounter();
        
    private:
        LuaState* state_;               // Associated Lua state
        lua_Hook currentHook_;          // Current hook function
        i32 hookMask_;                  // Current hook mask
        i32 hookCount_;                 // Hook count for LUA_MASKCOUNT
        i32 instructionCounter_;        // Current instruction counter
        
        // Internal helper methods
        void initializeDebugSystem_();
        void cleanupDebugSystem_();
        void fillDebugInfo_(lua_Debug* ar, const char* what);
        void fillSourceInfo_(lua_Debug* ar, const char* source);
        void fillFunctionInfo_(lua_Debug* ar, const char* funcName);
        bool isValidStackLevel_(i32 level) const;
        
        // Debug information formatting
        void formatShortSource_(lua_Debug* ar, const char* source);
        const char* getFunctionType_(const char* funcName);
        const char* getNameType_(const char* funcName);
    };
    
    // Global debug utilities
    /**
     * @brief Create formatted debug message
     * @param event Debug event type
     * @param ar Debug information
     * @return std::string Formatted debug message
     */
    std::string formatDebugMessage(i32 event, const lua_Debug* ar);
    
    /**
     * @brief Convert debug event to string
     * @param event Debug event type
     * @return const char* Event name
     */
    const char* debugEventToString(i32 event);
    
    /**
     * @brief Convert hook mask to string
     * @param mask Hook mask
     * @return std::string Mask description
     */
    std::string hookMaskToString(i32 mask);
    
} // namespace Lua
