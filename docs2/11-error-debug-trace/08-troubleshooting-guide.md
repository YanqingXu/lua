# Troubleshooting Guide — 问题排查指南

## 语法报错但 Lua 官方能运行

```
1. Lexer token 是否正确?
   → lua_bytecode 检查 token 输出
   → 检查是否有关键字/运算符遗漏

2. Parser 是否支持该语法?
   → 检查 parseExpression/parseStatement 的 switch 分支
   → 检查运算符优先级表

3. 作用域是否提前结束?
   → 检查 block exit 的 upvalue close
```

## 执行结果和 Lua 官方不同

```
1. Bytecode 是否生成错误?
   → lua_bytecode --diff my.lua (对比字节码)
   → 检查 CodeGen 的发射逻辑

2. VM 指令是否执行错误?
   → 开启 --trace 逐指令对比
   → 检查对应 handler 的 RK 寻址/寄存器访问

3. 多返回值是否被截断?
   → 检查 CALL 指令的 C 参数 (nresults)
   → 检查 RETURN 指令的 B 参数

4. Metatable 是否生效?
   → 检查 __index/__newindex 是否正确注册
   → 检查 getMetamethodByObject 的查找路径
```

## 函数返回值不对

```
1. 检查 OP_CALL 实现
   → src/vm/vm_handlers/vm_handlers_call.cpp

2. 检查 OP_RETURN 实现
   → 返回值放置: stack[ci.func + i]

3. 检查 CallInfo
   → nresults 字段的值
   → LUA_MULTRET 的处理

4. 检查多返回值规则
   → 表达式上下文截断
   → 表构造器末尾展开
```

## 闭包变量丢失

```
1. 检查 upvalue 捕获
   → UpvalueDesc 的 stackLevel/index 是否正确

2. 检查 open upvalue 链表
   → findOrCreateUpvalue 是否正常

3. 检查 close upvalue 时机
   → break/return 时是否发出 CLOSE 指令

4. 检查 close() 是否正确复制值
   → closedValue_ = *v_ → v_ = &closedValue_
```

## 内存问题 (use-after-free / double-free)

```
1. 检查 GC 标记
   → 对象是否被正确标记 (markChildren)
   → 是否有循环引用导致无法回收

2. 检查指针有效性
   → base 指针在栈扩展后是否刷新 (refreshBase)
   → StringPool 返回的指针在 GC 后是否有效

3. 检查 RAII
   → unique_ptr/shared_ptr 的生命周期
```
