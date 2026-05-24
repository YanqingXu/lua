/**
 * @file parser_table.cpp
 * @brief Lua表构造器解析实现
 *
 * 实现Lua表字段、键值项、数组项、字段分隔符和尾随分隔符解析。
 */

#include "parser_impl.hpp"
#include "parser_utils.hpp"

#include <utility>

namespace Lua {

ExprPtr Parser::Impl::parseTableConstructor() {
    RecursionGuard guard(*this);

    i32 line = current().line;
    i32 column = current().column;

    expect(static_cast<TokenType>('{'), "Expected '{'");

    TableExpr tableExpr;
    tableExpr.line = line;
    tableExpr.column = column;

    while (!check(static_cast<TokenType>('}'))) {
        TableField field;

        if (match(static_cast<TokenType>('['))) {
            field.key = parseExpression();
            expect(static_cast<TokenType>(']'), "Expected ']' after table key");
            expect(static_cast<TokenType>('='), "Expected '=' after table key");
            field.value = parseExpression();
        }
        else if (current().isName()) {
            Token nextToken = peek();

            if (nextToken.type == static_cast<TokenType>('=')) {
                Str name(ParserUtils::tokenString(current()));
                i32 nameLine = current().line;
                i32 nameColumn = current().column;
                advance();

                StringExpr keyExpr;
                keyExpr.value = std::move(name);
                keyExpr.line = nameLine;
                keyExpr.column = nameColumn;
                field.key = makeExpr<StringExpr>(std::move(keyExpr));

                advance();
                field.value = parseExpression();
            } else {
                field.key = nullptr;
                field.value = parseExpression();
            }
        }
        else {
            field.key = nullptr;
            field.value = parseExpression();
        }

        tableExpr.fields.push_back(std::move(field));

        if (!match(static_cast<TokenType>(','))) {
            match(static_cast<TokenType>(';'));
        }

        if (check(static_cast<TokenType>('}'))) {
            break;
        }
    }

    expect(static_cast<TokenType>('}'), "Expected '}' to close table constructor");

    return makeExpr<TableExpr>(std::move(tableExpr));
}

}
