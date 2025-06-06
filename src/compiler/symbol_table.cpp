#include "symbol_table.hpp"

namespace Lua {

    SymbolTable::SymbolTable() : currentScopeLevel(-1) {
        // Start with the global scope
        enterScope();
    }

    void SymbolTable::enterScope() {
        currentScopeLevel++;
        // Add a new scope if we're entering a scope deeper than what we have
        if (currentScopeLevel >= static_cast<int>(scopes.size())) {
            scopes.push_back(std::unordered_map<Str, Symbol>());
        }
    }

    void SymbolTable::leaveScope() {
        if (currentScopeLevel > 0) { // Don't leave global scope
            // Clear the current scope's symbols
            scopes[currentScopeLevel].clear();
            currentScopeLevel--;
        }
    }

    bool SymbolTable::define(const Str& name, SymbolType type) {
        // Check if the symbol is already defined in the current scope
        if (isDefinedInCurrentScope(name)) {
            return false; // Symbol already exists in current scope
        }

        // Create and insert the new symbol
        scopes[currentScopeLevel].emplace(
            name,
            Symbol(name, type, currentScopeLevel)
        );

        return true;
    }

    std::optional<Symbol> SymbolTable::resolve(const Str& name) const {
        // Search from current scope outward
        for (int level = currentScopeLevel; level >= 0; level--) {
            auto& scope = scopes[level];
            auto it = scope.find(name);
            if (it != scope.end()) {
                return it->second;
            }
        }

        return std::nullopt; // Symbol not found
    }

    bool SymbolTable::isDefinedInCurrentScope(const Str& name) const {
        if (currentScopeLevel < 0 || currentScopeLevel >= static_cast<int>(scopes.size())) {
            return false;
        }

        const auto& currentScope = scopes[currentScopeLevel];
        return currentScope.find(name) != currentScope.end();
    }

} // namespace Lua