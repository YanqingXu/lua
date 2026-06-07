# How to Add an Opcode — 如何新增指令

## 步骤

### 1. 在 `opcode.hpp` 中加入新指令
```cpp
// src/compiler/opcode.hpp
enum class OpCode : u8 {
    // ... existing ...
    MY_NEW_OP,  // 新增指令 (放在 VARARG 之前)
};
// 更新 NUM_OPCODES
```

### 2. 在 `kOpcodeMetadata` 中添加元数据
```cpp
// opcode.hpp 中的 kOpcodeMetadata 数组
detail::makeOpcodeMetadata(OpCode::MY_NEW_OP, "MYNEWOP", OpMode::iABC,
    OpArgMask::OpArgR, OpArgMask::OpArgK, true, false,
    VM::OpcodeGroup::Arithmetic, false),
```

### 3. 在 disassembler 中支持打印
```cpp
// src/bytecode/bytecode_printer.cpp
case OpCode::MY_NEW_OP:
    printf("R(%d) = R(%d) myop RK(%d)\n", A, B, C);
    break;
```

### 4. 在 compiler 中生成该指令 (codegen)
```cpp
// 在适当的 emitter 中
i32 myNewOpEmitter(/* params */) {
    return emitABC(OpCode::MY_NEW_OP, destReg, srcReg, operand);
}
```

### 5. 在 VM dispatch 中实现
```cpp
// src/vm/vm.cpp switch 中
case OpCode::MY_NEW_OP:
    status = VM::detail::execOpMyNewOp(opContext, inst);
    break;

// 或添加 handler
// src/vm/vm_handlers/vm_handlers_arith.cpp
HandlerStatus execOpMyNewOp(OpExecutionContext& ctx, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);
    
    Value& result = ctx.R(a);
    Value& src = ctx.R(b);
    Value& operand = ctx.RK(c);
    
    result = /* 你的操作 */;
    
    return HandlerStatus::Continue;
}
```

### 6. 写测试
```lua
-- tests/lua/regressions/test_my_new_op.lua
local result = my_new_operation(1, 2)
assert(result == expected)
```

## Checklist

- [ ] 指令格式明确 (ABC/ABx/AsBx)
- [ ] 读取寄存器/常量明确 (R/RK/K)
- [ ] 写入寄存器明确
- [ ] PC 行为明确 (是否修改 PC)
- [ ] 错误处理明确
- [ ] Metadata 正确
- [ ] Disassembler 支持
- [ ] 测试覆盖
