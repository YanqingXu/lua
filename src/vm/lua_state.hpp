#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "value.hpp"
#include "call_result.hpp"
#include "lua_coroutine.hpp"
#include <memory>

namespace Lua {
    // Forward declarations
    class GlobalState;
    class GarbageCollector;
    class Table;
    class Function;
    class LuaCoroutine;
    class GCString;
    class ErrorHandler;
    class DebugHookManager;
    struct lua_Debug;

    // Lua 5.1 compatible type definitions
    typedef int (*lua_CFunction) (struct lua_State *L);
    typedef void (*lua_Hook) (struct lua_State *L, struct lua_Debug *ar);

    // Error handling structure (simplified C++ version of lua_longjmp)
    struct LuaLongJmp {
        std::exception_ptr exception;
        i32 status;
        LuaLongJmp* previous;
    };

    // Lua 5.1 type constants
    constexpr i32 LUA_TNONE = -1;
    constexpr i32 LUA_TNIL = 0;
    constexpr i32 LUA_TBOOLEAN = 1;
    constexpr i32 LUA_TLIGHTUSERDATA = 2;
    constexpr i32 LUA_TNUMBER = 3;
    constexpr i32 LUA_TSTRING = 4;
    constexpr i32 LUA_TTABLE = 5;
    constexpr i32 LUA_TFUNCTION = 6;
    constexpr i32 LUA_TUSERDATA = 7;
    constexpr i32 LUA_TTHREAD = 8;

    // Lua 5.1 function call constants
    constexpr i32 LUA_MULTRET = -1;     // Return all results

    // Lua 5.1 status constants
    constexpr i32 LUA_OK = 0;
    constexpr i32 LUA_YIELD = 1;
    constexpr i32 LUA_ERRRUN = 2;
    constexpr i32 LUA_ERRSYNTAX = 3;
    constexpr i32 LUA_ERRMEM = 4;
    constexpr i32 LUA_ERRERR = 5;


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

        // Helper methods for Lua 5.1 compatibility
        /**
         * @brief Convert Lua index to internal stack index
         * @param idx Lua stack index (1-based or negative)
         * @return i32 Internal stack index (0-based) or -1 if invalid
         */
        i32 luaIndex2StackIndex(i32 idx);
        
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
        
        // Hook information (Lua 5.1 compatible)
        lua_Hook hook_;              // Hook function pointer (fixed type)
        i32 basehookcount_;          // Base hook count
        i32 hookcount_;              // Current hook count
        u8 hookmask_;                // Hook mask
        u8 allowhook_;               // Allow hook flag (missing field added)

        // Global and environment tables (Lua 5.1 compatible)
        Value l_gt_;                 // Global table (TValue l_gt in official)
        Value env_;                  // Environment table temp storage (TValue env in official)

        // GC integration (Lua 5.1 compatible)
        GCObject* gclist_;           // GC list node (GCObject *gclist in official)

        // Error handling (Lua 5.1 compatible)
        struct LuaLongJmp* errorJmp_; // Error recovery point (struct lua_longjmp *errorJmp in official)

        // Debug hooks system (Phase 3 - Week 9) - simplified implementation
        // Full DebugHookManager will be implemented later
        
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
         * @brief Set stack top directly using pointer
         * @param newTop New top pointer
         */
        void setTopDirect(Value* newTop);
        
        /**
         * @brief Get current stack top index
         * @return i32 Current top index
         */
        i32 getTop() const;

        /**
         * @brief Get stack base pointer
         * @return Value* Pointer to stack base
         */
        Value* getStackBase() const;
        
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

        // Lua 5.1 Compatible Stack Manipulation API
        /**
         * @brief Push copy of element at index to top (lua_pushvalue)
         * @param idx Stack index to copy from
         */
        void pushValue(i32 idx);

        /**
         * @brief Remove element at index (lua_remove)
         * @param idx Stack index to remove
         */
        void remove(i32 idx);

        /**
         * @brief Move top element to index position (lua_insert)
         * @param idx Target position for insertion
         */
        void insert(i32 idx);

        /**
         * @brief Replace element at index with top element (lua_replace)
         * @param idx Stack index to replace
         */
        void replace(i32 idx);

        // Lua 5.1 Compatible Push Functions
        /**
         * @brief Push nil value (lua_pushnil)
         */
        void pushNil();

        /**
         * @brief Push number value (lua_pushnumber)
         * @param n Number to push
         */
        void pushNumber(f64 n);

        /**
         * @brief Push integer value (lua_pushinteger)
         * @param n Integer to push
         */
        void pushInteger(i64 n);

        /**
         * @brief Push string value (lua_pushstring)
         * @param s C string to push
         */
        void pushString(const char* s);

        /**
         * @brief Push string with length (lua_pushlstring)
         * @param s String data
         * @param len String length
         */
        void pushLString(const char* s, usize len);

        /**
         * @brief Push boolean value (lua_pushboolean)
         * @param b Boolean value to push
         */
        void pushBoolean(bool b);

