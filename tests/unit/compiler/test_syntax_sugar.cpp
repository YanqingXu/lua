/**
 * @file test_syntax_sugar.cpp
 * @brief 测试Lua 5.1.5语法糖功能
 * 
 * 测试以下语法糖：
 * 1. 方法定义: function t:method() end
 * 2. 表成员函数定义: function t.a.b.c.f() end
 * 3. 函数调用字符串语法糖: f"string"
 * 4. 函数调用表语法糖: f{table}
 * 5. 表字段前瞻解析: {name, age=25}
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser.hpp"
#include "compiler/ast.hpp"
#include <cassert>
#include <string>

using namespace Lua;
using namespace LuaTest;

/**
 * @brief 测试方法定义语法糖
 * 
 * function obj:method(a, b) end
 * 应该等价于 function obj.method(self, a, b) end
 */
void testMethodDefinition(TestSuite& suite) {
    const char* code = R"(
        function obj:method(a, b)
            return a + b
        end
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* funcStmt = std::get_if<FunctionStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, funcStmt != nullptr, "Statement is FunctionStmt");
    ASSERT_TRUE(suite, funcStmt->name == "method", "Function name is 'method'");
    ASSERT_TRUE(suite, funcStmt->tablePath.size() == 1, "Table path has 1 element");
    ASSERT_TRUE(suite, funcStmt->tablePath[0] == "obj", "Table path is 'obj'");
    ASSERT_TRUE(suite, funcStmt->isMethod == true, "Is method definition");
    ASSERT_TRUE(suite, funcStmt->params.size() == 3, "Has 3 params (self, a, b)");
    ASSERT_TRUE(suite, funcStmt->params[0] == "self", "First param is 'self'");
    ASSERT_TRUE(suite, funcStmt->params[1] == "a", "Second param is 'a'");
    ASSERT_TRUE(suite, funcStmt->params[2] == "b", "Third param is 'b'");
}

/**
 * @brief 测试表成员函数定义
 * 
 * function lib.math.utils.square(x) end
 */
void testTableMemberFunction(TestSuite& suite) {
    const char* code = R"(
        function lib.math.utils.square(x)
            return x * x
        end
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* funcStmt = std::get_if<FunctionStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, funcStmt != nullptr, "Statement is FunctionStmt");
    ASSERT_TRUE(suite, funcStmt->name == "square", "Function name is 'square'");
    ASSERT_TRUE(suite, funcStmt->tablePath.size() == 3, "Table path has 3 elements");
    ASSERT_TRUE(suite, funcStmt->tablePath[0] == "lib", "First path element is 'lib'");
    ASSERT_TRUE(suite, funcStmt->tablePath[1] == "math", "Second path element is 'math'");
    ASSERT_TRUE(suite, funcStmt->tablePath[2] == "utils", "Third path element is 'utils'");
    ASSERT_TRUE(suite, funcStmt->isMethod == false, "Not a method definition");
    ASSERT_TRUE(suite, funcStmt->params.size() == 1, "Has 1 param");
    ASSERT_TRUE(suite, funcStmt->params[0] == "x", "Param is 'x'");
}

/**
 * @brief 测试简单函数定义（无表路径）
 */
void testSimpleFunctionDefinition(TestSuite& suite) {
    const char* code = R"(
        function foo(a, b)
            return a + b
        end
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* funcStmt = std::get_if<FunctionStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, funcStmt != nullptr, "Statement is FunctionStmt");
    ASSERT_TRUE(suite, funcStmt->name == "foo", "Function name is 'foo'");
    ASSERT_TRUE(suite, funcStmt->tablePath.size() == 0, "No table path");
    ASSERT_TRUE(suite, funcStmt->isMethod == false, "Not a method");
}

/**
 * @brief 测试函数调用字符串语法糖
 * 
 * f"string" 应该等价于 f("string")
 */
void testFunctionCallStringSugar(TestSuite& suite) {
    const char* code = R"(
        local x = print"Hello, World!"
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* localStmt = std::get_if<LocalStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, localStmt != nullptr, "Statement is LocalStmt");
    ASSERT_TRUE(suite, localStmt->values.size() == 1, "Has one value");

    auto* callExpr = std::get_if<CallExpr>(&localStmt->values[0]->variant);
    ASSERT_TRUE(suite, callExpr != nullptr, "Value is CallExpr");
    ASSERT_TRUE(suite, callExpr->args.size() == 1, "Has one argument");

    auto* strExpr = std::get_if<StringExpr>(&callExpr->args[0]->variant);
    ASSERT_TRUE(suite, strExpr != nullptr, "Argument is StringExpr");
    ASSERT_TRUE(suite, strExpr->value == "Hello, World!", "String value is correct");
}

/**
 * @brief 测试函数调用表语法糖
 * 
 * f{table} 应该等价于 f({table})
 */
void testFunctionCallTableSugar(TestSuite& suite) {
    const char* code = R"(
        local p = create_point{x=10, y=20}
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* localStmt = std::get_if<LocalStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, localStmt != nullptr, "Statement is LocalStmt");
    ASSERT_TRUE(suite, localStmt->values.size() == 1, "Has one value");

    auto* callExpr = std::get_if<CallExpr>(&localStmt->values[0]->variant);
    ASSERT_TRUE(suite, callExpr != nullptr, "Value is CallExpr");
    ASSERT_TRUE(suite, callExpr->args.size() == 1, "Has one argument");

    auto* tableExpr = std::get_if<TableExpr>(&callExpr->args[0]->variant);
    ASSERT_TRUE(suite, tableExpr != nullptr, "Argument is TableExpr");
    ASSERT_TRUE(suite, tableExpr->fields.size() == 2, "Table has 2 fields");
}

