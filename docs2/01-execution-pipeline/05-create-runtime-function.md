# Create Runtime Function — 创建运行时函数

## 1. 这个模块解决什么问题？

将编译产生的 Proto 包装为运行时 Closure（闭包），准备进入 VM 执行。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → Create Function → VM Execute
                                                ↑
                                           (第五阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/function.cpp` | Proto / Closure / Function 实现 |
| `src/core/function.hpp` | 类型定义 |

## 4. 从 Proto 到可执行函数

```cpp
// 编译得到 Proto
Proto* proto = codegen.generate(chunk);

// 创建 Lua Closure（包装 Proto + upvalues）
Closure* closure = Closure::createLua(proto, upvalues);

// 或创建 C Closure（包装 C++ 函数指针）
Closure* cclosure = Closure::createC(cFunctionPointer, upvalues);

// 包装为 Function（GCObject）
Function* func = Function::create(closure);
```

## 5. Function 类型

```cpp
class Function : public GCObject {
    enum FunctionType {
        LuaFunction,    // Lua 闭包
        CFunction       // C 函数
    };
    
    union {
        Closure* luaClosure;   // Lua 闭包
        lua_CFunction cFunc;   // C 函数指针
    };
    
    Table* env;                // 函数环境（_ENV 或 getfenv）
};
```

## 6. Closure 结构

```cpp
class Closure {
    Proto* proto;               // 函数原型（编译产物）
    Vec<Upvalue*> upvalues;     // 捕获的上值列表
    Table* env;                 // 函数环境
    
    static Closure* createLua(Proto* proto, Vec<Upvalue*> upvalues);
    static Closure* createC(lua_CFunction func, Vec<Upvalue*> upvalues);
};
```

## 7. 子函数处理

```
主 Proto
  ├── subProtos[0] → 子函数 0 的 Proto
  ├── subProtos[1] → 子函数 1 的 Proto
  └── ...

每个子 Proto 在运行时：
  遇到 CLOSURE 指令 → 创建对应的 Closure → 绑定 Upvalue
```

## 8. Upvalue 捕获时机

```
编译时（CodeGen）:
  - 确定函数引用了哪些外部变量
  - 生成 UpvalueDesc（描述在哪个栈层级、哪个变量）

运行时（VM CLOSURE 指令）:
  - 根据 UpvalueDesc 查找或创建 Upvalue
  - 绑定到新创建的 Closure
```

## 9. Lua Closure vs C Closure

| 特性 | Lua Closure | C Closure |
|------|------------|-----------|
| Proto | ✅ 有（字节码） | ❌ 无 |
| 执行方式 | VM::executeProto() | 直接调用 C++ 函数 |
| Upvalue | ✅ 可以捕获 | ✅ 可以捕获（较少用） |
| 创建 | `function() ... end` | `lua_pushcclosure()` |
