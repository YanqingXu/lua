/**
 * @file test_statement_emitter.cpp
 * @brief Tests for the CodeGenerator statement lowering boundary.
 */

#include "../framework/test_framework.hpp"
#include "compiler/ast.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/codegen/statement_emitter.hpp"
#include "runtime/runtime_services.hpp"

#include <type_traits>
#include <utility>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Statement Emitter";

}  // namespace

void testStatementEmitterPublicBoundary(TestSuite& suite) {
    using StatementResult =
        decltype(std::declval<StatementEmitter&>().statement(std::declval<const Stmt&>()));
    using BlockResult =
        decltype(std::declval<StatementEmitter&>().block(std::declval<const Vec<StmtPtr>&>()));

    constexpr bool constructibleFromFacade = std::is_constructible_v<StatementEmitter, CodeGenerator&>;
    constexpr bool statementKeepsVoidContract = std::is_same_v<StatementResult, void>;
    constexpr bool blockKeepsVoidContract = std::is_same_v<BlockResult, void>;

    ASSERT_TRUE(suite, constructibleFromFacade,
                "StatementEmitter should be constructible from CodeGenerator facade");
    ASSERT_TRUE(suite, statementKeepsVoidContract,
                "statement should keep the void lowering contract");
    ASSERT_TRUE(suite, blockKeepsVoidContract,
                "block should keep the void lowering contract");
}

void testStatementEmitterHandlesEmptyStatement(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodeGenerator codegen(services);
    StatementEmitter statements(codegen);

    EmptyStmt empty{};
    Stmt stmt(empty);

    bool handledEmptyStatement = true;
    try {
        statements.statement(stmt);
    } catch (...) {
        handledEmptyStatement = false;
    }

    ASSERT_TRUE(suite, handledEmptyStatement, "empty statement should lower without throwing");
}

void registerStatementEmitterTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Public Boundary", testStatementEmitterPublicBoundary);
    registry.registerTest(kSuiteName, "Handles Empty Statement", testStatementEmitterHandlesEmptyStatement);
}
