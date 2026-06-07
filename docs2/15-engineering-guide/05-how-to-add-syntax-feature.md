# How to Add a Syntax Feature — 如何新增语法特性

## 完整流程

```
1. Token (如果需要新 Token)
   src/compiler/parser/token.hpp
   src/compiler/lexer/lexer.cpp

2. AST 节点
   src/compiler/ast.hpp
   src/compiler/ast.cpp

3. Parser
   src/compiler/parser/parser_stmt.cpp (或 parser_expr.cpp)

4. AST Visitor (更新)
   src/compiler/ast_visitor.hpp

5. CodeGen / Bytecode
   src/compiler/codegen/codegen_stmt.cpp (或 expression_emitter.cpp)
   可能需要新指令 → src/compiler/opcode.hpp

6. VM
   src/vm/vm.cpp (switch) 或 vm_handlers/

7. Test
   tests/unit/compiler/test_*.cpp
   或 tests/lua/regressions/*.lua
```

## 示例: 新增 `unless` 语法

```lua
-- 目标: unless cond then body end  (等价于 if not cond then body end)
```

### 1. Token (不需要新 Token, 复用 IF/THEN/END)

### 2. AST 节点 (不需要新节点, 复用 IfStmt 但反转条件)

### 3. Parser
```cpp
// parser_stmt.cpp
StmtPtr Parser::parseUnlessStmt() {
    consume(UNLESS);  // 消费 unless 关键字
    ExprPtr cond = parseExpression();
    consume(THEN);
    Vec<StmtPtr> body = parseBlock();
    consume(END);
    
    // 反转条件: unless cond ≡ if not cond
    ExprPtr negCond = makeExpr(UnaryExpr{UnaryExpr::Op::Not, std::move(cond)});
    return makeStmt(IfStmt{{negCond, body}, {}, 0});
}
```

### 4. CodeGen (复用 if 的编译逻辑)

### 5. VM (不需要修改)

### 6. Test
```lua
unless false then
    print("executed")  -- 应该执行
end
```
