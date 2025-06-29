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
            scopes.push_back(HashMap<Str, Symbol>());
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

    Opt<Symbol> SymbolTable::resolve(const Str& name) const {
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

    // ScopeManager implementation
    ScopeManager::ScopeManager() : currentScope(nullptr), globalScopeLevel(0), maxRecursionDepth(DEFAULT_MAX_RECURSION_DEPTH) {
        // Start with global scope
        enterScope();
    }

    void ScopeManager::enterScope() {
        // Check recursion depth limit
        if (scopes.size() >= static_cast<size_t>(maxRecursionDepth)) {
            throw std::runtime_error("Scope stack depth exceeded: " + std::to_string(maxRecursionDepth) + 
                                    " (current stack size: " + std::to_string(scopes.size()) + ")");
        }
        
        // Validate current scope before proceeding
        if (currentScope && !currentScope->isValid()) {
            throw std::runtime_error("Current scope memory corruption detected in enterScope at stack depth " + 
                                    std::to_string(scopes.size()));
        }
        
        int level = currentScope ? currentScope->level + 1 : globalScopeLevel;
        auto newScope = std::make_unique<Scope>(currentScope, level);
        currentScope = newScope.get();
        scopes.push(std::move(newScope));
    }

    void ScopeManager::exitScope() {
        if (scopes.size() <= 1) {
            // Don't exit global scope
            return;
        }
        
        // Validate current scope before exit
        if (currentScope && !currentScope->isValid()) {
            throw std::runtime_error("Current scope memory corruption detected in exitScope");
        }
        
        scopes.pop();
        if (!scopes.empty()) {
            currentScope = scopes.top().get();
            // Validate new current scope
            if (currentScope && !currentScope->isValid()) {
                throw std::runtime_error("New current scope memory corruption detected after exitScope");
            }
        } else {
            currentScope = nullptr;
        }
    }

    bool ScopeManager::defineLocal(const Str& name, int stackIndex) {
        if (!currentScope) {
            return false;
        }
        
        // Validate scope memory integrity
        if (!currentScope->isValid()) {
            throw std::runtime_error("Current scope memory corruption detected in defineLocal");
        }
        
        // Check if variable already exists in current scope
        if (currentScope->locals.find(name) != currentScope->locals.end()) {
            return false; // Variable already defined in current scope
        }
        
        // Use provided stack index or auto-assign
        int actualStackIndex = (stackIndex >= 0) ? stackIndex : currentScope->localCount;
        
        // Create and insert the variable
        Variable var(name, SymbolType::Local, currentScope->level, actualStackIndex);
        currentScope->locals.emplace(name, var);
        currentScope->localCount++;
        
        return true;
    }

    Variable* ScopeManager::findVariable(const Str& name) {
        // Search from current scope upward
        Scope* scope = currentScope;
        while (scope) {
            auto it = scope->locals.find(name);
            if (it != scope->locals.end()) {
                return &it->second;
            }
            scope = scope->parent;
        }
        return nullptr;
    }

    const Variable* ScopeManager::findVariable(const Str& name) const {
        // Search from current scope upward
        Scope* scope = currentScope;
        while (scope) {
            auto it = scope->locals.find(name);
            if (it != scope->locals.end()) {
                return &it->second;
            }
            scope = scope->parent;
        }
        return nullptr;
    }

    bool ScopeManager::isUpvalue(const Str& name) const {
        if (!currentScope) {
            return false;
        }
        
        // Check if variable exists in current scope
        if (currentScope->locals.find(name) != currentScope->locals.end()) {
            return false; // It's a local variable, not an upvalue
        }
        
        // Check if variable exists in parent scopes
        Scope* scope = currentScope->parent;
        while (scope) {
            if (scope->locals.find(name) != scope->locals.end()) {
                return true; // Found in parent scope, so it's an upvalue
            }
            scope = scope->parent;
        }
        
        return false; // Variable not found
    }

    bool ScopeManager::markAsCaptured(const Str& name) {
        Variable* var = findVariable(name);
        if (var) {
            var->isCaptured = true;
            return true;
        }
        return false;
    }

    int ScopeManager::addUpvalue(const Str& name, bool isLocal, int index) {
        if (!currentScope) {
            return -1;
        }
        
        // Check if upvalue already exists
        for (size_t i = 0; i < currentScope->upvalues.size(); ++i) {
            if (currentScope->upvalues[i].name == name) {
                return static_cast<int>(i); // Return existing index
            }
        }
        
        // Add new upvalue
        int upvalueIndex = static_cast<int>(currentScope->upvalues.size());
        currentScope->upvalues.emplace_back(name, upvalueIndex, isLocal, index);
        
        return upvalueIndex;
    }

    const Vec<UpvalueDescriptor>& ScopeManager::getUpvalues() const {
        static Vec<UpvalueDescriptor> empty;
        if (!currentScope) {
            return empty;
        }
        
        // Validate scope before accessing upvalues
        if (!currentScope->isValid()) {
            throw std::runtime_error("Current scope memory corruption detected in getUpvalues");
        }
        
        return currentScope->upvalues;
    }
    
    bool ScopeManager::validateCurrentScope() const {
        if (!currentScope) {
            return true; // null is valid state
        }
        return currentScope->isValid();
    }
    
    void ScopeManager::validateAllScopes() const {
        // Validate scopes by traversing from current scope upward
        Scope* scope = currentScope;
        while (scope) {
            if (!scope->isValid()) {
                throw std::runtime_error("Scope memory corruption detected in scope validation");
            }
            scope = scope->parent;
        }
    }

    bool ScopeManager::isInCurrentScope(const Str& name) const {
        if (!currentScope) {
            return false;
        }
        
        // Validate scope memory integrity
        if (!currentScope->isValid()) {
            throw std::runtime_error("Current scope memory corruption detected in isInCurrentScope");
        }
        
        try {
            return currentScope->locals.find(name) != currentScope->locals.end();
        } catch (const std::exception& e) {
            throw std::runtime_error("Exception in scope locals access: " + std::string(e.what()));
        }
    }

    bool ScopeManager::isLocalVariable(const Str& name) const {
        // A local variable is one that exists in the current scope or any parent scope
        // within the same function (not crossing function boundaries)
        const Variable* var = findVariable(name);
        return var != nullptr;
    }

    bool ScopeManager::isFreeVariable(const Str& name) const {
        // A free variable is one that:
        // 1. Is not defined in the current scope
        // 2. Is defined in some parent scope
        if (isInCurrentScope(name)) {
            return false; // It's a local variable
        }
        
        // Check if it exists in parent scopes
        if (!currentScope) {
            return false;
        }
        
        Scope* scope = currentScope->parent;
        while (scope) {
            if (scope->locals.find(name) != scope->locals.end()) {
                return true; // Found in parent scope, so it's free
            }
            scope = scope->parent;
        }
        
        return false; // Variable not found anywhere
    }

    int ScopeManager::getCurrentScopeLevel() const {
        return currentScope ? currentScope->level : -1;
    }

    int ScopeManager::getLocalCount() const {
        return currentScope ? currentScope->localCount : 0;
    }

    void ScopeManager::dumpScopes() const {
        // Debug function to print scope hierarchy
        // Implementation can be added for debugging purposes
        // For now, just a placeholder
    }

    void ScopeManager::clear() {
        // Safely clear all scopes
        while (!scopes.empty()) {
            scopes.pop();
        }
        currentScope = nullptr;
        // Re-enter global scope
        enterScope();
    }

    void ScopeManager::setParentScope(ScopeManager* parentScopeManager) {
        if (parentScopeManager && parentScopeManager->currentScope) {
            // If we don't have a current scope, create one
            if (!currentScope) {
                enterScope();
            }

            // Set the parent scope chain
            currentScope->parent = parentScopeManager->currentScope;
        }
    }

} // namespace Lua