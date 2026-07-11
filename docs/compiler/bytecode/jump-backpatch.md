# Jump Backpatch — 跳转回填

## 1. 这个模块解决什么问题？

编译控制流时，跳转目标在编译跳转指令时还不知道，需要后续回填。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/jump_patcher.cpp` | 跳转回填实现 |

## 3. 为什么需要回填？

```
编译 if cond then body else alt end 时：

1. 遇到 if → 需要编译条件跳转
   但此时还不知道 body 有多长，无法确定跳转目标
   
2. 解决: 先发射跳转指令，记录位置
   编译完 body 后，回填跳转目标

3. 同样，else 之后的 JMP 也需要回填
```

## 4. 回填机制

```cpp
// 发射跳转指令，返回"欠条"（可回填的跳转位置）
struct JumpPatch {
    i32 instructionPC;  // 跳转指令所在的 PC
    // 跳转目标待定
};

// 发射跳转
JumpPatch emitJump(OpCode op) {
    i32 pc = emitInst(op, 0);  // 暂时填 0
    return JumpPatch{pc};
}

// 回填：目标位置确定了
void patchJump(JumpPatch patch, i32 targetPC) {
    i32 offset = targetPC - (patch.instructionPC + 1);
    SETARG_sBx(code[patch.instructionPC], offset);
}
```

## 5. If-Else 回填示例

```
编译: if cond then trueBody else falseBody end

1. EQ/TEST cond 0
   → 需要跳到 falseBody，但还不知道 falseBody 在哪
   → emitJump(TEST) → patch1

2. 编译 trueBody:
   instruction 3: ...
   instruction 4: ...

3. JMP → 需要跳到 end，但还不知道 end 在哪
   → emitJump(JMP) → patch2

4. 编译 falseBody:  ← patch1 的目标位置!
   patchJump(patch1, 当前PC)

5. 编译 falseBody:
   instruction 5: ...

6. (end)  ← patch2 的目标位置!
   patchJump(patch2, 当前PC)
```

## 6. 回填链（Linked List）

```
对于嵌套控制流，可能需要回填多个跳转：

while cond do
    if innerCond then
        break    ← 需要跳出 while
    end
end

break 的跳转需要回填到 while 的 end 之后。
这通过"退出点链表"管理。
```

## 7. 常见 Bug

| 问题 | 原因 |
|------|------|
| 跳转到错误位置 | offset 计算错误（忘记 +1 或不加 1） |
| 嵌套 break 回填到错误层级 | 退出点链管理错误 |
| 跳转越界 | sBx 范围不够 (超过 ±131071) |
