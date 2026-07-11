# Native Call — C 函数调用

## 1. 这个模块解决什么问题？

VM 如何调用 C 函数（标准库函数、通过 C API 注册的函数）。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm_call.cpp` | C 函数调用实现 |
| `src/api/lapi.cpp` | C API 实现 |
| `src/lib/*.cpp` | 各标准库 (都是 C 函数) |

## 3. C 函数签名

```cpp
// C 函数类型
using lua_CFunction = int (*)(lua_State* L);

// C 函数接收 LuaState，通过栈操作获取参数和返回结果
// 返回值: 推入栈的结果数量

int myFunction(lua_State* L) {
    // 从栈获取参数
    f64 a = lua_tonumber(L, 1);
    f64 b = lua_tonumber(L, 2);
    
    // 计算结果
    f64 result = a + b;
    
    // 推入返回值
    lua_pushnumber(L, result);
    
    // 返回结果数量
    return 1;
}
```

## 4. C 函数调用栈布局

```
C 函数被调用时的栈:

  [C 函数对象]          ← ci.func
  [arg1]                 ← 参数1 (lua 栈索引 1)
  [arg2]                 ← 参数2 (栈索引 2)
  [arg3]                 ← 参数3 (栈索引 3)
  ...                    ← 更多参数
  [nil] ... [nil]        ← 预留空间 (maxStackSize)

C 函数返回后的栈:

  [ret1]                 ← 返回值1 (覆盖了 C 函数对象)
  [ret2]                 ← 返回值2
  [ret3]                 ← 返回值3
  [nil]                  ← 补充的 nil (如果 nresults > actualResults)
```

## 5. C Closure 结构

```cpp
// C Closure: 包装 C 函数 + upvalues
Closure* createC(lua_CFunction func, Vec<Upvalue*> upvalues) {
    Closure* cl = new Closure();
    cl->cFunc = func;
    cl->upvalues = upvalues;
    return cl;
}

// C 闭包的 upvalue 通过 lua_upvalueindex 访问
// 例如: lua_tostring(L, lua_upvalueindex(1))
```

## 6. C → Lua 重入保护

```
C 函数可能回调 Lua (如元方法、pcall handler):

C 函数 → VM::call() → Lua 函数 → 可能再次进入 C 函数

重入保护:
  - 每次重入增加 nexeccalls
  - 超过 MAX_CALLS 抛出 "stack overflow"
  - VM 侧检测 C/C++→Lua 重入深度
```

## 7. C 函数注册

```cpp
// 注册到全局表
lua_pushcfunction(L, myFunction);
lua_setglobal(L, "myFunc");

// 或通过标准库注册
void openMyLib(LuaState* L) {
    lua_register(L, "myFunc", myFunction);
}
```
