#include "upvalue_analyzer.hpp"
#include <iostream>

namespace Lua {
    UpvalueAnalyzer::UpvalueAnalyzer(ScopeManager& scopeManager)
        : scopeManager_(&scopeManager) {
    }
    
    UpvalueAnalyzer::UpvalueAnalyzer()
        : scopeManager_(nullptr) {
    }

    Vec<UpvalueDescriptor> UpvalueAnalyzer::analyzeFunction(const FunctionExpr* funcExpr) {
        reset();

        // Create local scope manager if none provided
        ScopeManager localScopeManager;
        ScopeManager* sm = scopeManager_ ? scopeManager_ : &localScopeManager;

        // Enter function scope
        sm->enterScope();

        // Define parameters as local variables
        for (const auto& param : funcExpr->getParameters()) {
            bool success = sm->defineLocal(param);
            if (!success) {
                // Parameter definition failed, possibly duplicate definition
                // Should report error in actual application, continue for testing here
            }
        }

        // Analyze function body
        analyzeStatement(funcExpr->getBody());

        // Create upvalue descriptors for all free variables
        for (const auto& freeVar : freeVars_) {
            upvalues_.push_back(createUpvalueDescriptor(freeVar));
        }

        // Exit function scope
        sm->exitScope();

        return upvalues_;
    }

    Vec<UpvalueDescriptor> UpvalueAnalyzer::analyzeFunction(const FunctionStmt* funcStmt) {
        reset();

        // Create local scope manager if none provided
        ScopeManager localScopeManager;
        ScopeManager* sm = scopeManager_ ? scopeManager_ : &localScopeManager;

        // Enter function scope
        sm->enterScope();

        // Define parameters as local variables
        for (const auto& param : funcStmt->getParameters()) {
            bool success = sm->defineLocal(param);
            if (!success) {
                // Parameter definition failed, possibly due to duplicate definition
                // In actual application should report error, here continue for testing
            }
        }

        // Analyze function body
        analyzeStatement(funcStmt->getBody());

        // Create upvalue descriptors for all free variables
        for (const auto& freeVar : freeVars_) {
            upvalues_.push_back(createUpvalueDescriptor(freeVar));
        }

        // Exit function scope
        sm->exitScope();

        return upvalues_;
    }

    bool UpvalueAnalyzer::isLocalVariable(const Str& name) const {
        return scopeManager_ ? scopeManager_->isLocalVariable(name) : false;
    }

    bool UpvalueAnalyzer::isFreeVariable(const Str& name) const {
        return scopeManager_ ? !scopeManager_->isLocalVariable(name) : true;
    }

    void UpvalueAnalyzer::analyzeExpression(const Expr* expr) {
        if (!expr) return;

        switch (expr->getType()) {
        case ExprType::Variable:
            analyzeVariableExpr(static_cast<const VariableExpr*>(expr));
            break;
        case ExprType::Binary:
            analyzeBinaryExpr(static_cast<const BinaryExpr*>(expr));
            break;
        case ExprType::Unary:
            analyzeUnaryExpr(static_cast<const UnaryExpr*>(expr));
            break;
        case ExprType::Call:
            analyzeCallExpr(static_cast<const CallExpr*>(expr));
            break;
        case ExprType::Member:
            analyzeMemberExpr(static_cast<const MemberExpr*>(expr));
            break;
        case ExprType::Index:
            analyzeIndexExpr(static_cast<const IndexExpr*>(expr));
            break;
        case ExprType::Table:
            analyzeTableExpr(static_cast<const TableExpr*>(expr));
            break;
        case ExprType::Function: {
            // Nested functions require recursive analysis
            const auto* funcExpr = static_cast<const FunctionExpr*>(expr);
            UpvalueAnalyzer nestedAnalyzer(*scopeManager_);
            nestedAnalyzer.analyzeFunction(funcExpr);
            break;
        }
        case ExprType::Literal:
            // Literals do not contain variable references
            break;
        }
    }

    void UpvalueAnalyzer::analyzeStatement(const Stmt* stmt) {
        if (!stmt) return;

        switch (stmt->getType()) {
        case StmtType::Expression:
            analyzeExpression(static_cast<const ExprStmt*>(stmt)->getExpression());
            break;
        case StmtType::Block:
            analyzeBlockStmt(static_cast<const BlockStmt*>(stmt));
            break;
        case StmtType::Local:
            analyzeLocalStmt(static_cast<const LocalStmt*>(stmt));
            break;
        case StmtType::Assign:
            analyzeAssignStmt(static_cast<const AssignStmt*>(stmt));
            break;
        case StmtType::If:
            analyzeIfStmt(static_cast<const IfStmt*>(stmt));
            break;
        case StmtType::While:
            analyzeWhileStmt(static_cast<const WhileStmt*>(stmt));
            break;
        case StmtType::For:
            analyzeForStmt(static_cast<const ForStmt*>(stmt));
            break;
        case StmtType::ForIn:
            analyzeForInStmt(static_cast<const ForInStmt*>(stmt));
            break;
        case StmtType::Return:
            analyzeReturnStmt(static_cast<const ReturnStmt*>(stmt));
            break;
        case StmtType::RepeatUntil:
            analyzeRepeatUntilStmt(static_cast<const RepeatUntilStmt*>(stmt));
            break;
        case StmtType::Function:
            analyzeFunctionStmt(static_cast<const FunctionStmt*>(stmt));
            break;
        case StmtType::Break:
            // break statements do not contain variable references
            break;
        }
    }

