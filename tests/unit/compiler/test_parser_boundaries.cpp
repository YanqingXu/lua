/**
 * @file test_parser_boundaries.cpp
 * @brief Parser behavior sentinels for the physical split roadmap task.
 *
 * These tests intentionally inspect AST shapes instead of VM results. They
 * lock the parser boundaries that will be moved into separate implementation
 * files in the next optimization step.
 */

#include "../framework/test_framework.hpp"

#include "common/lua_error.hpp"
#include "common/types.hpp"
#include "compiler/ast.hpp"
#include "compiler/lexer.hpp"

#include <expected>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

// This boundary sentinel needs to assert the private helper return type without
// widening Parser's production interface.
#define private public
#include "compiler/parser.hpp"
#undef private

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Parser Boundary Sentinels";

bool parseChunk(TestSuite& suite, const char* source, Chunk& out, const char* testName) {
    Parser parser(source);
    auto parsed = parser.parse();
    if (!parsed) {
        ASSERT_TRUE(suite, false, testName);
        return false;
    }

    out = std::move(*parsed);
    return true;
}

template<typename T>
const T* asStmt(const StmtPtr& stmt) {
    if (!stmt) return nullptr;
    return std::get_if<T>(&stmt->variant);
}

template<typename T>
const T* asExpr(const ExprPtr& expr) {
    if (!expr) return nullptr;
    return std::get_if<T>(&expr->variant);
}

const BinaryExpr* expectBinary(TestSuite& suite,
                               const ExprPtr& expr,
                               BinaryExpr::Op op,
                               const char* testName) {
    const BinaryExpr* bin = asExpr<BinaryExpr>(expr);
    ASSERT_TRUE(suite, bin != nullptr, testName);
    if (!bin) return nullptr;

    ASSERT_TRUE(suite, bin->op == op, testName);
    return bin;
}

void expectStringKey(TestSuite& suite,
                     const TableField& field,
                     const char* expected,
                     const char* testName) {
    const StringExpr* key = asExpr<StringExpr>(field.key);
    ASSERT_TRUE(suite, key != nullptr, testName);
    if (!key) return;

    ASSERT_TRUE(suite, key->value == expected, testName);
}

void expectParseError(TestSuite& suite,
                      const char* source,
                      const char* expectedText,
                      const char* testName) {
    Parser parser(source);
    auto parsed = parser.parse();
    if (parsed) {
        ASSERT_TRUE(suite, false, testName);
        return;
    }

    const ParseError& e = parsed.error();
    std::string message = e.what();
    ASSERT_TRUE(suite, !message.empty(), testName);
    ASSERT_TRUE(suite, message.find(expectedText) != std::string::npos, testName);
    ASSERT_TRUE(suite, e.getLine() >= 1 && e.getColumn() >= 0, testName);
}

void testStatementBoundaryFamilies(TestSuite& suite) {
    Chunk chunk;
    bool ok = parseChunk(suite, R"lua(
        local total = 0
        if total == 0 then
            total = 1
        elseif total == 1 then
            total = 2
        else
            total = 3
        end

        while total < 5 do
            total = total + 1
        end

        repeat
            total = total - 1
        until total == 0

        for i = 1, 3, 1 do
            total = total + i
        end

        for k, v in pairs({ a = 1 }) do
            total = total + v
        end

        do
            local scoped = total
        end
    )lua", chunk, "statement families parse");
    if (!ok) return;

    ASSERT_TRUE(suite, chunk.statements.size() == 7, "top-level statement count is stable");

    const LocalStmt* localStmt = asStmt<LocalStmt>(chunk.statements[0]);
    ASSERT_TRUE(suite, localStmt != nullptr && localStmt->names.size() == 1 &&
                localStmt->values.size() == 1, "local statement shape is stable");

    const IfStmt* ifStmt = asStmt<IfStmt>(chunk.statements[1]);
    ASSERT_TRUE(suite, ifStmt != nullptr && ifStmt->branches.size() == 2 &&
                ifStmt->elseBranch.size() == 1, "if/elseif/else shape is stable");

    const WhileStmt* whileStmt = asStmt<WhileStmt>(chunk.statements[2]);
    ASSERT_TRUE(suite, whileStmt != nullptr && whileStmt->body.size() == 1,
                "while statement shape is stable");

    const RepeatStmt* repeatStmt = asStmt<RepeatStmt>(chunk.statements[3]);
    ASSERT_TRUE(suite, repeatStmt != nullptr && repeatStmt->body.size() == 1 &&
                repeatStmt->condition != nullptr, "repeat statement shape is stable");

    const ForNumStmt* forNum = asStmt<ForNumStmt>(chunk.statements[4]);
    ASSERT_TRUE(suite, forNum != nullptr && forNum->var == "i" &&
                forNum->init != nullptr && forNum->limit != nullptr &&
                forNum->step != nullptr, "numeric for statement shape is stable");

    const ForInStmt* forIn = asStmt<ForInStmt>(chunk.statements[5]);
    ASSERT_TRUE(suite, forIn != nullptr && forIn->vars.size() == 2 &&
                forIn->vars[0] == "k" && forIn->vars[1] == "v" &&
                forIn->iterators.size() == 1, "generic for statement shape is stable");

    const DoStmt* doStmt = asStmt<DoStmt>(chunk.statements[6]);
    ASSERT_TRUE(suite, doStmt != nullptr && doStmt->body.size() == 1,
                "do block statement shape is stable");
}

