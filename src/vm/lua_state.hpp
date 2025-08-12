#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "value.hpp"
#include "call_result.hpp"
#include <memory>

namespace Lua {
    // Forward declarations
    class GlobalState;
    class GarbageCollector;
    class Table;
    class Function;
    class String;
    
    /**
     * @brief Call information structure for function calls
     *
     * This structure stores information about each function call,
     * following the Lua 5.1 CallInfo design pattern. Enhanced for
     * better performance and debugging support.
     */
    struct CallInfo {
        Value* base;                 // Function base address in stack
        Value* func;                 // Function position in stack
        Value* top;                  // Function stack top
        const u32* savedpc;          // Saved program counter (instruction pointer)
        i32 nresults;                // Expected number of return values
        i32 tailcalls;               // Number of tail calls
        u8 callstatus;               // Call status flags
        u8 extra;                    // Extra information (for alignment and future use)

        // Call status flags (following Lua 5.1 design)
        enum CallStatus : u8 {
            CIST_LUA = 1,           // Lua function call
            CIST_HOOKED = 2,        // Hook call
            CIST_REENTRY = 4,       // Re-entry call
            CIST_YIELDED = 8,       // Yielded call
            CIST_YPCALL = 16,       // Yield in pcall
            CIST_TAIL = 32,         // Tail call
            CIST_FRESH = 64         // Fresh call (not resumed)
        };

        // Helper methods for call status checking
        bool isLua() const { return callstatus & CIST_LUA; }
        bool isTail() const { return callstatus & CIST_TAIL; }
        bool isYielded() const { return callstatus & CIST_YIELDED; }
        bool isHooked() const { return callstatus & CIST_HOOKED; }
        bool isReentry() const { return callstatus & CIST_REENTRY; }
        bool isYieldedPcall() const { return callstatus & CIST_YPCALL; }
        bool isFresh() const { return callstatus & CIST_FRESH; }

        // Call status manipulation
        void setLua() { callstatus |= CIST_LUA; }
        void setTail() { callstatus |= CIST_TAIL; }
        void setYielded() { callstatus |= CIST_YIELDED; }
        void setHooked() { callstatus |= CIST_HOOKED; }
        void setReentry() { callstatus |= CIST_REENTRY; }
        void setYieldedPcall() { callstatus |= CIST_YPCALL; }
        void setFresh() { callstatus |= CIST_FRESH; }

        void clearTail() { callstatus &= ~CIST_TAIL; }
        void clearYielded() { callstatus &= ~CIST_YIELDED; }
        void clearHooked() { callstatus &= ~CIST_HOOKED; }
        void clearFresh() { callstatus &= ~CIST_FRESH; }

        // Stack management helpers
        i32 getStackSize() const { return static_cast<i32>(top - base); }
        i32 getArgCount() const { return static_cast<i32>(top - func - 1); }
        bool hasValidBase() const { return base != nullptr && func != nullptr; }
        bool hasValidTop() const { return top != nullptr && top >= base; }

        // Debugging and diagnostic helpers
        bool isValid() const {
            return hasValidBase() && hasValidTop() &&
                   nresults >= -1 && tailcalls >= 0;
        }

        void reset() {
            base = nullptr;
            func = nullptr;
            top = nullptr;
            savedpc = nullptr;
            nresults = 0;
            tailcalls = 0;
            callstatus = 0;
            extra = 0;
        }

        // Constructor
        CallInfo() : base(nullptr), func(nullptr), top(nullptr),
                    savedpc(nullptr), nresults(0), tailcalls(0),
                    callstatus(0), extra(0) {}

        // Copy constructor and assignment
        CallInfo(const CallInfo& other) = default;
        CallInfo& operator=(const CallInfo& other) = default;
    };
    
    /**
     * @brief Upvalue structure for closures
     * 
     * Represents an upvalue in Lua closures, following Lua 5.1 design.
     */
    struct UpValue : public GCObject {
        Value* v;                    // Points to stack location or to value
        Value value;                 // The value itself (when closed)
        UpValue* next;               // Next upvalue in open upvalue list
        
        UpValue() : GCObject(GCObjectType::Upvalue, sizeof(UpValue)),
                   v(&value), next(nullptr) {}
        
        // GC interface
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override { return sizeof(UpValue); }
        usize getAdditionalSize() const override { return 0; }
    };
    
    /**
     * @brief Lua thread state (per-thread execution state)
     * 
     * This class represents a single Lua thread (coroutine) state,
     * following the Lua 5.1 official design. It manages the execution
     * stack, call information, and thread-specific state.
     */
    class LuaState : public GCObject {
    private:
        GlobalState* G_;             // Global state reference
        
        // Stack management (following Lua 5.1 design)
        Value* stack_;               // Stack base address
        Value* top_;                 // Stack top pointer
        Value* stack_last_;          // Stack end position
        i32 stacksize_;              // Stack size
        
        // Call management
        CallInfo* ci_;               // Current call information
        CallInfo* base_ci_;          // CallInfo array base address
        CallInfo* end_ci_;           // CallInfo array end
        i32 size_ci_;                // CallInfo array size
        
        // Execution state
        const u32* savedpc_;         // Saved program counter
        Value* base_;                // Current function base address
        
