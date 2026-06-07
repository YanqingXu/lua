# Proto Structure — Proto 结构

## 1. 这个模块解决什么问题？

Proto 是连接编译器（编译时）和 VM（运行时）的核心桥梁。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/function.hpp` | Proto 类定义 |
| `src/core/function.cpp` | Proto 实现 |

## 3. Proto 完整结构

```cpp
class Proto : public GCObject {
    // === 字节码 ===
    Vec<Instruction> code_;          // 指令序列
    
    // === 常量 ===
    Vec<Value> constants_;           // 常量表 (nil, bool, number, string)
    
    // === 局部变量 ===
    Vec<LocalVarInfo> localVars_;    // 局部变量信息
    // LocalVarInfo { Str name; i32 startPC; i32 endPC; }
    
    // === 子函数 ===
    Vec<Proto*> subProtos_;          // 内部定义的函数原型
    
    // === Upvalue ===
    Vec<UpvalueDesc> upvalueDescs_;  // Upvalue 描述
    // UpvalueDesc { i32 stackLevel; i32 index; bool inStack; }
    
    // === 元信息 ===
    usize maxStackSize_;             // 最大栈使用量
    i32 numParams_;                  // 参数数量
    bool isVararg_;                  // 是否接受可变参数
    
    // === 调试信息 ===
    Str source_;                     // 源文件名
    Vec<i32> lineInfo_;              // 每条指令的行号
};
```

## 4. 各部分详解

### code（字节码）
```
编译产物，不可变。VM 按顺序执行这些指令。
每行源码可能生成多条指令。
```

### constants（常量表）
```
编译时收集的所有字面量：
  - nil: 源代码中写 nil 的地方
  - bool: true / false
  - number: 所有数字字面量（去重）
  - string: 所有字符串字面量（去重）
```

### localVars（局部变量）
```
用于调试信息，记录每个局部变量的：
  - 名称
  - 起始 PC（变量从哪条指令开始可见）
  - 结束 PC（变量在哪条指令后不可见）
```

### subProtos（子函数）
```
递归结构：主 Proto 包含子 Proto，子 Proto 可以再包含子 Proto。
对应源码中嵌套定义的函数。
```

### upvalueDescs（Upvalue 描述）
```
编译时确定函数引用了哪些外部变量：
  - stackLevel: 在栈的哪一层（0=当前，1=上一层...）
  - index: 在该层的哪个寄存器
  - inStack: 是否在栈上（vs 已经被封闭）
```

## 5. Proto 生命周期

```
1. CodeGen 创建 Proto (new Proto)
2. 填充 code, constants, locals, subProtos, upvalueDescs
3. 传给 VM 执行
4. VM 执行完毕，Proto 可能被其他地方引用
5. GC 回收（当不再被引用时）
```

## 6. Proto vs Closure

```
Proto: 编译时产物，不可变，可共享
Closure: 运行时对象，可变（upvalue），每个闭包独立

多个闭包可以共享同一个 Proto:
  local x = 1
  return function() return x end,
         function() x = x + 1 end
  -- 两个闭包的 Proto 相同，但 upvalue 不同
```
