# Lua 5.1 Language Feature Matrix

## 语法特性

| 特性 | 示例 | 主要测试证据 |
|------|------|--------------|
| local variable | `local a = 1` | `locals.lua` |
| global variable | `a = 1` | `literals.lua` |
| function call | `f(1, 2)` | `calls.lua` |
| method call | `obj:m()` | `calls.lua` |
| closure | `return function() end` | `closure.lua` |
| vararg | `...` | `vararg.lua` |
| table constructor | `{1, 2, k=v}` | `literals.lua` |
| if/elseif/else | `if x then ... end` | `constructs.lua` |
| while | `while x do ... end` | `constructs.lua` |
| repeat-until | `repeat ... until x` | `constructs.lua` |
| numeric for | `for i=1,10 do end` | `constructs.lua` |
| generic for | `for k,v in pairs(t) do` | `constructs.lua` |
| break | `break` | `constructs.lua` |
| return | `return 1, 2` | `calls.lua` |
| do-end block | `do ... end` | compiler control-flow tests |
| tail call | `return f()` | `TAILCALL` VM tests |

## 语义特性

| 特性 | 实现机制 |
|------|----------|
| 数值 for 循环变量不重求 init/limit/step | `FORPREP` / `FORLOOP` |
| 闭包捕获变量而非值 | Open/Closed Upvalue 共享 |
| 多返回值规则 | `LUA_MULTRET` 与开放调用结果 |
| 短路求值 `and` / `or` | `TEST` + `JMP` |
| 关系链 `a<b==c>` | AST 展开和短路控制流 |
| metatable `__index` / `__newindex` | table 慢路径 |
| arithmetic metamethods | `__add` 等 TMS 分发 |
| `__call` | callable fallback |
| `__gc` | 两阶段 finalizer |
| weak table | `__mode` 与 GC weak phase |
| coroutine | thread 状态与 yield/resume |
| pcall/xpcall | 保护调用与错误对象传播 |
| getfenv/setfenv | Function environment |
| module/require | package loader 与 registry |
| load/dofile/loadfile/loadstring | parser/codegen 动态加载链 |
