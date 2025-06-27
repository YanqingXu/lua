#pragma once

#include "../common/types.hpp"
#include "../vm/value.hpp"
#include <stack>
#include <memory>

namespace Lua {

    enum class SymbolType {
        Variable,
        Function,
        Parameter,
        Local,
        Global,
        Upvalue
    };

    // Variable information for scope management
    struct Variable {
        Str name;
        SymbolType type;
        int scopeLevel;
        int stackIndex;     // Stack position for local variables
        bool isUpvalue;     // Whether this variable is captured as upvalue
        bool isCaptured;    // Whether this variable is captured by inner functions
        
        Variable(const Str& name, SymbolType type, int scopeLevel, int stackIndex = -1)
            : name(name), type(type), scopeLevel(scopeLevel), stackIndex(stackIndex),
              isUpvalue(false), isCaptured(false) {}
    };

    // Upvalue descriptor for closure compilation
    struct UpvalueDescriptor {
        Str name;
        int index;          // Index in upvalue array
        bool isLocal;       // True if captures local variable, false if captures upvalue
        int stackIndex;     // Stack index if isLocal=true, upvalue index if isLocal=false
        
        UpvalueDescriptor(const Str& name, int index, bool isLocal, int stackIndex)
            : name(name), index(index), isLocal(isLocal), stackIndex(stackIndex) {}
    };

    struct Symbol {
        Str name;
        SymbolType type;
        int scopeLevel; // The scope depth where this symbol is defined
        // Add other relevant information, e.g., Value, type information, register index
        // For functions, you might store parameter count, etc.
        // For variables, you might store if it's an upvalue, its stack slot, etc.

        Symbol(const Str& name, SymbolType type, int scopeLevel)
            : name(name), type(type), scopeLevel(scopeLevel) {}
    };

    class SymbolTable {
    private:
        Vec<HashMap<Str, Symbol>> scopes;
        int currentScopeLevel;

    public:
        SymbolTable();

        void enterScope();
        void leaveScope();

        // Defines a new symbol in the current scope.
        // Returns true if successful, false if the symbol already exists in the current scope.
        bool define(const Str& name, SymbolType type);

        // Resolves a symbol by searching from the current scope outwards.
        // Returns an optional Symbol. If the symbol is not found, returns std::nullopt.
        Opt<Symbol> resolve(const Str& name) const;

        // Checks if a symbol is defined in the current (innermost) scope.
        bool isDefinedInCurrentScope(const Str& name) const;

        int getCurrentScopeLevel() const { return currentScopeLevel; }
    };

    // Advanced scope manager for closure support
    class ScopeManager {
    public:
        // Scope structure for nested scope management
        struct Scope {
            HashMap<Str, Variable> locals;          // Local variables in this scope
            Vec<UpvalueDescriptor> upvalues;        // Upvalues captured by this scope
            Scope* parent;                          // Parent scope pointer
            int level;                              // Scope nesting level
            int localCount;                         // Number of local variables
            uint32_t magic;                         // Magic number for memory validation
            
            static constexpr uint32_t SCOPE_MAGIC = 0xDEADBEEF;
            
            Scope(Scope* parent = nullptr, int level = 0)
                : parent(parent), level(level), localCount(0), magic(SCOPE_MAGIC) {}
                
            ~Scope() {
                magic = 0; // Invalidate magic on destruction
            }
            
            bool isValid() const {
                return magic == SCOPE_MAGIC;
            }
        };

    private:
        std::stack<UPtr<Scope>> scopes;
        Scope* currentScope;
        int globalScopeLevel;
        int maxRecursionDepth;

    public:
        static constexpr int DEFAULT_MAX_RECURSION_DEPTH = 250;
        ScopeManager();
        ~ScopeManager() = default;

        // Scope management
        void enterScope();
        void exitScope();
        void setParentScope(ScopeManager* parentScopeManager);
        
        // Variable management
        bool defineLocal(const Str& name, int stackIndex = -1);
        Variable* findVariable(const Str& name);
        const Variable* findVariable(const Str& name) const;
        
        // Upvalue management
        bool isUpvalue(const Str& name) const;
        bool markAsCaptured(const Str& name);
        int addUpvalue(const Str& name, bool isLocal, int index);
        const Vec<UpvalueDescriptor>& getUpvalues() const;
        
        // Scope queries
        bool isInCurrentScope(const Str& name) const;
        bool isLocalVariable(const Str& name) const;
        bool isFreeVariable(const Str& name) const;
        
        // Getters
        int getCurrentScopeLevel() const;
        int getLocalCount() const;
        Scope* getCurrentScope() const { return currentScope; }
        
        // Debug and utility
        void dumpScopes() const;
        void clear();
        
        // Safety and configuration
        void setMaxRecursionDepth(int depth) { maxRecursionDepth = depth; }
        int getMaxRecursionDepth() const { return maxRecursionDepth; }
        bool validateCurrentScope() const;
        void validateAllScopes() const;
    };

} // namespace Lua