#pragma once

#include "../common/types.hpp"
#include "../common/defines.hpp"
#include "../gc/core/gc_object.hpp"
#include "instruction.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../vm/Value.hpp"
#include "../vm/upvalue.hpp"
#include <functional>

namespace Lua {
    // Forward declarations
    class State;
    //class Value;
    class GarbageCollector;
    template<typename T> class GCRef;
    
    // Native function type (Lua 5.1 standard - returns number of values pushed)
    using NativeFn = std::function<i32(State* state)>;

    // Legacy native function type (for backward compatibility)
    using NativeFnLegacy = std::function<Value(State* state, int nargs)>;
    
    // Function class
    class Function : public GCObject {
    public:
        enum class Type { Lua, Native };
        
    private:
        Type type;
        
        // Lua function data
        struct LuaData {
            Ptr<Vec<Instruction>> code;
            Vec<Value> constants;
            Vec<GCRef<Function>> prototypes;  // Nested function prototypes
            Vec<GCRef<Upvalue>> upvalues;  // Store upvalue references
            Function* prototype;    // Function prototype (parent function)
            u8 nparams;
            u8 nlocals;
            u8 nupvalues;
            bool isVariadic;        // Whether function accepts variable arguments
            
            // Add default constructor
            LuaData() : code(nullptr), constants{}, prototypes{}, upvalues{}, prototype(nullptr), nparams(0), nlocals(0), nupvalues(0), isVariadic(false) {}
        } lua;
        
        // Native function data
        struct NativeData {
            NativeFn fn;                    // Multi-return function (Lua 5.1 standard)
            NativeFnLegacy fnLegacy;        // Legacy single-return function
            bool isLegacy;                  // Flag to indicate function type

            // Add default constructor
            NativeData() : fn(nullptr), fnLegacy(nullptr), isLegacy(false) {}
        } native;
        
    public:
        // Constructor
        explicit Function(Type type);
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Create Lua function
        static GCRef<Function> createLua(
            Ptr<Vec<Instruction>> code, 
            const Vec<Value>& constants,
            const Vec<GCRef<Function>>& prototypes = {},
            u8 nparams = 0,
            u8 nlocals = 0,
            u8 nupvalues = 0,
            bool isVariadic = false
        );
        
        // Create native function (Lua 5.1 standard - multiple return values)
        static GCRef<Function> createNative(NativeFn fn);

        // Create legacy native function (single return value)
        static GCRef<Function> createNativeLegacy(NativeFnLegacy fn);
        
        // Get function type
        Type getType() const { return type; }
        
        // Get Lua function bytecode
        const Vec<Instruction>& getCode() const;
        
        // Get constant table
        const Vec<Value>& getConstants() const;
        
        // Get prototypes
        const Vec<GCRef<Function>>& getPrototypes() const;
        
        // Get native function (multi-return)
        NativeFn getNative() const;

        // Get legacy native function (single-return)
        NativeFnLegacy getNativeLegacy() const;

        // Check if native function is legacy type
        bool isNativeLegacy() const;
        
        // Get parameter count
        u8 getParamCount() const { return type == Type::Lua ? lua.nparams : 0; }
        
        // Get local variable count
        u8 getLocalCount() const { return type == Type::Lua ? lua.nlocals : 0; }
        
        // Get upvalue count
        u8 getUpvalueCount() const { return type == Type::Lua ? lua.nupvalues : 0; }
        
        // Get variadic flag
        bool getIsVariadic() const { return type == Type::Lua ? lua.isVariadic : false; }
        
        // Get upvalue by index
        GCRef<Upvalue> getUpvalue(usize index) const;
        
        // Set upvalue by index
        void setUpvalue(usize index, GCRef<Upvalue> upvalue);
        
        // Get constant count
        usize getConstantCount() const;
        
        // Get constant by index
        const Value& getConstant(usize index) const;
        
        // Get function prototype
        Function* getPrototype() const { return type == Type::Lua ? lua.prototype : nullptr; }
        
        // Set function prototype
        void setPrototype(Function* proto);
        
        // Close upvalues (for garbage collection)
        void closeUpvalues();
        
        // Boundary validation methods
        bool validateUpvalueCount() const;

        // Validate function nesting depth
        bool validateNestingDepth(u8 currentDepth) const;
        
        // Estimate memory usage for boundary checking
        usize estimateMemoryUsage() const;
        
        // Validate upvalue index bounds
        bool isValidUpvalueIndex(usize index) const;
    };
}