void testExpressionPrecedenceBoundary(TestSuite& suite) {
    Chunk chunk;
    bool ok = parseChunk(suite,
        "local result = a or b and c < d .. e + f * g ^ h",
        chunk,
        "complex expression parses");
    if (!ok) return;

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "expression sentinel has one statement");
    const LocalStmt* localStmt = asStmt<LocalStmt>(chunk.statements[0]);
    ASSERT_TRUE(suite, localStmt != nullptr && localStmt->values.size() == 1,
                "expression sentinel local value exists");
    if (!localStmt || localStmt->values.empty()) return;

    const BinaryExpr* orExpr = expectBinary(suite, localStmt->values[0],
                                            BinaryExpr::Op::Or,
                                            "or remains lowest precedence");
    if (!orExpr) return;

    const BinaryExpr* andExpr = expectBinary(suite, orExpr->right,
                                             BinaryExpr::Op::And,
                                             "and binds tighter than or");
    if (!andExpr) return;

    const BinaryExpr* relExpr = expectBinary(suite, andExpr->right,
                                             BinaryExpr::Op::Lt,
                                             "relational expression boundary is stable");
    if (!relExpr) return;

    const BinaryExpr* concatExpr = expectBinary(suite, relExpr->right,
                                                BinaryExpr::Op::Concat,
                                                "concat binds inside relational right operand");
    if (!concatExpr) return;

    const BinaryExpr* addExpr = expectBinary(suite, concatExpr->right,
                                             BinaryExpr::Op::Add,
                                             "additive expression nests under concat");
    if (!addExpr) return;

    const BinaryExpr* mulExpr = expectBinary(suite, addExpr->right,
                                             BinaryExpr::Op::Mul,
                                             "multiplicative expression nests under additive");
    if (!mulExpr) return;

    expectBinary(suite, mulExpr->right, BinaryExpr::Op::Pow,
                 "power expression remains right associative");
}