        // Lua 5.1 Compatible Type Conversion Functions
        /**
         * @brief Convert stack element to number (lua_tonumber)
         * @param idx Stack index
         * @return f64 Number value or 0 if not convertible
         */
        f64 toNumber(i32 idx);

        /**
         * @brief Convert stack element to integer (lua_tointeger)
         * @param idx Stack index
         * @return i64 Integer value or 0 if not convertible
         */
        i64 toInteger(i32 idx);

        /**
         * @brief Convert stack element to string (lua_tostring)
         * @param idx Stack index
         * @return const char* String value or nullptr if not convertible
         */
        const char* toString(i32 idx);

        /**
         * @brief Convert stack element to string with length (lua_tolstring)
         * @param idx Stack index
         * @param len Output parameter for string length
         * @return const char* String value or nullptr if not convertible
         */
        const char* toLString(i32 idx, usize* len);

        /**
         * @brief Convert stack element to boolean (lua_toboolean)
         * @param idx Stack index
         * @return bool Boolean value (false only for nil and false)
         */
        bool toBoolean(i32 idx);

        // Enhanced Type Checking (Lua 5.1 compatible)
        /**
         * @brief Check if element is C function (lua_iscfunction)
         * @param idx Stack index
         * @return bool True if element is C function
         */
        bool isCFunction(i32 idx);

        /**
         * @brief Check if element is userdata (lua_isuserdata)
         * @param idx Stack index
         * @return bool True if element is userdata
         */
        bool isUserdata(i32 idx);

        /**
         * @brief Get type of stack element (lua_type)
         * @param idx Stack index
         * @return i32 Type constant (LUA_TNIL, LUA_TNUMBER, etc.)
         */
        i32 type(i32 idx);

        /**
         * @brief Get type name (lua_typename)
         * @param tp Type constant
         * @return const char* Type name string
         */
        const char* typeName(i32 tp);

        // Lua 5.1 Compatible Table Operations API
        /**
         * @brief Get table value and push to stack (lua_gettable)
         * @param idx Table index on stack
         */
        void getTable(i32 idx);

        /**
         * @brief Set table value from stack (lua_settable)
         * @param idx Table index on stack
         */
        void setTable(i32 idx);

        /**
         * @brief Get table field by name (lua_getfield)
         * @param idx Table index on stack
         * @param k Field name
         */
        void getField(i32 idx, const char* k);

        /**
         * @brief Set table field by name (lua_setfield)
         * @param idx Table index on stack
         * @param k Field name
         */
        void setField(i32 idx, const char* k);

        /**
         * @brief Raw get from table (lua_rawget)
         * @param idx Table index on stack
         */
        void rawGet(i32 idx);

        /**
         * @brief Raw set to table (lua_rawset)
         * @param idx Table index on stack
         */
        void rawSet(i32 idx);

        /**
         * @brief Raw get by integer index (lua_rawgeti)
         * @param idx Table index on stack
         * @param n Integer key
         */
        void rawGetI(i32 idx, i32 n);

        /**
         * @brief Raw set by integer index (lua_rawseti)
         * @param idx Table index on stack
         * @param n Integer key
         */
        void rawSetI(i32 idx, i32 n);

        /**
         * @brief Create new table (lua_createtable)
         * @param narr Array part size hint
         * @param nrec Hash part size hint
         */
        void createTable(i32 narr, i32 nrec);

        // Lua 5.1 Compatible Function Call API
        /**
         * @brief Call function (lua_call)
         * @param nargs Number of arguments
         * @param nresults Number of results (LUA_MULTRET for all)
         */
        void call(i32 nargs, i32 nresults);

        /**
         * @brief Protected call (lua_pcall)
         * @param nargs Number of arguments
         * @param nresults Number of results (LUA_MULTRET for all)
         * @param errfunc Error function index (0 for none)
         * @return i32 Error code (0 for success)
         */
        i32 pcall(i32 nargs, i32 nresults, i32 errfunc);

        /**
         * @brief C function protected call (lua_cpcall)
         * @param func C function to call
         * @param ud User data to pass
         * @return i32 Error code (0 for success)
         */
        i32 cpcall(lua_CFunction func, void* ud);

        // Lua 5.1 Compatible Coroutine API
        /**
         * @brief Yield from coroutine (lua_yield)
         * @param nresults Number of results to yield
         * @return i32 Always returns LUA_YIELD
         */
        i32 yield(i32 nresults);

        /**
         * @brief Resume coroutine (lua_resume)
         * @param narg Number of arguments
         * @return i32 Status code
         */
        i32 resume(i32 narg);

        /**
         * @brief Get coroutine status (lua_status)
         * @return i32 Status code
         */
        i32 status();

        // Lua 5.1 Compatible Metatable API
        /**
         * @brief Get metatable (lua_getmetatable)
         * @param objindex Object index
         * @return i32 1 if metatable exists, 0 otherwise
         */
        i32 getMetatable(i32 objindex);

