# Function Definition — 函数定义解析

## 1. 这个模块解决什么问题？

Lua 函数定义的三种形式如何解析，以及方法调用的 self 语法糖。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/parser/parser_func.cpp` | 函数定义解析 |

## 3. 三种函数定义形式

### 形式 1：简单函数定义
```lua
function foo(a, b)
    return a + b
end
```
→ `FunctionStmt { name: "foo", tablePath: [], isLocal: false }`

### 形式 2：表成员函数
```lua
function t.a.b.foo(a, b)
    return a + b
end
```
→ `FunctionStmt { name: "foo", tablePath: ["t", "a", "b"], isLocal: false }`

等价于: `t.a.b.foo = function(a, b) return a + b end`

### 形式 3：方法定义（带 self）
```lua
function t:method(a, b)
    return self.x + a + b
end
```
→ `FunctionStmt { name: "method", tablePath: ["t"], isMethod: true }`

等价于: `t.method = function(self, a, b) return self.x + a + b end`

### 形式 4：局部函数
```lua
local function foo(a, b)
    return a + b
end
```
→ `FunctionStmt { name: "foo", isLocal: true }`

## 4. 方法调用语法糖

```lua
obj:method(arg1, arg2)
-- 等价于
obj.method(obj, arg1, arg2)
```

```lua
obj:method(arg1, arg2)
-- Parser 生成:
CallExpr {
    func: MemberExpr { table: NameExpr("obj"), member: "method" },
    args: [NameExpr("obj"), arg1, arg2],  ← self 自动插入
    isMethodCall: true
}
```

## 5. 匿名函数表达式

```lua
local f = function(a, b)
    return a + b
end
```
→ `FunctionExpr { params: ["a", "b"], body: [...] }`

## 6. 可变参数

```lua
function f(a, b, ...)
    local args = {...}
    return ...
end
```
→ `FunctionExpr/FunctionStmt { params: ["a", "b"], isVararg: true }`

## 7. 常见 Bug

| 问题 | 原因 |
|------|------|
| `local function f()` 中 f 不可见 | 声明顺序：应先登记 f 再编译 body |
| 方法调用 self 未插入 | 冒号语法解析遗漏 |
| 可变参数 `...` 在嵌套函数中 | 每个函数有独立的 vararg |
