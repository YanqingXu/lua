# Statement Parser — 语句解析

## 1. 这个模块解决什么问题？

如何解析 Lua 的各种语句（声明、控制流、赋值等）。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/parser/parser_stmt.cpp` | 所有语句的解析函数 |
| `src/compiler/parser/parser_func.cpp` | 函数定义解析 |
| `src/compiler/parser/parser.cpp` | 调度入口 |

## 3. parseStatement() 调度

```cpp
StmtPtr parseStatement() {
    switch (peekToken().type) {
    case IF:       return parseIfStmt();
    case WHILE:    return parseWhileStmt();
    case DO:       return parseDoStmt();
    case FOR:      return parseForStmt();
    case REPEAT:   return parseRepeatStmt();
    case FUNCTION: return parseFunctionStmt();
    case LOCAL:    return parseLocalStmt();
    case RETURN:   return parseReturnStmt();
    case BREAK:    return parseBreakStmt();
    case LBRACE:   return parseAssignOrCall();  // 表构造器开头的赋值
    default:       return parseAssignOrCall();  // 变量赋值或函数调用
    }
}
```

## 4. 各语句解析要点

### If 语句
```lua
if cond1 then body1
elseif cond2 then body2
else body3
end
```

### While 语句
```lua
while cond do body end
```

### Repeat 语句
```lua
repeat body until cond
-- 注意: repeat 的条件在 body 之后，body 至少执行一次
```

### For 语句（两种）
```lua
-- 数值 for
for var = init, limit, step do body end

-- 泛型 for
for var1, var2 in iterator1, iterator2 do body end
```

### Function 语句（三种形式）
```lua
-- 全局函数
function f() end

-- 表成员函数
function t.a.b.f() end

-- 方法定义（自动加 self）
function t:method() end

-- 局部函数
local function f() end
```

### 赋值语句
```lua
-- 普通赋值
a, b = 1, 2

-- 局部声明
local a, b = 1, 2
-- 注意: Lua 5.1 先求值 RHS，后引入新 local
```

## 5. 常见 Bug

| 问题 | 原因 |
|------|------|
| `function f` vs `local function f` | 绑定规则不同: global 用 SETGLOBAL, local 用寄存器 |
| `break` 只能在循环内 | 需要上下文检查 |
| `return` 必须是块的最后一条语句 | 语法限制 |
| 多重赋值的 LHS/RHS 求值顺序 | 先求所有 RHS，再写入所有 LHS |