/**
 * @brief 测试表字段前瞻解析
 *
 * 测试表构造器中正确区分数组元素和命名字段
 */
void testTableFieldLookahead(TestSuite& suite) {
    const char* code = R"(
        local t = {
            name,
            age = 25,
            func(),
            [key] = value
        }
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* localStmt = std::get_if<LocalStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, localStmt != nullptr, "Statement is LocalStmt");
    ASSERT_TRUE(suite, localStmt->values.size() == 1, "Has one value");

    auto* tableExpr = std::get_if<TableExpr>(&localStmt->values[0]->variant);
    ASSERT_TRUE(suite, tableExpr != nullptr, "Value is TableExpr");
    ASSERT_TRUE(suite, tableExpr->fields.size() == 4, "Table has 4 fields");

    // 第一个字段: name (数组元素)
    ASSERT_TRUE(suite, tableExpr->fields[0].key == nullptr, "Field 1: array element (no key)");
    auto* nameExpr = std::get_if<NameExpr>(&tableExpr->fields[0].value->variant);
    ASSERT_TRUE(suite, nameExpr != nullptr, "Field 1: is NameExpr");

    // 第二个字段: age = 25 (命名字段)
    ASSERT_TRUE(suite, tableExpr->fields[1].key != nullptr, "Field 2: has key");
    auto* keyExpr = std::get_if<StringExpr>(&tableExpr->fields[1].key->variant);
    ASSERT_TRUE(suite, keyExpr != nullptr, "Field 2: key is StringExpr");

    // 第三个字段: func() (数组元素)
    ASSERT_TRUE(suite, tableExpr->fields[2].key == nullptr, "Field 3: array element (no key)");
    auto* callExpr = std::get_if<CallExpr>(&tableExpr->fields[2].value->variant);
    ASSERT_TRUE(suite, callExpr != nullptr, "Field 3: is CallExpr");

    // 第四个字段: [key] = value (索引字段)
    ASSERT_TRUE(suite, tableExpr->fields[3].key != nullptr, "Field 4: has key");
}

/**
 * @brief 测试复杂表达式作为数组元素
 */
void testComplexArrayElement(TestSuite& suite) {
    const char* code = R"(
        local t = {
            a + b,
            func(x, y),
            obj.method(),
            1 + 2 * 3
        }
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* localStmt = std::get_if<LocalStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, localStmt != nullptr, "Statement is LocalStmt");

    auto* tableExpr = std::get_if<TableExpr>(&localStmt->values[0]->variant);
    ASSERT_TRUE(suite, tableExpr != nullptr, "Value is TableExpr");
    ASSERT_TRUE(suite, tableExpr->fields.size() == 4, "Table has 4 fields");

    // 所有字段都应该是数组元素（没有key）
    for (size_t i = 0; i < tableExpr->fields.size(); ++i) {
        ASSERT_TRUE(suite, tableExpr->fields[i].key == nullptr,
                   "Field " + std::to_string(i + 1) + " is array element");
    }
}

/**
 * @brief 测试混合表构造器
 */
void testMixedTableConstructor(TestSuite& suite) {
    const char* code = R"(
        local t = {
            1, 2, 3,
            name = "test",
            [10] = "ten",
            4, 5,
            key = "value"
        }
    )";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    ASSERT_TRUE(suite, chunk.statements.size() == 1, "One statement parsed");

    auto* localStmt = std::get_if<LocalStmt>(&chunk.statements[0]->variant);
    ASSERT_TRUE(suite, localStmt != nullptr, "Statement is LocalStmt");

    auto* tableExpr = std::get_if<TableExpr>(&localStmt->values[0]->variant);
    ASSERT_TRUE(suite, tableExpr != nullptr, "Value is TableExpr");
    ASSERT_TRUE(suite, tableExpr->fields.size() == 8, "Table has 8 fields");

    // 验证数组元素（1, 2, 3, 4, 5）
    ASSERT_TRUE(suite, tableExpr->fields[0].key == nullptr, "Field 1: array element");
    ASSERT_TRUE(suite, tableExpr->fields[1].key == nullptr, "Field 2: array element");
    ASSERT_TRUE(suite, tableExpr->fields[2].key == nullptr, "Field 3: array element");

    // 验证命名字段（name = "test"）
    ASSERT_TRUE(suite, tableExpr->fields[3].key != nullptr, "Field 4: has key");

    // 验证索引字段（[10] = "ten"）
    ASSERT_TRUE(suite, tableExpr->fields[4].key != nullptr, "Field 5: has key");

    // 验证更多数组元素（4, 5）
    ASSERT_TRUE(suite, tableExpr->fields[5].key == nullptr, "Field 6: array element");
    ASSERT_TRUE(suite, tableExpr->fields[6].key == nullptr, "Field 7: array element");

    // 验证命名字段（key = "value"）
    ASSERT_TRUE(suite, tableExpr->fields[7].key != nullptr, "Field 8: has key");
}

/**
 * @brief 注册所有语法糖测试
 */
void registerSyntaxSugarTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Syntax Sugar", "Method Definition", testMethodDefinition);
    registry.registerTest("Syntax Sugar", "Table Member Function", testTableMemberFunction);
    registry.registerTest("Syntax Sugar", "Simple Function Definition", testSimpleFunctionDefinition);
    registry.registerTest("Syntax Sugar", "Function Call String Sugar", testFunctionCallStringSugar);
    registry.registerTest("Syntax Sugar", "Function Call Table Sugar", testFunctionCallTableSugar);
    registry.registerTest("Syntax Sugar", "Table Field Lookahead", testTableFieldLookahead);
    registry.registerTest("Syntax Sugar", "Complex Array Element", testComplexArrayElement);
    registry.registerTest("Syntax Sugar", "Mixed Table Constructor", testMixedTableConstructor);
}


