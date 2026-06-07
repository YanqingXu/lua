# Lua 5.1 Language Feature Matrix

## 语法特性

| 特性 | 示例 | 状态 | 测试 |
|------|------|------|------|
| local variable | `local a = 1` | ✅ done | `locals.lua` |
| global variable | `a = 1` | ✅ done | `literals.lua` |
| function call | `f(1, 2)` | ✅ done | `calls.lua` |
| method call | `obj:m()` | ✅ done | `calls.lua` |
| closure | `return function() end` | ✅ done | `closure.lua` |
| vararg | `...` | ✅ done | `vararg.lua` |
| table constructor | `{1, 2, k=v}` | ✅ done | `literals.lua` |
| if/elseif/else | `if x then ... end` | ✅ done | `constructs.lua` |
| while | `while x do ... end` | ✅ done | `constructs.lua` |
| repeat-until | `repeat ... until x` | ✅ done | `constructs.lua` |
| numeric for | `for i=1,10 do end` | ✅ done | `constructs.lua` |
| generic for | `for k,v in pairs(t) do` | ✅ done | `constructs.lua` |
| break | `break` | ✅ done | `constructs.lua` |
| return | `return 1, 2` | ✅ done | `calls.lua` |
| do-end block | `do ... end` | ✅ done | — |
| tail call | `return f()` | ✅ done (TAILCALL) | — |

## 语义特性

| 特性 | 状态 | 备注 |
|------|------|------|
| 数值 for 循环变量不重求 init/limit/step | ✅ | FORPREP/FORLOOP |
| 闭包捕获变量 (非值) | ✅ | Upvalue 共享 |
| 多返回值规则 | ✅ | LUA_MULTRET |
| 短路求值 `and`/`or` | ✅ | TEST + JMP |
| 关系链 `a<b==c>` | ✅ | 展开为 and |
| metatable (__index/__newindex) | ✅ | |
| arithmetic metamethods | ✅ | __add 等 |
| __call | ✅ | |
| __gc | ✅ | 两阶段 finalizer |
| weak table | ✅ | __mode |
| coroutine (yield/resume) | ✅ | |
| pcall/xpcall | ✅ | |
| getfenv/setfenv | ✅ | |
| module/require | ✅ | |
| load/dofile/loadfile/loadstring | ✅ | |
