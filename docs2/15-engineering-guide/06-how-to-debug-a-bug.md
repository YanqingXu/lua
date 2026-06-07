# How to Debug a Bug — 如何排查 Bug

## 1. 定位 Bug 的阶段

```
发现问题 → 判断属于哪个阶段:

1. Lexer 阶段?
   → 用 lua_bytecode 查看 token 输出是否正确
   → 检查关键字、字面量、注释处理

2. Parser 阶段?
   → REPL 中 .ast 查看 AST 是否正确
   → 检查语法规则、优先级、作用域

3. CodeGen 阶段?
   → lua_bytecode 查看字节码是否正确
   → 检查指令发射、寄存器分配、跳转回填

4. VM 阶段?
   → lua_app --trace 逐指令对比
   → 检查 handler 实现、RK 寻址、类型转换

5. Runtime 阶段?
   → 检查 Value/Table/Function/Upvalue 实现
   → 检查 GC 是否误回收
```

## 2. 二分法定位

```
1. 先用官方 Lua 5.1 执行，确认这是本项目的 Bug（不是 Lua 语言特性）
2. 用 lua_bytecode 对比字节码:
   lua_bytecode --diff official_output.txt our_output.txt
3. 如果字节码相同 → Bug 在 VM
   如果字节码不同 → Bug 在 Lexer/Parser/CodeGen
4. 缩小范围，直到找到具体指令或函数
```

## 3. 最小复现

```
从失败用例中提取最小复现:
  1. 去掉无关代码
  2. 简化表达式
  3. 直到剩余 5-10 行的最小脚本
  4. 把这个最小脚本保存为回归测试
```

## 4. 常用调试手段

```
1. 在 VM 主循环中加条件断点:
   if (pc == 42) __debugbreak();

2. 打印寄存器状态:
   for (int i = 0; i < maxStack; i++)
       printf("R(%d) = %s\n", i, R(i).toString().c_str());

3. 对比官方 Lua 的字节码:
   luac -l script.lua  (官方)
   lua_bytecode script.lua  (本项目)

4. 开启 trace:
   lua_app --trace script.lua
```
