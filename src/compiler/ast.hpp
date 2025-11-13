#pragma once

/**
 * @file ast.hpp
 * @brief Lua抽象语法树（AST）节点定义
 * 
 * 实现Lua 5.1的抽象语法树节点类型，用于表示解析后的程序结构。
 * 
 * 核心功能：
 * - 定义所有语句类型（赋值、控制结构、函数定义等）
 * - 定义所有表达式类型（字面量、运算符、函数调用等）
 * - 使用智能指针管理节点内存
 * - 支持访问者模式进行AST遍历
 * 
 * 设计原则：
 * - 使用std::variant实现类型安全的多态
 * - 使用std::unique_ptr管理节点生命周期
 * - 清晰的节点层次结构
 * - 完整的位置信息（行号、列号）
 */

#include "common/types.hpp"
#include <memory>
#include <variant>
#include <vector>

namespace Lua {

// 前向声明
struct Expr;
struct Stmt;

// 智能指针类型别名
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// =====================================================================
// 表达式节点（Expressions）
// =====================================================================

/**
 * @brief nil字面量
 */
struct NilExpr {
    i32 line;
    i32 column;
};

/**
 * @brief 布尔字面量
 */
struct BoolExpr {
    bool value;
    i32 line;
    i32 column;
};

/**
 * @brief 数字字面量
 */
struct NumberExpr {
    f64 value;
    i32 line;
    i32 column;
};

/**
 * @brief 字符串字面量
 */
struct StringExpr {
    Str value;
    i32 line;
    i32 column;
};

/**
 * @brief 变长参数 ...
 */
struct VarargExpr {
    i32 line;
    i32 column;
};

/**
 * @brief 标识符（变量名）
 */
struct NameExpr {
    Str name;
    i32 line;
    i32 column;
};

/**
 * @brief 二元运算表达式
 */
struct BinaryExpr {
    enum class Op {
        // 算术运算
        Add, Sub, Mul, Div, Mod, Pow,
        // 比较运算
        Eq, Ne, Lt, Le, Gt, Ge,
        // 逻辑运算
        And, Or,
        // 字符串连接
        Concat
    };
    
    Op op;
    ExprPtr left;
    ExprPtr right;
    i32 line;
    i32 column;
};

/**
 * @brief 一元运算表达式
 */
struct UnaryExpr {
    enum class Op {
        Not,    // not
        Neg,    // -
        Len     // #
    };
    
    Op op;
    ExprPtr operand;
    i32 line;
    i32 column;
};

/**
 * @brief 表构造器
 */
struct TableField {
    ExprPtr key;    // nil表示数组部分
    ExprPtr value;
};

struct TableExpr {
    Vec<TableField> fields;
    i32 line;
    i32 column;
};

/**
 * @brief 函数调用
 */
struct CallExpr {
    ExprPtr func;
    Vec<ExprPtr> args;
    i32 line;
    i32 column;
};

/**
 * @brief 表索引访问 table[key]
 */
struct IndexExpr {
    ExprPtr table;
    ExprPtr index;
    i32 line;
    i32 column;
};

/**
 * @brief 成员访问 table.member
 */
struct MemberExpr {
    ExprPtr table;
    Str member;
    i32 line;
    i32 column;
};

/**
 * @brief 函数定义表达式
 */
struct FunctionExpr {
    Vec<Str> params;
    bool isVararg;
    Vec<StmtPtr> body;
    i32 line;
    i32 column;
};

/**
 * @brief 表达式的variant类型
 */
using ExprVariant = std::variant<
    NilExpr,
    BoolExpr,
    NumberExpr,
    StringExpr,
    VarargExpr,
    NameExpr,
    BinaryExpr,
    UnaryExpr,
    TableExpr,
    CallExpr,
    IndexExpr,
    MemberExpr,
    FunctionExpr
>;

/**
 * @brief 表达式基类
 */
struct Expr {
    ExprVariant variant;

    template<typename T>
    explicit Expr(T&& v) : variant(std::forward<T>(v)) {}

    i32 getLine() const;
    i32 getColumn() const;
};

// =====================================================================
// 语句节点（Statements）
// =====================================================================

/**
 * @brief 空语句
 */
struct EmptyStmt {
    i32 line;
    i32 column;
};

/**
 * @brief 赋值语句
 */
struct AssignStmt {
    Vec<ExprPtr> targets;  // 左值列表
    Vec<ExprPtr> values;   // 右值列表
    i32 line;
    i32 column;
};

/**
 * @brief 局部变量声明
 */
struct LocalStmt {
    Vec<Str> names;
    Vec<ExprPtr> values;
    i32 line;
    i32 column;
};

/**
 * @brief 函数调用语句
 */
struct CallStmt {
    ExprPtr call;
    i32 line;
    i32 column;
};

/**
 * @brief if语句
 */
struct IfStmt {
    struct Branch {
        ExprPtr condition;
        Vec<StmtPtr> body;
    };

    Vec<Branch> branches;  // if和elseif分支
    Vec<StmtPtr> elseBranch;  // else分支
    i32 line;
    i32 column;
};

/**
 * @brief while循环
 */
struct WhileStmt {
    ExprPtr condition;
    Vec<StmtPtr> body;
    i32 line;
    i32 column;
};

/**
 * @brief repeat-until循环
 */
struct RepeatStmt {
    Vec<StmtPtr> body;
    ExprPtr condition;
    i32 line;
    i32 column;
};

/**
 * @brief 数值for循环
 */
struct ForNumStmt {
    Str var;
    ExprPtr init;
    ExprPtr limit;
    ExprPtr step;  // 可选，默认为1
    Vec<StmtPtr> body;
    i32 line;
    i32 column;
};

/**
 * @brief 泛型for循环
 */
struct ForInStmt {
    Vec<Str> vars;
    Vec<ExprPtr> iterators;
    Vec<StmtPtr> body;
    i32 line;
    i32 column;
};

/**
 * @brief 函数定义语句
 */
struct FunctionStmt {
    Str name;
    Vec<Str> params;
    bool isVararg;
    Vec<StmtPtr> body;
    bool isLocal;
    i32 line;
    i32 column;
};

/**
 * @brief return语句
 */
struct ReturnStmt {
    Vec<ExprPtr> values;
    i32 line;
    i32 column;
};

/**
 * @brief break语句
 */
struct BreakStmt {
    i32 line;
    i32 column;
};

/**
 * @brief do-end块
 */
struct DoStmt {
    Vec<StmtPtr> body;
    i32 line;
    i32 column;
};

/**
 * @brief 语句的variant类型
 */
using StmtVariant = std::variant<
    EmptyStmt,
    AssignStmt,
    LocalStmt,
    CallStmt,
    IfStmt,
    WhileStmt,
    RepeatStmt,
    ForNumStmt,
    ForInStmt,
    FunctionStmt,
    ReturnStmt,
    BreakStmt,
    DoStmt
>;

/**
 * @brief 语句基类
 */
struct Stmt {
    StmtVariant variant;

    template<typename T>
    explicit Stmt(T&& v) : variant(std::forward<T>(v)) {}

    i32 getLine() const;
    i32 getColumn() const;
};

/**
 * @brief 程序块（Chunk）
 */
struct Chunk {
    Vec<StmtPtr> statements;
};

} // namespace Lua

