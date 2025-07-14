#pragma once

#include "ast_base.hpp"
#include "expressions.hpp"

namespace Lua {
    // Expression statement
    class ExprStmt : public Stmt {
    private:
        UPtr<Expr> expression;

    public:
        explicit ExprStmt(UPtr<Expr> expression, const SourceLocation& location = SourceLocation())
            : Stmt(location), expression(std::move(expression)) {}

        StmtType getType() const override { return StmtType::Expression; }
        const Expr* getExpression() const { return expression.get(); }
    };

    // Block statement (block composed of multiple statements)
    class BlockStmt : public Stmt {
    private:
        Vec<UPtr<Stmt>> statements;

    public:
        explicit BlockStmt(Vec<UPtr<Stmt>> statements, const SourceLocation& location = SourceLocation())
            : Stmt(location), statements(std::move(statements)) {}

        StmtType getType() const override { return StmtType::Block; }
        const Vec<UPtr<Stmt>>& getStatements() const { return statements; }
    };

    // Local variable declaration statement
    class LocalStmt : public Stmt {
    private:
        Str name;
        UPtr<Expr> initializer;

    public:
        LocalStmt(const Str& name, UPtr<Expr> initializer, const SourceLocation& location = SourceLocation())
            : Stmt(location), name(name), initializer(std::move(initializer)) {}

        StmtType getType() const override { return StmtType::Local; }
        const Str& getName() const { return name; }
        const Expr* getInitializer() const { return initializer.get(); }
    };

    // Multi-local variable declaration statement (local a, b, c = expr1, expr2, ...)
    class MultiLocalStmt : public Stmt {
    private:
        Vec<Str> names;
        Vec<UPtr<Expr>> initializers;

    public:
        MultiLocalStmt(Vec<Str> names, Vec<UPtr<Expr>> initializers, const SourceLocation& location = SourceLocation())
            : Stmt(location), names(std::move(names)), initializers(std::move(initializers)) {}

        StmtType getType() const override { return StmtType::MultiLocal; }
        const Vec<Str>& getNames() const { return names; }
        const Vec<UPtr<Expr>>& getInitializers() const { return initializers; }
    };

    // Assignment statement (var = expr, obj.field = expr, obj[key] = expr)
    class AssignStmt : public Stmt {
    private:
        UPtr<Expr> target;
        UPtr<Expr> value;

    public:
        AssignStmt(UPtr<Expr> target, UPtr<Expr> value, const SourceLocation& location = SourceLocation())
            : Stmt(location), target(std::move(target)), value(std::move(value)) {}

        StmtType getType() const override { return StmtType::Assign; }
        const Expr* getTarget() const { return target.get(); }
        const Expr* getValue() const { return value.get(); }
    };

    // If statement (if condition then body [else elseBody] end)
    class IfStmt : public Stmt {
    private:
        UPtr<Expr> condition;
        UPtr<Stmt> thenBranch;
        UPtr<Stmt> elseBranch;

    public:
        IfStmt(UPtr<Expr> condition, UPtr<Stmt> thenBranch, UPtr<Stmt> elseBranch = nullptr, const SourceLocation& location = SourceLocation())
            : Stmt(location), condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

        StmtType getType() const override { return StmtType::If; }
        const Expr* getCondition() const { return condition.get(); }
        const Stmt* getThenBranch() const { return thenBranch.get(); }
        const Stmt* getElseBranch() const { return elseBranch.get(); }
    };

    // While statement (while condition do body end)
    class WhileStmt : public Stmt {
    private:
        UPtr<Expr> condition;
        UPtr<Stmt> body;

    public:
        WhileStmt(UPtr<Expr> condition, UPtr<Stmt> body, const SourceLocation& location = SourceLocation())
            : Stmt(location), condition(std::move(condition)), body(std::move(body)) {}

        StmtType getType() const override { return StmtType::While; }
        const Expr* getCondition() const { return condition.get(); }
        const Stmt* getBody() const { return body.get(); }
    };

    // For statement (for var = start, end [, step] do body end)
    class ForStmt : public Stmt {
    private:
        Str variable;        // Loop variable name
        UPtr<Expr> start;    // Start value
        UPtr<Expr> end;      // End value
        UPtr<Expr> step;     // Step value (optional, defaults to 1)
        UPtr<Stmt> body;     // Loop body

