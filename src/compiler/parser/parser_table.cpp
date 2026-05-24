/**
 * @file parser_table.cpp
 * @brief Lua Parser table constructor implementation.
 */

#include "parser_impl.hpp"
#include "parser_utils.hpp"

#include <utility>

namespace Lua {

ExprPtr Parser::Impl::parseTableConstructor() {
    RecursionGuard guard(*this);  // 递归深度保护

    i32 line = current().line;
    i32 column = current().column;

    expect(static_cast<TokenType>('{'), "Expected '{'");

    TableExpr tableExpr;
    tableExpr.line = line;
    tableExpr.column = column;

    while (!check(static_cast<TokenType>('}'))) {
        TableField field;

        // [key] = value
        if (match(static_cast<TokenType>('['))) {
            field.key = parseExpression();
            expect(static_cast<TokenType>(']'), "Expected ']' after table key");
            expect(static_cast<TokenType>('='), "Expected '=' after table key");
            field.value = parseExpression();
        }
        // name = value 或数组元素
        else if (current().isName()) {
            // 使用前瞻判断是 name = value 还是数组元素
            Token nextToken = peek();

            if (nextToken.type == static_cast<TokenType>('=')) {
                // name = value 形式
                Str name(ParserUtils::tokenString(current()));
                i32 nameLine = current().line;
                i32 nameColumn = current().column;
                advance();  // 消费 name

                StringExpr keyExpr;
                keyExpr.value = std::move(name);
                keyExpr.line = nameLine;
                keyExpr.column = nameColumn;
                field.key = makeExpr<StringExpr>(std::move(keyExpr));

                advance();  // 消费 '='
                field.value = parseExpression();
            } else {
                // 数组元素，解析完整表达式
                field.key = nullptr;
                field.value = parseExpression();
            }
        }
        // 数组元素
        else {
            field.key = nullptr;
            field.value = parseExpression();
        }

        tableExpr.fields.push_back(std::move(field));

        // 字段分隔符: , 或 ;
        if (!match(static_cast<TokenType>(','))) {
            match(static_cast<TokenType>(';'));
        }

        // 允许尾随分隔符
        if (check(static_cast<TokenType>('}'))) {
            break;
        }
    }

    expect(static_cast<TokenType>('}'), "Expected '}' to close table constructor");

    return makeExpr<TableExpr>(std::move(tableExpr));
}

} // namespace Lua
