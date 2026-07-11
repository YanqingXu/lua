# Reference Graph — 引用关系图

## 1. GC 根集

```
根集 (Root Set) — GC 从这些对象开始标记:

GlobalState
  ├── registry (Table)         — C 代码存储
  ├── mainThread (Thread)      — 主线程
  ├── metatables[9] (Table*)   — 基础类型元表
  └── stringPool (StringPool)  — 字符串池

LuaState (每个线程)
  ├── stack (Stack)            — 所有活跃值
  ├── callStack (CallInfo[])   — 调用栈
  └── openUpvalues (Upvalue*)  — Open upvalue 链表
```

## 2. 引用链示例

```
registry (Table)
  └── "_G" → Table (全局表)
        ├── "print" → Function (print)
        │               └── Closure (C)
        │                    └── upvalues: [...]
        ├── "math" → Table (math 库)
        │             ├── "abs" → Function
        │             └── "pi" → Number (3.14159)
        └── "myTable" → Table
                         ├── array[0] → String "hello"
                         └── hash["func"] → Function
                                              └── Proto
                                                   └── subProtos[0] → Proto
                                                                         └── constants[0] → String "x"
```

## 3. 循环引用

```
a = {}
b = {}
a.ref = b
b.ref = a
-- 循环引用! 

GC 处理:
  标记-清除算法天然处理循环引用
  → 从根集出发标记
  → a 和 b 都能从根集到达 → 标记为 Black → 不会被回收
  
如果 a 和 b 都不可达:
  → 都不能从根集到达 → 保持 White → 都被回收
  ✓ 无内存泄漏
```