        // Upvalue management
        UpValue* openupval_;         // Open upvalue list
        
        // Thread state information
        u8 status_;                  // Thread status (LUA_OK, LUA_YIELD, etc.)
        i32 nCcalls_;                // Number of nested C calls
        
        // Error handling
        i32 errfunc_;                // Error function index
        
        // Hook information
        void* hook_;                 // Hook function pointer
        i32 basehookcount_;          // Base hook count
        i32 hookcount_;              // Current hook count
        u8 hookmask_;                // Hook mask
        
    public:
        /**
         * @brief Construct a new Lua State object
         * @param g Global state reference
         */
        explicit LuaState(GlobalState* g);
        
        /**
         * @brief Destroy the Lua State object
         */
        ~LuaState();
        
        // Stack operations (Lua 5.1 API compatible)
        /**
         * @brief Push a value onto the stack
         * @param val Value to push
         */
        void push(const Value& val);
        
        /**
         * @brief Pop a value from the stack
         * @return Value popped value
         */
        Value pop();
        
        /**
         * @brief Convert stack index to absolute address
         * @param idx Stack index (can be negative for relative indexing)
         * @return Value* Pointer to stack element
         */
        Value* index2addr(i32 idx);
        
        /**
         * @brief Check if stack has enough space for n more elements
         * @param n Number of elements to check for
         */
        void checkstack(i32 n);
        
        /**
         * @brief Set stack top to specific index
         * @param idx New top index
         */
        void setTop(i32 idx);
        
        /**
         * @brief Get current stack top index
         * @return i32 Current top index
         */
        i32 getTop() const;
        
        /**
         * @brief Get stack element by index
         * @param idx Stack index
         * @return Value& Reference to stack element
         */
        Value& get(i32 idx);
        
        /**
         * @brief Set stack element by index
         * @param idx Stack index
         * @param val Value to set
         */
        void set(i32 idx, const Value& val);
        
        // Call management (Lua 5.1 style)
        /**
         * @brief Prepare for function call
         * @param func Function to call
         * @param nresults Expected number of results
         */
        void precall(Value* func, i32 nresults);
        
        /**
         * @brief Post-call cleanup
         * @param firstResult First result position
         */
        void postcall(Value* firstResult);
        
        /**
         * @brief Call function with specified arguments and results
         * @param nargs Number of arguments
         * @param nresults Number of expected results
         */
        void call(i32 nargs, i32 nresults);
        
        // Global variable operations
        /**
         * @brief Set global variable
         * @param name Variable name
         * @param val Value to set
         */
        void setGlobal(const String* name, const Value& val);
        
        /**
         * @brief Get global variable
         * @param name Variable name
         * @return Value Global variable value
         */
        Value getGlobal(const String* name);

        // Type checking operations (for StateAdapter compatibility)
        /**
         * @brief Check if value at index is nil
         * @param idx Stack index
         * @return bool True if value is nil
         */
        bool isNil(i32 idx);

        /**
         * @brief Check if value at index is boolean
         * @param idx Stack index
         * @return bool True if value is boolean
         */
        bool isBoolean(i32 idx);

        /**
         * @brief Check if value at index is number
         * @param idx Stack index
         * @return bool True if value is number
         */
        bool isNumber(i32 idx);

        /**
         * @brief Check if value at index is string
         * @param idx Stack index
         * @return bool True if value is string
         */
        bool isString(i32 idx);

        /**
         * @brief Check if value at index is function
         * @param idx Stack index
         * @return bool True if value is function
         */
        bool isFunction(i32 idx);
        
        // GC interface
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Accessors
        /**
         * @brief Get global state reference
         * @return GlobalState* Global state pointer
         */
        GlobalState* getGlobalState() const { return G_; }
        
        /**
         * @brief Get current call information
         * @return CallInfo* Current CallInfo pointer
         */
        CallInfo* getCurrentCI() const { return ci_; }
        
        /**
         * @brief Get thread status
         * @return u8 Thread status
         */
        u8 getStatus() const { return status_; }
        
        /**
         * @brief Set thread status
         * @param status New status
         */
        void setStatus(u8 status) { status_ = status; }
        
        // Stack size management
        /**
         * @brief Grow stack to accommodate more elements
         * @param n Number of additional elements needed
         */
        void growStack(i32 n);
        
        /**
         * @brief Shrink stack to reduce memory usage
         */
        void shrinkStack();
        
    private:
        // Internal helper methods
        void initializeStack_();
        void initializeCallInfo_();
        void cleanupStack_();
        void cleanupCallInfo_();
        void reallocStack_(i32 newsize);
        void reallocCI_(i32 newsize);
        
        // Prevent copying
        LuaState(const LuaState&) = delete;
        LuaState& operator=(const LuaState&) = delete;
    };
    
    // Thread status constants (Lua 5.1 compatible)
    constexpr u8 LUA_OK = 0;
    constexpr u8 LUA_YIELD = 1;
    constexpr u8 LUA_ERRRUN = 2;
    constexpr u8 LUA_ERRSYNTAX = 3;
    constexpr u8 LUA_ERRMEM = 4;
    constexpr u8 LUA_ERRERR = 5;
}