void testFunctionTableAndPostfixBoundaries(TestSuite& suite) {
    Chunk chunk;
    bool ok = parseChunk(suite, R"lua(
        function api.tools:run(a, ...)
            local packed = {
                [a] = function(x) return x end;
                name = "runner",
                api.tools.helper("x"),
                trailing = { 1; 2, 3, }
            }
            return self:finish(packed).value
        end
    )lua", chunk, "function/table/postfix sentinel parses");
    if (!ok) return;

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "function sentinel has one statement");
    const FunctionStmt* func = asStmt<FunctionStmt>(chunk.statements[0]);
    ASSERT_TRUE(suite, func != nullptr, "function statement shape is stable");
    if (!func) return;

    ASSERT_TRUE(suite, func->name == "run", "method function name is stable");
    ASSERT_TRUE(suite, func->tablePath.size() == 2 &&
                func->tablePath[0] == "api" &&
                func->tablePath[1] == "tools", "method table path is stable");
    ASSERT_TRUE(suite, func->isMethod, "method flag is stable");
    ASSERT_TRUE(suite, func->isVararg, "vararg method flag is stable");
    ASSERT_TRUE(suite, func->params.size() == 2 &&
                func->params[0] == "self" &&
                func->params[1] == "a", "method params include self and fixed args");
    ASSERT_TRUE(suite, func->body.size() == 2, "function body statement count is stable");

    const LocalStmt* localStmt = asStmt<LocalStmt>(func->body[0]);
    ASSERT_TRUE(suite, localStmt != nullptr && localStmt->values.size() == 1,
                "function body local table exists");
    if (!localStmt || localStmt->values.empty()) return;

    const TableExpr* table = asExpr<TableExpr>(localStmt->values[0]);
    ASSERT_TRUE(suite, table != nullptr && table->fields.size() == 4,
                "table constructor field count is stable");
    if (!table || table->fields.size() != 4) return;

    ASSERT_TRUE(suite, asExpr<NameExpr>(table->fields[0].key) != nullptr,
                "bracket table field key is an expression");
    ASSERT_TRUE(suite, asExpr<FunctionExpr>(table->fields[0].value) != nullptr,
                "function expression table field is stable");

    expectStringKey(suite, table->fields[1], "name",
                    "name=value table field key is stable");
    ASSERT_TRUE(suite, asExpr<StringExpr>(table->fields[1].value) != nullptr,
                "name=value table field value is stable");

    ASSERT_TRUE(suite, table->fields[2].key == nullptr &&
                asExpr<CallExpr>(table->fields[2].value) != nullptr,
                "array table field can hold postfix call");

    expectStringKey(suite, table->fields[3], "trailing",
                    "trailing table field key is stable");
    const TableExpr* nested = asExpr<TableExpr>(table->fields[3].value);
    ASSERT_TRUE(suite, nested != nullptr && nested->fields.size() == 3,
                "nested table constructor tolerates mixed separators and trailing separator");

    const ReturnStmt* ret = asStmt<ReturnStmt>(func->body[1]);
    ASSERT_TRUE(suite, ret != nullptr && ret->values.size() == 1,
                "function body return shape is stable");
    ASSERT_TRUE(suite, ret != nullptr &&
                asExpr<MemberExpr>(ret->values[0]) != nullptr,
                "postfix member after method call remains stable");
}

void testParserErrorBoundaries(TestSuite& suite) {
    expectParseError(suite, "1 + 2", "unexpected symbol",
                     "standalone non-call expression remains invalid statement");
    expectParseError(suite, "function missing(a)\n  return a\n", "Expected 'end'",
                     "missing function end reports expected end");
    expectParseError(suite, "local t = { name = }", "unexpected symbol",
                     "malformed table field reports unexpected symbol");
}

void testTokenStringReturnsBorrowedView(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(Parser::tokenString(std::declval<const Token&>())), StrView>);

    Token nameToken(TokenType::Name, "identifier", 1, 1);
    StrView nameView = Parser::tokenString(nameToken);
    ASSERT_TRUE(suite, nameView == "identifier", "name token string should expose lexeme text");
    ASSERT_TRUE(suite, nameView.data() == nameToken.lexeme.data(),
                "name token string should borrow the token lexeme storage");

    Token stringToken(TokenType::String, "\"literal\"", 1, 1);
    stringToken.value = Str("literal");
    const Str& stringValue = std::get<Str>(stringToken.value);
    StrView stringView = Parser::tokenString(stringToken);
    ASSERT_TRUE(suite, stringView == "literal", "string token string should expose decoded value");
    ASSERT_TRUE(suite, stringView.data() == stringValue.data(),
                "string token string should borrow the token value storage");
}

} // namespace

void registerParserBoundaryTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "statement families", testStatementBoundaryFamilies);
    registry.registerTest(kSuiteName, "expression precedence", testExpressionPrecedenceBoundary);
    registry.registerTest(kSuiteName, "function table postfix", testFunctionTableAndPostfixBoundaries);
    registry.registerTest(kSuiteName, "error boundaries", testParserErrorBoundaries);
    registry.registerTest(kSuiteName, "tokenString returns borrowed view", testTokenStringReturnsBorrowedView);
}
