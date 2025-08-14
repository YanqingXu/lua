#include "debug_hooks.hpp"
#include "lua_state.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>

namespace Lua {

    // DebugHookManager Implementation
    
    DebugHookManager::DebugHookManager(LuaState* state)
        : state_(state)
        , currentHook_(nullptr)
        , hookMask_(0)
        , hookCount_(0)
        , instructionCounter_(0)
    {
        initializeDebugSystem_();
    }
    
    DebugHookManager::~DebugHookManager() {
        cleanupDebugSystem_();
    }
    
    void DebugHookManager::setHook(lua_Hook func, i32 mask, i32 count) {
        currentHook_ = func;
        hookMask_ = mask;
        hookCount_ = count;
        instructionCounter_ = 0;
        
        // Reset instruction counter if count hook is enabled
        if (mask & LUA_MASKCOUNT) {
            instructionCounter_ = 0;
        }
    }
    
    void DebugHookManager::triggerCallHook(const char* funcName) {
        if (!shouldTriggerHook(LUA_MASKCALL) || !currentHook_) {
            return;
        }
        
        lua_Debug ar;
        ar.event = LUA_HOOKCALL;
        fillFunctionInfo_(&ar, funcName);
        
        // Call the hook function
        if (state_) {
            // Convert LuaState to lua_State for hook call
            lua_State* L = reinterpret_cast<lua_State*>(state_);
            currentHook_(L, &ar);
        }
    }
    
    void DebugHookManager::triggerReturnHook(const char* funcName) {
        if (!shouldTriggerHook(LUA_MASKRET) || !currentHook_) {
            return;
        }
        
        lua_Debug ar;
        ar.event = LUA_HOOKRET;
        fillFunctionInfo_(&ar, funcName);
        
        // Call the hook function
        if (state_) {
            lua_State* L = reinterpret_cast<lua_State*>(state_);
            currentHook_(L, &ar);
        }
    }
    
    void DebugHookManager::triggerLineHook(i32 line, const char* source) {
        if (!shouldTriggerHook(LUA_MASKLINE) || !currentHook_) {
            return;
        }
        
        lua_Debug ar;
        ar.event = LUA_HOOKLINE;
        ar.currentline = line;
        fillSourceInfo_(&ar, source);
        
        // Call the hook function
        if (state_) {
            lua_State* L = reinterpret_cast<lua_State*>(state_);
            currentHook_(L, &ar);
        }
    }
    
    void DebugHookManager::triggerCountHook() {
        if (!shouldTriggerHook(LUA_MASKCOUNT) || !currentHook_) {
            return;
        }
        
        // Check if instruction count threshold is reached
        if (hookCount_ > 0 && instructionCounter_ >= hookCount_) {
            lua_Debug ar;
            ar.event = LUA_HOOKCOUNT;
            
            // Reset counter
            instructionCounter_ = 0;
            
            // Call the hook function
            if (state_) {
                lua_State* L = reinterpret_cast<lua_State*>(state_);
                currentHook_(L, &ar);
            }
        }
    }
    
    bool DebugHookManager::shouldTriggerHook(i32 mask) const {
        return (hookMask_ & mask) != 0;
    }
    
    bool DebugHookManager::getInfo(lua_Debug* ar, const char* what) {
        if (!ar || !what) {
            return false;
        }
        
        fillDebugInfo_(ar, what);
        return true;
    }
    
    bool DebugHookManager::getStack(i32 level, lua_Debug* ar) {
        if (!ar || !isValidStackLevel_(level)) {
            return false;
        }
        
        // Fill basic stack information
        ar->i_ci = level;
        
        // In a full implementation, this would traverse the call stack
        // For now, provide basic information
        if (level == 0) {
            ar->what = "Lua";
            ar->currentline = 1;
            ar->linedefined = 1;
            ar->lastlinedefined = -1;
            ar->nups = 0;
        }
        
        return true;
    }
    
    const char* DebugHookManager::getLocal(lua_Debug* ar, i32 n) {
        if (!ar || n < 1) {
            return nullptr;
        }
        
        // In a full implementation, this would access local variables
        // For now, return placeholder
        static char localName[32];
        snprintf(localName, sizeof(localName), "local_%d", n);
        return localName;
    }
    
    const char* DebugHookManager::setLocal(lua_Debug* ar, i32 n) {
        if (!ar || n < 1) {
            return nullptr;
        }
        
        // In a full implementation, this would set local variable values
        // For now, return the variable name
        return getLocal(ar, n);
    }
    
    const char* DebugHookManager::getUpvalue(i32 funcindex, i32 n) {
        if (n < 1) {
            return nullptr;
        }
        
        // In a full implementation, this would access upvalues
        // For now, return placeholder
        static char upvalueName[32];
        snprintf(upvalueName, sizeof(upvalueName), "upvalue_%d", n);
        return upvalueName;
    }
    
    const char* DebugHookManager::setUpvalue(i32 funcindex, i32 n) {
        if (n < 1) {
            return nullptr;
        }
        
        // In a full implementation, this would set upvalue values
        // For now, return the upvalue name
        return getUpvalue(funcindex, n);
    }
    
