# Parser Test Cases — 解析器测试用例

## 1. 这个模块解决什么问题？

验证 Lexer 和 Parser 的正确性的测试用例集合。

## 2. 核心测试文件

| 测试文件 | 测试内容 |
|---------|---------|
| `tests/unit/compiler/test_lexer_number.cpp` | 数字词法 |
| `tests/unit/compiler/test_lexer_lookahead.cpp` | LL(1) 前瞻 |
| `tests/unit/compiler/test_parser_boundaries.cpp` | 解析器边界条件 |
| `tests/unit/compiler/test_parser_recursion.cpp` | 递归深度 |
| `tests/unit/compiler/test_parser_error_recovery.cpp` | 错误恢复 |
| `tests/unit/compiler/test_parser_memory_pool.cpp` | 内存管理 |
| `tests/unit/compiler/test_syntax_sugar.cpp` | 语法糖 |
| `tests/unit/compiler/test_binary_unary_expr.cpp` | 二元/一元表达式 |

## 3. Lexer 测试关键覆盖

```lua
-- 数字
42
3.14
.5
1.
1e10
1.5e-3
0xFF
0x1.2p3

-- 字符串
"hello"
'world'
[[multi line]]
[=[level 1]=]

-- 注释
-- single line
--[[ multi line ]]
```

## 4. Parser 测试关键覆盖

```lua
-- 变量声明
local x = 1
local a, b = 1, 2

-- 赋值
a = 1
a, b = 1, 2
t.key = value
t[key] = value

-- 表达式
a + b * c
(a + b) * c
not a and b or c
a < b == c > d  -- 关系链

-- 方法调用
obj:method()
obj:method(arg1, arg2)

-- 函数
function f(a, b) return a+b end
local function f() end
function t:f() end
```

## 5. 运行测试

```bash
# 编译
bin/build_test.bat

# 运行全部测试
bin/lua_test.exe

# 运行特定测试套件
bin/lua_test.exe --suite Lexer
bin/lua_test.exe --suite Parser
```
