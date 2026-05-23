/**
 * @file parser_primary.cpp
 * @brief Lua Parser primary and postfix expression implementation.
 */

#include "parser.hpp"
#include "parser_utils.hpp"

#include <utility>

namespace Lua {

ExprPtr Parser::parsePrimaryExpr() {
    i32 line = current_.line;
    i32 column = current_.column;

    // nil
    if (match(TokenType::Nil)) {
        NilExpr nilExpr;
        nilExpr.line = line;
        nilExpr.column = column;
        return parsePostfixExpr(makeExpr<NilExpr>(std::move(nilExpr)));
    }

    // true
    if (match(TokenType::True)) {
        BoolExpr boolExpr;
        boolExpr.value = true;
        boolExpr.line = line;
        boolExpr.column = column;
        return parsePostfixExpr(makeExpr<BoolExpr>(std::move(boolExpr)));
    }

    // false
    if (match(TokenType::False)) {
        BoolExpr boolExpr;
        boolExpr.value = false;
        boolExpr.line = line;
        boolExpr.column = column;
        return parsePostfixExpr(makeExpr<BoolExpr>(std::move(boolExpr)));
    }

    // 数字
    if (current_.isNumber()) {
        NumberExpr numExpr;
        numExpr.value = std::get<f64>(current_.value);
        numExpr.line = line;
        numExpr.column = column;
        advance();
        return parsePostfixExpr(makeExpr<NumberExpr>(std::move(numExpr)));
    }

    // 字符串
    if (current_.isString()) {
        StringExpr strExpr;
        strExpr.value = Str(ParserUtils::tokenString(current_));
        strExpr.line = line;
        strExpr.column = column;
        advance();
        return parsePostfixExpr(makeExpr<StringExpr>(std::move(strExpr)));
    }

    // ...（变长参数）
    if (match(TokenType::Dots)) {
        VarargExpr varargExpr;
        varargExpr.line = line;
        varargExpr.column = column;
        return parsePostfixExpr(makeExpr<VarargExpr>(std::move(varargExpr)));
    }

    // 表构造器
    if (check(static_cast<TokenType>('{'))) {
        return parsePostfixExpr(parseTableConstructor());
    }

    // 函数定义
    if (check(TokenType::Function)) {
        return parsePostfixExpr(parseFunctionExpr());
    }

    // 括号表达式
    if (match(static_cast<TokenType>('('))) {
        ExprPtr expr = parseExpression();
        expect(static_cast<TokenType>(')'), "Expected ')' after expression");

        ParenExpr parenExpr;
        parenExpr.expression = std::move(expr);
        parenExpr.line = line;
        parenExpr.column = column;
        return parsePostfixExpr(makeExpr<ParenExpr>(std::move(parenExpr)));
    }

    // 标识符
    if (current_.isName()) {
        NameExpr nameExpr;
        nameExpr.name = Str(ParserUtils::tokenString(current_));
        nameExpr.line = line;
        nameExpr.column = column;
        advance();
        return parsePostfixExpr(makeExpr<NameExpr>(std::move(nameExpr)));
    }

    // 使用官方 Lua 风格的错误消息
    error("unexpected symbol");
	return nullptr;  // 永远不会到达
}

ExprPtr Parser::parsePostfixExpr(ExprPtr base) {
    while (true) {
        i32 line = current_.line;
        i32 column = current_.column;

        // 函数调用: func(args)
        if (match(static_cast<TokenType>('('))) {
            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            // 解析参数列表
            if (!check(static_cast<TokenType>(')'))) {
                callExpr.args = parseExprList();
            }

            expect(static_cast<TokenType>(')'), "Expected ')' after arguments");
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        // 索引访问: table[key]
        else if (match(static_cast<TokenType>('['))) {
            IndexExpr indexExpr;
            indexExpr.table = std::move(base);
            indexExpr.index = parseExpression();
            indexExpr.line = line;
            indexExpr.column = column;

            expect(static_cast<TokenType>(']'), "Expected ']' after index");
            base = makeExpr<IndexExpr>(std::move(indexExpr));
        }
        // 成员访问: table.member
        else if (match(static_cast<TokenType>('.'))) {
            if (!current_.isName()) {
                error("Expected member name after '.'");
            }

            MemberExpr memberExpr;
            memberExpr.table = std::move(base);
            memberExpr.member = Str(ParserUtils::tokenString(current_));
            memberExpr.line = line;
            memberExpr.column = column;
            advance();

            base = makeExpr<MemberExpr>(std::move(memberExpr));
        }
        // 方法调用: obj:method(args)
        else if (match(static_cast<TokenType>(':'))) {
            if (!current_.isName()) {
                error("Expected method name after ':'");
            }

            Str methodName(ParserUtils::tokenString(current_));
            advance();

            // 创建成员访问
            MemberExpr memberExpr;
            memberExpr.table = std::move(base);
            memberExpr.member = std::move(methodName);
            memberExpr.line = line;
            memberExpr.column = column;

            ExprPtr method = makeExpr<MemberExpr>(std::move(memberExpr));

            // 创建函数调用
            expect(static_cast<TokenType>('('), "Expected '(' after method name");

            CallExpr callExpr;
            callExpr.func = std::move(method);
            callExpr.isMethodCall = true;  // 标记为方法调用
            callExpr.line = line;
            callExpr.column = column;

            if (!check(static_cast<TokenType>(')'))) {
                callExpr.args = parseExprList();
            }

            expect(static_cast<TokenType>(')'), "Expected ')' after arguments");
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        // 函数调用语法糖: f"string" 等价于 f("string")
        else if (current_.isString()) {
            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            // 创建字符串参数
            StringExpr strExpr;
            strExpr.value = Str(ParserUtils::tokenString(current_));
            strExpr.line = current_.line;
            strExpr.column = current_.column;
            advance();

            callExpr.args.push_back(makeExpr<StringExpr>(std::move(strExpr)));
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        // 函数调用语法糖: f{table} 等价于 f({table})
        else if (check(static_cast<TokenType>('{'))) {
            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            // 解析表构造器作为参数
            callExpr.args.push_back(parseTableConstructor());
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        else {
            break;
        }
    }

    return base;
}

} // namespace Lua