    void DebugHookManager::clearHooks() {
        currentHook_ = nullptr;
        hookMask_ = 0;
        hookCount_ = 0;
        instructionCounter_ = 0;
    }
    
    void DebugHookManager::updateInstructionCounter() {
        if (hookMask_ & LUA_MASKCOUNT) {
            instructionCounter_++;
            
            // Trigger count hook if threshold reached
            if (hookCount_ > 0 && instructionCounter_ >= hookCount_) {
                triggerCountHook();
            }
        }
    }
    
    void DebugHookManager::initializeDebugSystem_() {
        // Initialize debug system
        clearHooks();
    }
    
    void DebugHookManager::cleanupDebugSystem_() {
        // Clean up debug resources
        clearHooks();
    }
    
    void DebugHookManager::fillDebugInfo_(lua_Debug* ar, const char* what) {
        if (!ar || !what) {
            return;
        }
        
        // Process each character in the 'what' string
        for (const char* p = what; *p; ++p) {
            switch (*p) {
                case 'n': // name and namewhat
                    ar->name = "unknown";
                    ar->namewhat = "global";
                    break;
                case 'S': // source, short_src, linedefined, lastlinedefined, what
                    ar->source = "=[C]";
                    ar->what = "C";
                    ar->linedefined = -1;
                    ar->lastlinedefined = -1;
                    formatShortSource_(ar, ar->source);
                    break;
                case 'l': // currentline
                    ar->currentline = 1;
                    break;
                case 't': // istailcall (Lua 5.2+, but we can ignore)
                    break;
                case 'u': // nups
                    ar->nups = 0;
                    break;
            }
        }
    }
    
    void DebugHookManager::fillSourceInfo_(lua_Debug* ar, const char* source) {
        if (!ar) {
            return;
        }
        
        ar->source = source ? source : "=[C]";
        formatShortSource_(ar, ar->source);
    }
    
    void DebugHookManager::fillFunctionInfo_(lua_Debug* ar, const char* funcName) {
        if (!ar) {
            return;
        }
        
        ar->name = funcName ? funcName : "unknown";
        ar->namewhat = getNameType_(funcName);
        ar->what = getFunctionType_(funcName);
    }
    
    bool DebugHookManager::isValidStackLevel_(i32 level) const {
        // In a full implementation, this would check actual call stack depth
        return level >= 0 && level < 100; // Reasonable limit
    }
    
    void DebugHookManager::formatShortSource_(lua_Debug* ar, const char* source) {
        if (!ar || !source) {
            return;
        }
        
        size_t len = strlen(source);
        if (len < sizeof(ar->short_src)) {
            strcpy(ar->short_src, source);
        } else {
            // Truncate and add ellipsis
            strncpy(ar->short_src, source, sizeof(ar->short_src) - 4);
            strcpy(ar->short_src + sizeof(ar->short_src) - 4, "...");
        }
    }
    
    const char* DebugHookManager::getFunctionType_(const char* funcName) {
        if (!funcName) {
            return "C";
        }
        
        // Simple heuristic: if it looks like a C function name, return "C"
        if (strstr(funcName, "_") || isupper(funcName[0])) {
            return "C";
        }
        
        return "Lua";
    }
    
    const char* DebugHookManager::getNameType_(const char* funcName) {
        if (!funcName) {
            return "";
        }
        
        // Simple heuristic for name type
        if (strstr(funcName, "::") || strstr(funcName, ".")) {
            return "method";
        }
        
        return "global";
    }
    
    // Global utility functions implementation
    
    std::string formatDebugMessage(i32 event, const lua_Debug* ar) {
        if (!ar) {
            return "Invalid debug info";
        }
        
        std::ostringstream oss;
        oss << "[DEBUG] " << debugEventToString(event);
        
        if (ar->name) {
            oss << " in " << ar->name;
        }
        
        if (ar->source && ar->currentline > 0) {
            oss << " at " << ar->short_src << ":" << ar->currentline;
        }
        
        return oss.str();
    }
    
    const char* debugEventToString(i32 event) {
        switch (event) {
            case LUA_HOOKCALL: return "CALL";
            case LUA_HOOKRET: return "RETURN";
            case LUA_HOOKLINE: return "LINE";
            case LUA_HOOKCOUNT: return "COUNT";
            case LUA_HOOKTAILRET: return "TAILRET";
            default: return "UNKNOWN";
        }
    }
    
    std::string hookMaskToString(i32 mask) {
        std::ostringstream oss;
        bool first = true;
        
        if (mask & LUA_MASKCALL) {
            if (!first) oss << "|";
            oss << "CALL";
            first = false;
        }
        if (mask & LUA_MASKRET) {
            if (!first) oss << "|";
            oss << "RET";
            first = false;
        }
        if (mask & LUA_MASKLINE) {
            if (!first) oss << "|";
            oss << "LINE";
            first = false;
        }
        if (mask & LUA_MASKCOUNT) {
            if (!first) oss << "|";
            oss << "COUNT";
            first = false;
        }
        
        return first ? "NONE" : oss.str();
    }

} // namespace Lua
