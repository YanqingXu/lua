# Source Location — 源码位置追踪

## 1. 行号信息存储

```cpp
// Proto 中
Vec<i32> lineInfo_;  // lineInfo[pc] = 源码行号

// 每条指令对应一个源文件行号
// 多行源码可能生成多条指令, 一条源码可能生成多条指令
```

## 2. 位置查询

```cpp
// 通过 PC 查询行号
i32 getLine(i32 pc) {
    if (pc < lineInfo_.size())
        return lineInfo_[pc];
    return -1;
}

// 通过 savedpc 查询当前执行位置
i32 getCurrentLine(CallInfo& ci) {
    i32 pc = ci.savedpc - code.data() - 1;  // 前一条指令
    return proto->getLine(pc);
}
```

## 3. CallInfo 中的位置

```
CallInfo {
    savedpc → 指向当前/下一条指令
    func → 函数在栈中的索引
}

通过 savedpc 可以反推:
  - 行号: proto->lineInfo[pc - 1]
  - 函数名: 通过 debug info 查找
  - 源文件: proto->source
```

## 4. 错误消息中的位置

```lua
-- 错误消息格式
[string "source"]:3: attempt to perform arithmetic on a nil value
stack traceback:
    [string "source"]:5: in function 'foo'
    [string "source"]:10: in main chunk
```
