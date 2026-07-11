# Table Constructor — 表构造器解析

## 1. 这个模块解决什么问题？

解析 Lua 的表构造器语法 `{...}`。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/parser/parser_table.cpp` | 表构造器解析 |

## 3. 表构造器形式

### 数组风格
```lua
{1, 2, 3}           -- { [1]=1, [2]=2, [3]=3 }
{1, 2, 3,}          -- 尾部逗号合法
```

### 键值对风格
```lua
{ key = "value" }            -- { ["key"] = "value" }
{ ["key"] = "value" }        -- 显式键
{ [expr] = value }           -- 表达式作为键
```

### 混合风格
```lua
{1, 2, key="value", [expr]=42, 3}
-- 数组部分: 1, 2, 3
-- 哈希部分: key="value", [expr]=42
```

## 4. TableField 结构

```cpp
struct TableField {
    ExprPtr key;     // nil 表示数组部分（自动索引）
    ExprPtr value;   // 值
};

// 数组元素: { value: 42, key: nil }     ← 自动分配索引
// 键值对:   { value: 42, key: "x" }
```

## 5. 解析流程

```
parseTableConstructor():
  expect(LBRACE)
  fields = []
  while (not RBRACE and not EOF):
    if (LBRACK):                   → [expr] = value
      key = parseExpression()
      expect(RBRACK)
      expect(EQUAL)
      value = parseExpression()
      fields.push({key, value})
    else if (NAME and peekNext == EQUAL): → key = value
      key = StringExpr(name)
      expect(EQUAL)
      value = parseExpression()
      fields.push({key, value})
    else:                          → 数组元素
      value = parseExpression()
      fields.push({nil, value})  ← key=nil 表示数组
    if (COMMA): continue
    else if (RBRACE or SEMICOLON): break
  expect(RBRACE)
  return TableExpr{fields}
```

## 6. 多返回值在表构造器中

```lua
function f() return 1, 2, 3 end
local t = { f() }     -- {1, 2, 3}  展开
local t = { f(), 4 }  -- {1, 4}     非末尾只取第一个
```

## 7. 常见 Bug

| 问题 | 原因 |
|------|------|
| 尾部逗号报错 | 解析器不允许尾部逗号 |
| 分号当分隔符报错 | 分号可用作字段分隔符 |
| 数组索引计算错误 | 混合构造器中自动索引需正确累加 |
