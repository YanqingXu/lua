# Debugging VM — VM 调试

## 1. 这个模块解决什么问题？

如何调试 VM 执行过程中的问题。

## 2. 调试工具链

| 工具 | 用途 |
|------|------|
| `lua_bytecode` | 查看编译后的字节码 |
| `lua_app --trace` | 指令级执行追踪 |
| `REPL .bytecode` | 交互式查看字节码 |
| `REPL .ast` | 交互式查看 AST |
| `REPL .gc` | 交互式查看 GC 状态 |
| `bin/out.jsonl` + `trace_viewer.html` | 可视化 trace 查看器 |

## 3. 常见 VM 问题排查

### 问题: 执行结果不对

```
1. 先用 lua_bytecode 看字节码是否正确
2. 如果不是字节码问题 → VM 指令实现有问题
3. 开启 --trace 逐指令对比预期
4. 关注: 寄存器读写、常量访问、类型转换
```

### 问题: 栈溢出

```
1. 检查是否无限递归
2. 检查 TAILCALL 是否正确复用栈帧
3. 检查 MAX_CALLS 是否需要调整
4. 检查 C→Lua 重入是否过深
```

### 问题: 闭包变量值不对

```
1. 检查 Upvalue 是否绑定到正确的位置
2. 检查 Upvalue 的 Open/Closed 状态
3. 检查 CLOSE 指令是否正确关闭
```

### 问题: 函数调用后栈状态错乱

```
1. 检查 CALL 的 B/C 参数是否正确
2. 检查返回值放置逻辑
3. 检查多返回值截断逻辑
4. 检查 nresults = LUA_MULTRET 的处理
```

## 4. 断点调试

```cpp
// 在 C++ 代码中设置条件断点:

// 断在特定 PC:
if (pc == 42) __debugbreak();

// 断在特定操作码:
if (op == OpCode::CALL && pc > 10) __debugbreak();

// 断在特定寄存器值:
if (R(0).isNumber() && R(0).asNumber() == 42.0) __debugbreak();
```

## 5. 字节码 dump

```cpp
// 开启字节码 dump (编译时)
// 在 VM 执行前输出完整的字节码

// vm.cpp 中如果 VM::detail::shouldDumpBytecode() 为 true:
[BCDUMP] proto(0x...) 12 instructions, pc=0
  [0]  op=0  A=0  B=1  C=0  Bx=1   sBx=-131070
  [1]  op=30 A=0  B=2  C=3  Bx=770 sBx=-130301
  ...
```
