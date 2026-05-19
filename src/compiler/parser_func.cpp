/**
 * @file parser_func.cpp
 * @brief Lua Parser function declaration/expression implementation.
 */

#include "parser.hpp"

#include <utility>

namespace Lua {

StmtPtr Parser::parseFunctionStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Function, "Expected 'function'");

    FunctionStmt funcStmt;
    funcStmt.line = line;
    funcStmt.column = column;
    funcStmt.isLocal = false;
    funcStmt.isMethod = false;

    // 解析函数名：支持 name, t.a.b.c.name, t:method
    if (!current_.isName()) {
        error("Expected function name");
    }

    // 第一个名字
    funcStmt.name = tokenString(current_);
    advance();

    // 解析表路径和方法语法
    // function t.a.b.c.foo() 或 function t:method()
    while (check(static_cast<TokenType>('.')) || check(static_cast<TokenType>(':'))) {
        if (match(static_cast<TokenType>('.'))) {
            // 表成员访问
            funcStmt.tablePath.push_back(funcStmt.name);

            if (!current_.isName()) {
                error("Expected field name after '.'");
            }
            funcStmt.name = tokenString(current_);
            advance();
        } else if (match(static_cast<TokenType>(':'))) {
            // 方法定义语法糖
            funcStmt.tablePath.push_back(funcStmt.name);
            funcStmt.isMethod = true;

            if (!current_.isName()) {
                error("Expected method name after ':'");
            }
            funcStmt.name = tokenString(current_);
            advance();
            break;  // 冒号后不能再有点或冒号
        }
    }

    // 解析参数列表
    expect(static_cast<TokenType>('('), "Expected '(' after function name");
    funcStmt.params = parseParamList();
    expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

    // 检查是否有可变参数（最后一个参数是 "..."）
    funcStmt.isVararg = false;
    if (!funcStmt.params.empty() && funcStmt.params.back() == "...") {
        funcStmt.isVararg = true;
        funcStmt.params.pop_back();  // 移除 "..." 参数名
    }

    // 如果是方法定义，自动在参数列表开头添加 self
    if (funcStmt.isMethod) {
        funcStmt.params.insert(funcStmt.params.begin(), "self");
    }

    // 解析函数体
    funcStmt.body = parseBlock();
    expect(TokenType::End, "Expected 'end' to close function");

    return makeStmt<FunctionStmt>(std::move(funcStmt));
}

ExprPtr Parser::parseFunctionExpr() {
    RecursionGuard guard(*this);  // 递归深度保护

    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Function, "Expected 'function'");

    FunctionExpr funcExpr;
    funcExpr.line = line;
    funcExpr.column = column;

    // 解析参数列表
    expect(static_cast<TokenType>('('), "Expected '(' after 'function'");
    funcExpr.params = parseParamList();
    expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

    // 检查是否有可变参数
    funcExpr.isVararg = false;
    if (!funcExpr.params.empty() && funcExpr.params.back() == "...") {
        funcExpr.isVararg = true;
        funcExpr.params.pop_back();  // 移除 "..." 参数名
    }

    // 解析函数体
    funcExpr.body = parseBlock();
    expect(TokenType::End, "Expected 'end' to close function");

    return makeExpr<FunctionExpr>(std::move(funcExpr));
}

Vec<Str> Parser::parseParamList() {
    Vec<Str> params;

    // 空参数列表
    if (check(static_cast<TokenType>(')'))) {
        return params;
    }

    // 变长参数
    if (match(TokenType::Dots)) {
        params.push_back("...");
        return params;
    }

    // 解析参数名
    do {
        if (current_.isName()) {
            params.push_back(tokenString(current_));
            advance();
        } else if (match(TokenType::Dots)) {
            params.push_back("...");
            break;  // ... 必须是最后一个参数
        } else {
            error("Expected parameter name");
        }
    } while (match(static_cast<TokenType>(',')));

    return params;
}

} // namespace Lua
