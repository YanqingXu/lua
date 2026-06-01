/**
 * @file parser_primary.cpp
 * @brief Lua基础表达式与后缀表达式解析实现
 *
 * 实现字面量、标识符、括号表达式、表与函数表达式入口以及调用、索引、成员访问解析。
 */

#include "parser_impl.hpp"
#include "parser_utils.hpp"

#include <utility>

namespace Lua {

ExprPtr Parser::Impl::parsePrimaryExpr() {
    i32 line = current().line;
    i32 column = current().column;

    if (match(TokenType::Nil)) {
        NilExpr nilExpr;
        nilExpr.line = line;
        nilExpr.column = column;
        return makeExpr<NilExpr>(std::move(nilExpr));
    }

    if (match(TokenType::True)) {
        BoolExpr boolExpr;
        boolExpr.value = true;
        boolExpr.line = line;
        boolExpr.column = column;
        return makeExpr<BoolExpr>(std::move(boolExpr));
    }

    if (match(TokenType::False)) {
        BoolExpr boolExpr;
        boolExpr.value = false;
        boolExpr.line = line;
        boolExpr.column = column;
        return makeExpr<BoolExpr>(std::move(boolExpr));
    }

    if (current().isNumber()) {
        NumberExpr numExpr;
        numExpr.value = std::get<f64>(current().value);
        numExpr.line = line;
        numExpr.column = column;
        advance();
        return makeExpr<NumberExpr>(std::move(numExpr));
    }

    if (current().isString()) {
        StringExpr strExpr;
        strExpr.value = Str(ParserUtils::tokenString(current()));
        strExpr.line = line;
        strExpr.column = column;
        advance();
        return makeExpr<StringExpr>(std::move(strExpr));
    }

    if (match(TokenType::Dots)) {
        VarargExpr varargExpr;
        varargExpr.line = line;
        varargExpr.column = column;
        return makeExpr<VarargExpr>(std::move(varargExpr));
    }

    if (check(static_cast<TokenType>('{'))) {
        return parseTableConstructor();
    }

    if (check(TokenType::Function)) {
        return parseFunctionExpr();
    }

    if (match(static_cast<TokenType>('('))) {
        ExprPtr expr = parseExpression();
        expect(static_cast<TokenType>(')'), "Expected ')' after expression");

        ParenExpr parenExpr;
        parenExpr.expression = std::move(expr);
        parenExpr.line = line;
        parenExpr.column = column;
        return parsePostfixExpr(makeExpr<ParenExpr>(std::move(parenExpr)));
    }

    if (current().isName()) {
        Token nameToken = current();
        NameExpr nameExpr;
        nameExpr.name = Str(ParserUtils::tokenString(nameToken));
        nameExpr.line = line;
        nameExpr.column = column;
        noteNameUse(nameExpr.name, nameToken);
        advance();
        return parsePostfixExpr(makeExpr<NameExpr>(std::move(nameExpr)));
    }

    error("unexpected symbol");
}

ExprPtr Parser::Impl::parsePostfixExpr(ExprPtr base) {
    while (true) {
        i32 line = current().line;
        i32 column = current().column;

        auto rejectAmbiguousNewlineCall = [&]() {
            if (base && current().line > previous().line) {
                errorAt(current(), "ambiguous syntax (function call x new statement)");
            }
        };

        if (check(static_cast<TokenType>('('))) {
            rejectAmbiguousNewlineCall();
            advance();
            RecursionGuard argumentGuard(*this, MAX_BLOCK_RECURSION_DEPTH);

            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            if (!check(static_cast<TokenType>(')'))) {
                callExpr.args = parseExprList();
            }

            expect(static_cast<TokenType>(')'), "Expected ')' after arguments");
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        else if (match(static_cast<TokenType>('['))) {
            IndexExpr indexExpr;
            indexExpr.table = std::move(base);
            indexExpr.index = parseExpression();
            indexExpr.line = line;
            indexExpr.column = column;

            expect(static_cast<TokenType>(']'), "Expected ']' after index");
            base = makeExpr<IndexExpr>(std::move(indexExpr));
        }
        else if (match(static_cast<TokenType>('.'))) {
            if (!current().isName()) {
                error("Expected member name after '.'");
            }

            MemberExpr memberExpr;
            memberExpr.table = std::move(base);
            memberExpr.member = Str(ParserUtils::tokenString(current()));
            memberExpr.line = line;
            memberExpr.column = column;
            advance();

            base = makeExpr<MemberExpr>(std::move(memberExpr));
        }
        else if (match(static_cast<TokenType>(':'))) {
            if (!current().isName()) {
                error("Expected method name after ':'");
            }

            Str methodName(ParserUtils::tokenString(current()));
            advance();

            MemberExpr memberExpr;
            memberExpr.table = std::move(base);
            memberExpr.member = std::move(methodName);
            memberExpr.line = line;
            memberExpr.column = column;

            CallExpr callExpr;
            callExpr.func = makeExpr<MemberExpr>(std::move(memberExpr));
            callExpr.isMethodCall = true;
            callExpr.line = line;
            callExpr.column = column;

            if (check(static_cast<TokenType>('('))) {
                if (current().line > line) {
                    errorAt(current(), "ambiguous syntax (function call x new statement)");
                }
                advance();
                RecursionGuard argumentGuard(*this, MAX_BLOCK_RECURSION_DEPTH);

                if (!check(static_cast<TokenType>(')'))) {
                    callExpr.args = parseExprList();
                }

                expect(static_cast<TokenType>(')'), "Expected ')' after arguments");
            } else if (current().isString()) {
                if (current().line > line) {
                    errorAt(current(), "ambiguous syntax (function call x new statement)");
                }

                StringExpr strExpr;
                strExpr.value = Str(ParserUtils::tokenString(current()));
                strExpr.line = current().line;
                strExpr.column = current().column;
                advance();

                callExpr.args.push_back(makeExpr<StringExpr>(std::move(strExpr)));
            } else if (check(static_cast<TokenType>('{'))) {
                if (current().line > line) {
                    errorAt(current(), "ambiguous syntax (function call x new statement)");
                }

                callExpr.args.push_back(parseTableConstructor());
            } else {
                error("Expected function arguments after method name");
            }

            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        else if (current().isString()) {
            rejectAmbiguousNewlineCall();

            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            StringExpr strExpr;
            strExpr.value = Str(ParserUtils::tokenString(current()));
            strExpr.line = current().line;
            strExpr.column = current().column;
            advance();

            callExpr.args.push_back(makeExpr<StringExpr>(std::move(strExpr)));
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        else if (check(static_cast<TokenType>('{'))) {
            rejectAmbiguousNewlineCall();

            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            callExpr.args.push_back(parseTableConstructor());
            base = makeExpr<CallExpr>(std::move(callExpr));
        }
        else {
            break;
        }
    }

    return base;
}

}
