# Proto — 函数原型

## 1. Proto 的核心结构

```cpp
class Proto : public GCObject {
    Vec<Instruction> code_;          // 字节码指令序列
    Vec<Value> constants_;           // 常量表
    Vec<LocalVarInfo> localVars_;    // 局部变量 (名称 + 起止PC)
    Vec<Proto*> subProtos_;          // 子函数原型
    Vec<UpvalueDesc> upvalueDescs_;  // Upvalue 描述
    
    usize maxStackSize_;             // 最大寄存器使用量
    i32 numParams_;                  // 参数数量
    bool isVararg_;                  // 是否可变参数
    
    // 调试信息
    Str source_;                     // 源文件名
    Vec<i32> lineInfo_;              // 每条指令对应的行号
};
```

## 2. LocalVarInfo

```cpp
struct LocalVarInfo {
    Str name;      // 变量名
    i32 startPC;   // 从哪条指令开始可见
    i32 endPC;     // 在哪条指令后失效
};
```

## 3. UpvalueDesc

```cpp
struct UpvalueDesc {
    i32 stackLevel;  // 在栈的哪一层 (0=当前, 1=上一层...)
    i32 index;       // 在该层的寄存器索引
    bool inStack;    // 是否还在栈上 (false=已被 close)
};
```

## 4. Proto 的生命周期

```
创建: CodeGen::generate() → new Proto
注册: 父 Proto::subProtos 持有指针
GC:   不再被引用时由 GC 回收
      (注意: Proto 如果被 string.dump 的闭包引用也需要标记)
```
