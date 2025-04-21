#pragma once

#include "../types.hpp"
#include "../lexer/lexer.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/value.hpp"
#include <memory>
#include <vector>

namespace Lua {
    // 表达式类型
    enum class ExprType {
        Binary,     // 二元表达式
        Unary,      // 一元表达式
        Literal,    // 字面量
        Variable,   // 变量
        Call,       // 函数调用
        Table       // 表构造
    };
    
    // 表达式基类
    class Expr {
    public:
        virtual ~Expr() = default;
        virtual ExprType getType() const = 0;
    };
    
    // 字面量表达式 (数字、字符串、布尔值、nil)
    class LiteralExpr : public Expr {
    private:
        Value value;
        
    public:
        explicit LiteralExpr(const Value& value) : value(value) {}
        
        ExprType getType() const override { return ExprType::Literal; }
        const Value& getValue() const { return value; }
    };
    
    // 变量表达式
    class VariableExpr : public Expr {
    private:
        Str name;
        
    public:
        explicit VariableExpr(const Str& name) : name(name) {}
        
        ExprType getType() const override { return ExprType::Variable; }
        const Str& getName() const { return name; }
    };
    
    // 一元表达式
    class UnaryExpr : public Expr {
    private:
        TokenType op;
        UniquePtr<Expr> right;
        
    public:
        UnaryExpr(TokenType op, UniquePtr<Expr> right)
            : op(op), right(std::move(right)) {}
            
        ExprType getType() const override { return ExprType::Unary; }
        TokenType getOperator() const { return op; }
        const Expr* getRight() const { return right.get(); }
    };
    
    // 二元表达式
    class BinaryExpr : public Expr {
    private:
        UniquePtr<Expr> left;
        TokenType op;
        UniquePtr<Expr> right;
        
    public:
        BinaryExpr(UniquePtr<Expr> left, TokenType op, UniquePtr<Expr> right)
            : left(std::move(left)), op(op), right(std::move(right)) {}
            
        ExprType getType() const override { return ExprType::Binary; }
        const Expr* getLeft() const { return left.get(); }
        TokenType getOperator() const { return op; }
        const Expr* getRight() const { return right.get(); }
    };
    
    // 函数调用表达式
    class CallExpr : public Expr {
    private:
        UniquePtr<Expr> callee;
        Vec<UniquePtr<Expr>> arguments;
        
    public:
        CallExpr(UniquePtr<Expr> callee, Vec<UniquePtr<Expr>> arguments)
            : callee(std::move(callee)), arguments(std::move(arguments)) {}
            
        ExprType getType() const override { return ExprType::Call; }
        const Expr* getCallee() const { return callee.get(); }
        const Vec<UniquePtr<Expr>>& getArguments() const { return arguments; }
    };
    
    // 语句类型
    enum class StmtType {
        Expression,  // 表达式语句
        Block,       // 块语句
        If,          // if语句
        While,       // while循环
        Function,    // 函数定义
        Return,      // return语句
        Local        // 局部变量声明
    };
    
    // 语句基类
    class Stmt {
    public:
        virtual ~Stmt() = default;
        virtual StmtType getType() const = 0;
    };
    
    // 表达式语句
    class ExprStmt : public Stmt {
    private:
        UniquePtr<Expr> expression;
        
    public:
        explicit ExprStmt(UniquePtr<Expr> expression)
            : expression(std::move(expression)) {}
            
        StmtType getType() const override { return StmtType::Expression; }
        const Expr* getExpression() const { return expression.get(); }
    };
    
    // 块语句 (多条语句组成的块)
    class BlockStmt : public Stmt {
    private:
        Vec<UniquePtr<Stmt>> statements;
        
    public:
        explicit BlockStmt(Vec<UniquePtr<Stmt>> statements)
            : statements(std::move(statements)) {}
            
        StmtType getType() const override { return StmtType::Block; }
        const Vec<UniquePtr<Stmt>>& getStatements() const { return statements; }
    };
    
    // 局部变量声明语句
    class LocalStmt : public Stmt {
    private:
        Str name;
        UniquePtr<Expr> initializer;
        
    public:
        LocalStmt(const Str& name, UniquePtr<Expr> initializer)
            : name(name), initializer(std::move(initializer)) {}
            
        StmtType getType() const override { return StmtType::Local; }
        const Str& getName() const { return name; }
        const Expr* getInitializer() const { return initializer.get(); }
    };
    
    // 语法分析器类
    class Parser {
    private:
        Lexer lexer;
        Token current;
        Token previous;
        bool hadError;
        
        // 辅助方法
        void advance();
        bool check(TokenType type) const;
        bool match(TokenType type);
        bool match(std::initializer_list<TokenType> types);
        Token consume(TokenType type, const Str& message);
        void error(const Str& message);
        void synchronize();
        
        // 解析表达式
        UniquePtr<Expr> expression();
        UniquePtr<Expr> simpleExpression();
        UniquePtr<Expr> term();
        UniquePtr<Expr> factor();
        UniquePtr<Expr> primary();
        
        // 解析语句
        UniquePtr<Stmt> statement();
        UniquePtr<Stmt> expressionStatement();
        UniquePtr<Stmt> localDeclaration();
        
    public:
        explicit Parser(const Str& source);
        
        // 解析整个程序
        Vec<UniquePtr<Stmt>> parse();
        
        // 检查是否有错误
        bool hasError() const { return hadError; }
    };
}