        /**
         * @brief Set metatable (lua_setmetatable)
         * @param objindex Object index
         * @return i32 1 on success, 0 on failure
         */
        i32 setMetatable(i32 objindex);

        /**
         * @brief Get function environment (lua_getfenv)
         * @param idx Function index
         */
        void getFenv(i32 idx);

        /**
         * @brief Set function environment (lua_setfenv)
         * @param idx Function index
         * @return i32 1 on success, 0 on failure
         */
        i32 setFenv(i32 idx);
        
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
        

        
        // Enhanced Error Handling (Phase 3)

        /**
         * @brief Set error jump point for error recovery
         * @param jmp Error jump structure
         */
        void setErrorJmp(LuaLongJmp* jmp);

        /**
         * @brief Clear current error jump point
         */
        void clearErrorJmp();

        /**
         * @brief Throw Lua error with specified code and message
         * @param status Error status code
         * @param msg Error message
         */
        [[noreturn]] void throwError(i32 status, const char* msg);

        /**
         * @brief Handle C++ exception and convert to Lua error
         * @param e Exception to handle
         * @return i32 Lua error code
         */
        i32 handleException(const std::exception& e);

        // Debug Hooks System (Phase 3 - Week 9)

        /**
         * @brief Set debug hook function with mask and count (Lua 5.1 compatible)
         * @param func Hook function (lua_Hook)
         * @param mask Hook mask (combination of LUA_MASK* constants)
         * @param count Instruction count for LUA_MASKCOUNT
         */
        void setHook(lua_Hook func, i32 mask, i32 count);

        /**
         * @brief Get current hook function (Lua 5.1 compatible)
         * @return lua_Hook Current hook function or nullptr
         */
        lua_Hook getHook() const;

        /**
         * @brief Get current hook mask (Lua 5.1 compatible)
         * @return i32 Current hook mask
         */
        i32 getHookMask() const;

        /**
         * @brief Get current hook count (Lua 5.1 compatible)
         * @return i32 Current hook count
         */
        i32 getHookCount() const;

        /**
         * @brief Get debug information (Lua 5.1 compatible)
         * @param ar Debug information structure to fill
         * @param what Information to retrieve ("nSltu" format)
         * @return bool True if information was retrieved successfully
         */
        bool getInfo(lua_Debug* ar, const char* what);

        /**
         * @brief Get debug information for stack level (Lua 5.1 compatible)
         * @param level Stack level (0 = current function)
         * @param ar Debug information structure to fill
         * @return bool True if information was retrieved successfully
         */
        bool getStack(i32 level, lua_Debug* ar);

        // Global variable operations
        /**
         * @brief Set global variable
         * @param name Variable name
         * @param val Value to set
         */
        void setGlobal(const GCString* name, const Value& val);

        /**
         * @brief Get global variable
         * @param name Variable name
         * @return Value Global variable value
         */
        Value getGlobal(const GCString* name);

        // High-level execution interface (migrated from State class)
        /**
         * @brief Execute Lua code from string
         * @param code Lua source code
         * @return bool True if execution successful, false otherwise
         */
        bool doString(const Str& code);

        /**
         * @brief Execute Lua code from string and return result
         * @param code Lua source code
         * @return Value Result of execution
         */
        Value doStringWithResult(const Str& code);

        /**
         * @brief Execute Lua code from file
         * @param filename Path to Lua file
         * @return bool True if execution successful, false otherwise
         */
        bool doFile(const Str& filename);

        /**
         * @brief Call a Lua function with arguments
         * @param func Function to call
         * @param args Arguments to pass
         * @return Value Result of function call
         */
        Value callFunction(const Value& func, const Vec<Value>& args);

        /**
         * @brief Call a Lua function with multiple return values
         * @param func Function to call
         * @param args Arguments to pass
         * @return CallResult Result with multiple return values
         */
        CallResult callMultiple(const Value& func, const Vec<Value>& args);

        /**
         * @brief Clear the execution stack
         */
        void clearStack();

        // Coroutine methods
        /**
         * @brief Create a new coroutine
         * @param func Function to run in coroutine
         * @return Pointer to created coroutine
         */
        LuaCoroutine* createCoroutine(GCRef<Function> func);

        /**
         * @brief Resume a coroutine
         * @param coro Coroutine to resume
         * @param args Arguments to pass
         * @return Result of coroutine execution
         */
        CoroutineResult resumeCoroutine(LuaCoroutine* coro, const Vec<Value>& args);

        /**
         * @brief Yield from current coroutine
         * @param values Values to yield
         * @return Result of yield operation
         */
        CoroutineResult yieldFromCoroutine(const Vec<Value>& values);

        /**
         * @brief Get coroutine status
         * @param coro Coroutine to check
         * @return Status of the coroutine
         */
        CoroutineStatus getCoroutineStatus(LuaCoroutine* coro);

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
}