    void UpvalueAnalyzer::analyzeVariableExpr(const VariableExpr* varExpr) {
        const Str& name = varExpr->getName();

        // If not a local variable, it might be a free variable
        if (!scopeManager_->isLocalVariable(name)) {
            // Check if it's a variable from outer scope
            if (scopeManager_->isFreeVariable(name)) {
                freeVars_.insert(name);
                // Mark as captured variable
                scopeManager_->markAsCaptured(name);
            }
        }
    }

    void UpvalueAnalyzer::analyzeBinaryExpr(const BinaryExpr* binExpr) {
        analyzeExpression(binExpr->getLeft());
        analyzeExpression(binExpr->getRight());
    }

    void UpvalueAnalyzer::analyzeUnaryExpr(const UnaryExpr* unaryExpr) {
        analyzeExpression(unaryExpr->getRight());
    }

    void UpvalueAnalyzer::analyzeCallExpr(const CallExpr* callExpr) {
        analyzeExpression(callExpr->getCallee());
        for (const auto& arg : callExpr->getArguments()) {
            analyzeExpression(arg.get());
        }
    }

    void UpvalueAnalyzer::analyzeMemberExpr(const MemberExpr* memberExpr) {
        analyzeExpression(memberExpr->getObject());
    }

    void UpvalueAnalyzer::analyzeIndexExpr(const IndexExpr* indexExpr) {
        analyzeExpression(indexExpr->getObject());
        analyzeExpression(indexExpr->getIndex());
    }

    void UpvalueAnalyzer::analyzeTableExpr(const TableExpr* tableExpr) {
        for (const auto& field : tableExpr->getFields()) {
            if (field.key) {
                analyzeExpression(field.key.get());
            }
            analyzeExpression(field.value.get());
        }
    }

    void UpvalueAnalyzer::analyzeBlockStmt(const BlockStmt* blockStmt) {
        // Note: Block statements in function body should not create new scope
        // because function parameters are already defined in function scope
        // Analyze statements directly without entering new scope

        for (const auto& stmt : blockStmt->getStatements()) {
            analyzeStatement(stmt.get());
        }
    }

    void UpvalueAnalyzer::analyzeLocalStmt(const LocalStmt* localStmt) {
        // First analyze initialization expression (may reference other variables)
        if (localStmt->getInitializer()) {
            analyzeExpression(localStmt->getInitializer());
        }

        // Then define local variable
        scopeManager_->defineLocal(localStmt->getName());
    }

    void UpvalueAnalyzer::analyzeAssignStmt(const AssignStmt* assignStmt) {
        analyzeExpression(assignStmt->getTarget());
        analyzeExpression(assignStmt->getValue());
    }

    void UpvalueAnalyzer::analyzeIfStmt(const IfStmt* ifStmt) {
        analyzeExpression(ifStmt->getCondition());
        analyzeStatement(ifStmt->getThenBranch());
        if (ifStmt->getElseBranch()) {
            analyzeStatement(ifStmt->getElseBranch());
        }
    }

    void UpvalueAnalyzer::analyzeWhileStmt(const WhileStmt* whileStmt) {
        analyzeExpression(whileStmt->getCondition());
        analyzeStatement(whileStmt->getBody());
    }

    void UpvalueAnalyzer::analyzeForStmt(const ForStmt* forStmt) {
        // Enter for loop scope
        scopeManager_->enterScope();

        // Analyze loop range expressions
        analyzeExpression(forStmt->getStart());
        analyzeExpression(forStmt->getEnd());
        if (forStmt->getStep()) {
            analyzeExpression(forStmt->getStep());
        }

        // Define loop variable
        scopeManager_->defineLocal(forStmt->getVariable());

        // Analyze loop body
        analyzeStatement(forStmt->getBody());

        // Exit for loop scope
        scopeManager_->exitScope();
    }

    void UpvalueAnalyzer::analyzeForInStmt(const ForInStmt* forInStmt) {
        // Enter for-in loop scope
        scopeManager_->enterScope();

        // Analyze iterator expressions
        for (const auto& iterator : forInStmt->getIterators()) {
            analyzeExpression(iterator.get());
        }

        // Define loop variables
        for (const auto& var : forInStmt->getVariables()) {
            scopeManager_->defineLocal(var);
        }

        // Analyze loop body
        analyzeStatement(forInStmt->getBody());

        // Exit for-in loop scope
        scopeManager_->exitScope();
    }

    void UpvalueAnalyzer::analyzeReturnStmt(const ReturnStmt* returnStmt) {
        if (returnStmt->getValue()) {
            analyzeExpression(returnStmt->getValue());
        }
    }

    void UpvalueAnalyzer::analyzeRepeatUntilStmt(const RepeatUntilStmt* repeatStmt) {
        analyzeStatement(repeatStmt->getBody());
        analyzeExpression(repeatStmt->getCondition());
    }

    void UpvalueAnalyzer::analyzeFunctionStmt(const FunctionStmt* funcStmt) {
        // Nested function definitions require recursive analysis
        UpvalueAnalyzer nestedAnalyzer(*scopeManager_);
        nestedAnalyzer.analyzeFunction(funcStmt);
    }

    UpvalueDescriptor UpvalueAnalyzer::createUpvalueDescriptor(const Str& name) {
        int index = static_cast<int>(upvalues_.size());
        bool isLocal = true;
        int stackIndex = 0;

        // Find variable information
        if (scopeManager_) {
            auto* variable = scopeManager_->findVariable(name);
            if (variable) {
                // Check if variable is in direct outer scope
                if (scopeManager_->isUpvalue(name)) {
                    isLocal = false;
                }
                stackIndex = variable->stackIndex;
            }
        }

        return UpvalueDescriptor(name, index, isLocal, stackIndex);
    }

    void UpvalueAnalyzer::reset() {
        upvalues_.clear();
        freeVars_.clear();
    }
}