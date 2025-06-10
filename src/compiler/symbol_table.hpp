#pragma once

#include "../common/types.hpp"
#include "../vm/value.hpp"

namespace Lua {

    enum class SymbolType {
        Variable,
        Function,
        Parameter,
        // Add other types as needed (e.g., Local, Global, Upvalue)
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

} // namespace Lua