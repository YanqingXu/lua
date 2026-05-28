/**
 * @file parser_func.cpp
 * @brief Lua函数声明与函数表达式解析实现
 *
 * 实现全局函数、局部函数共享的参数列表解析以及函数体解析流程。
 */

#include "parser_impl.hpp"
#include "parser_utils.hpp"

#include <utility>

namespace Lua {

StmtPtr Parser::Impl::parseFunctionStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Function, "Expected 'function'");

    FunctionStmt funcStmt;
    funcStmt.line = line;
    funcStmt.column = column;
    funcStmt.isLocal = false;
    funcStmt.isMethod = false;

    if (!current().isName()) {
        error("Expected function name");
    }

    funcStmt.name = Str(ParserUtils::tokenString(current()));
    advance();

    while (check(static_cast<TokenType>('.')) || check(static_cast<TokenType>(':'))) {
        if (match(static_cast<TokenType>('.'))) {
            funcStmt.tablePath.push_back(std::move(funcStmt.name));

            if (!current().isName()) {
                error("Expected field name after '.'");
            }
            funcStmt.name = Str(ParserUtils::tokenString(current()));
            advance();
        } else if (match(static_cast<TokenType>(':'))) {
            funcStmt.tablePath.push_back(std::move(funcStmt.name));
            funcStmt.isMethod = true;

            if (!current().isName()) {
                error("Expected method name after ':'");
            }
            funcStmt.name = Str(ParserUtils::tokenString(current()));
            advance();
            break;
        }
    }

    expect(static_cast<TokenType>('('), "Expected '(' after function name");
    funcStmt.params = parseParamList();
    expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

    funcStmt.isVararg = false;
    if (!funcStmt.params.empty() && funcStmt.params.back() == "...") {
        funcStmt.isVararg = true;
        funcStmt.params.pop_back();
    }

    if (funcStmt.isMethod) {
        funcStmt.params.insert(funcStmt.params.begin(), "self");
    }

    enterFunctionSyntaxScope(line, funcStmt.params);
    funcStmt.body = parseBlock();
    leaveFunctionSyntaxScope();
    expect(TokenType::End, "Expected 'end' to close function");

    return makeStmt<FunctionStmt>(std::move(funcStmt));
}

ExprPtr Parser::Impl::parseFunctionExpr() {
    RecursionGuard guard(*this);

    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Function, "Expected 'function'");

    FunctionExpr funcExpr;
    funcExpr.line = line;
    funcExpr.column = column;

    expect(static_cast<TokenType>('('), "Expected '(' after 'function'");
    funcExpr.params = parseParamList();
    expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

    funcExpr.isVararg = false;
    if (!funcExpr.params.empty() && funcExpr.params.back() == "...") {
        funcExpr.isVararg = true;
        funcExpr.params.pop_back();
    }

    enterFunctionSyntaxScope(line, funcExpr.params);
    funcExpr.body = parseBlock();
    leaveFunctionSyntaxScope();
    expect(TokenType::End, "Expected 'end' to close function");

    return makeExpr<FunctionExpr>(std::move(funcExpr));
}

Vec<Str> Parser::Impl::parseParamList() {
    Vec<Str> params;

    if (check(static_cast<TokenType>(')'))) {
        return params;
    }

    if (match(TokenType::Dots)) {
        params.push_back("...");
        return params;
    }

    do {
        if (current().isName()) {
            params.emplace_back(ParserUtils::tokenString(current()));
            advance();
        } else if (match(TokenType::Dots)) {
            params.push_back("...");
            break;
        } else {
            error("Expected parameter name");
        }
    } while (match(static_cast<TokenType>(',')));

    return params;
}

}