    public:
        ForStmt(const Str& variable, UPtr<Expr> start, UPtr<Expr> end, UPtr<Expr> step, UPtr<Stmt> body, const SourceLocation& location = SourceLocation())
            : Stmt(location), variable(variable), start(std::move(start)), end(std::move(end)), step(std::move(step)), body(std::move(body)) {}

        StmtType getType() const override { return StmtType::For; }
        const Str& getVariable() const { return variable; }
        const Expr* getStart() const { return start.get(); }
        const Expr* getEnd() const { return end.get(); }
        const Expr* getStep() const { return step.get(); }
        const Stmt* getBody() const { return body.get(); }
    };

    // For-in statement (for var1, var2, ... in expr-list do body end)
    class ForInStmt : public Stmt {
    private:
        Vec<Str> variables;        // Loop variable names
        Vec<UPtr<Expr>> iterators; // Iterator expression list
        UPtr<Stmt> body;           // Loop body

    public:
        ForInStmt(const Vec<Str>& variables, Vec<UPtr<Expr>> iterators, UPtr<Stmt> body, const SourceLocation& location = SourceLocation())
            : Stmt(location), variables(variables), iterators(std::move(iterators)), body(std::move(body)) {}

        StmtType getType() const override { return StmtType::ForIn; }
        const Vec<Str>& getVariables() const { return variables; }
        const Vec<UPtr<Expr>>& getIterators() const { return iterators; }
        const Stmt* getBody() const { return body.get(); }
    };

    // Return statement
    class ReturnStmt : public Stmt {
    private:
        Vec<UPtr<Expr>> values; // Return value list (supports multiple return values)

    public:
        // Constructor for single return value (backward compatibility)
        ReturnStmt(UPtr<Expr> value = nullptr, const SourceLocation& location = SourceLocation())
            : Stmt(location) {
            if (value) {
                values.push_back(std::move(value));
            }
        }

        // Constructor for multiple return values
        ReturnStmt(Vec<UPtr<Expr>> values, const SourceLocation& location = SourceLocation())
            : Stmt(location), values(std::move(values)) {
        }

        StmtType getType() const override { return StmtType::Return; }
        
        // Get all return values
        const Vec<UPtr<Expr>>& getValues() const { return values; }
        
        // Get single return value (for backward compatibility)
        const Expr* getValue() const { 
            return values.empty() ? nullptr : values[0].get(); 
        }
        
        // Check if has return values
        bool hasValues() const { return !values.empty(); }
        
        // Get number of return values
        size_t getValueCount() const { return values.size(); }
    };

    // Break statement
    class BreakStmt : public Stmt {
    public:
        BreakStmt(const SourceLocation& location = SourceLocation()) : Stmt(location) {}

        StmtType getType() const override { return StmtType::Break; }
    };

    // Repeat-until statement (repeat body until condition)
    class RepeatUntilStmt : public Stmt {
    private:
        UPtr<Stmt> body;      // Loop body
        UPtr<Expr> condition; // Until condition

    public:
        RepeatUntilStmt(UPtr<Stmt> body, UPtr<Expr> condition, const SourceLocation& location = SourceLocation())
            : Stmt(location), body(std::move(body)), condition(std::move(condition)) {}

        StmtType getType() const override { return StmtType::RepeatUntil; }
        const Stmt* getBody() const { return body.get(); }
        const Expr* getCondition() const { return condition.get(); }
    };

    // Function definition statement (function name(params) body end)
    class FunctionStmt : public Stmt {
    private:
        Str name;                // Function name
        Vec<Str> parameters;     // Parameter list
        UPtr<Stmt> body;         // Function body
        bool isVariadic;         // Whether function accepts variable arguments

    public:
        FunctionStmt(const Str& name, const Vec<Str>& parameters, UPtr<Stmt> body, bool variadic = false, const SourceLocation& location = SourceLocation())
            : Stmt(location), name(name), parameters(parameters), body(std::move(body)), isVariadic(variadic) {}

        StmtType getType() const override { return StmtType::Function; }
        const Str& getName() const { return name; }
        const Vec<Str>& getParameters() const { return parameters; }
        const Stmt* getBody() const { return body.get(); }
        bool getIsVariadic() const { return isVariadic; }
    };

    // Do statement (do body end)
    class DoStmt : public Stmt {
    private:
        UPtr<Stmt> body;  // Block body

    public:
        DoStmt(UPtr<Stmt> body, const SourceLocation& location = SourceLocation())
            : Stmt(location), body(std::move(body)) {}

        StmtType getType() const override { return StmtType::Do; }
        const Stmt* getBody() const { return body.get(); }
    };
}