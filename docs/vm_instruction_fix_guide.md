# VM指令修复指南

## 概述

本指南提供了修复当前VM指令实现中关键不合规问题的具体方案，按优先级排序，包含详细的代码示例和实现步骤。

## P0 - 立即修复项

### 1. 修复比较指令的条件跳转语义

#### 问题描述
当前的EQ、LT、LE指令错误地将比较结果存储到寄存器，而官方Lua 5.1.5中这些是条件跳转指令。

#### 官方Lua 5.1.5实现参考
```c
case OP_EQ: {
  TValue *rb = RKB(i);
  TValue *rc = RKC(i);
  Protect(
    if (equalobj(L, rb, rc) == GETARG_A(i))
      dojump(L, pc, GETARG_sBx(*pc));
  )
  pc++;
  continue;
}
```

#### 修复方案
```cpp
// 在 vm_executor.cpp 中修复 handleEq 方法
void VMExecutor::handleEq(LuaState* L, Instruction instr, Value* base, 
                         const Vec<Value>& constants, u32& pc) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    u16 c = instr.getC();
    
    Value* vb = getRK(base, constants, b);
    Value* vc = getRK(base, constants, c);
    
    bool equal = false;
    if (vb && vc) {
        // 实现相等比较逻辑
        if (vb->type() == vc->type()) {
            if (vb->isNumber()) equal = (vb->asNumber() == vc->asNumber());
            else if (vb->isString()) equal = (vb->asString() == vc->asString());
            else if (vb->isBoolean()) equal = (vb->asBoolean() == vc->asBoolean());
            else if (vb->isNil()) equal = true;
            // TODO: 添加其他类型的比较
        }
    }
    
    // 关键修复：实现条件跳转语义
    // 如果 (比较结果 == A参数)，则跳转到下一条指令的sBx偏移
    if (equal == (a != 0)) {
        // 获取下一条指令的sBx参数作为跳转偏移
        if (pc + 1 < codeSize) {
            Instruction nextInstr = code[pc + 1];
            i16 sbx = nextInstr.getSBx();
            pc += sbx;
        }
    }
    pc++; // 跳过下一条指令（JMP指令）
}

// 类似地修复 handleLt 和 handleLe
void VMExecutor::handleLt(LuaState* L, Instruction instr, Value* base, 
                         const Vec<Value>& constants, u32& pc) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    u16 c = instr.getC();
    
    Value* vb = getRK(base, constants, b);
    Value* vc = getRK(base, constants, c);
    
    bool less = false;
    if (vb && vc && vb->isNumber() && vc->isNumber()) {
        less = (vb->asNumber() < vc->asNumber());
        // TODO: 添加字符串比较和元方法支持
    }
    
    if (less == (a != 0)) {
        if (pc + 1 < codeSize) {
            Instruction nextInstr = code[pc + 1];
            i16 sbx = nextInstr.getSBx();
            pc += sbx;
        }
    }
    pc++;
}
```

#### 修改执行循环
```cpp
// 在 executeLoop 中修改比较指令的处理
case OpCode::EQ:
    handleEq(L, instr, base, constants, pc);
    continue; // 使用continue而不是break，因为pc已经被修改

case OpCode::LT:
    handleLt(L, instr, base, constants, pc);
    continue;

case OpCode::LE:
    handleLe(L, instr, base, constants, pc);
    continue;
```

### 2. 移除调试输出代码

#### 问题描述
生产代码中包含大量调试输出，影响性能和代码清洁度。

#### 修复方案
```cpp
// 移除 handleTest 中的调试输出
void VMExecutor::handleTest(LuaState* L, Instruction instr, Value* base, u32& pc) {
    u8 a = instr.getA();
    u16 c = instr.getC();
    
    Value& testValue = base[a];
    bool isFalse = testValue.isNil() || (testValue.isBoolean() && !testValue.asBoolean());
    
    // 移除所有 std::cout 调试输出
    
    if (isFalse == (c == 0)) {
        pc++; // 跳过下一条指令
    }
}

// 移除 handleLe 中的调试输出
void VMExecutor::handleLe(LuaState* L, Instruction instr, Value* base, 
                         const Vec<Value>& constants, u32& pc) {
    // ... 实现逻辑，移除所有 std::cout 语句
}
```

### 3. 完善CONCAT指令实现

#### 问题描述
当前CONCAT只支持两个值的连接，官方Lua 5.1.5支持多个值的连接。

#### 官方Lua 5.1.5实现参考
```c
case OP_CONCAT: {
  int b = GETARG_B(i);
  int c = GETARG_C(i);
  Protect(luaV_concat(L, c-b+1, c); luaC_checkGC(L));
  setobjs2s(L, RA(i), base+b);
  continue;
}
```

#### 修复方案
```cpp
void VMExecutor::handleConcat(LuaState* L, Instruction instr, Value* base, 
                             const Vec<Value>& constants) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    u16 c = instr.getC();
    
    // 连接从 R(B) 到 R(C) 的所有值
    std::string result;
    for (u16 i = b; i <= c; i++) {
        result += base[i].toString();
    }
    
    // 检查GC（字符串创建可能触发GC）
    luaC_checkGC(L);
    
    // 创建GC字符串
    GCString* gcStr = luaS_newlstr(L, result.c_str(), result.length());
    base[a] = Value(gcStr);
}
```

## P1 - 高优先级修复项

### 4. 实现基础元方法支持

#### 算术运算元方法框架
```cpp
// 添加元方法支持的算术运算
void VMExecutor::handleAddWithMetamethod(LuaState* L, Instruction instr, 
                                        Value* base, const Vec<Value>& constants) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    u16 c = instr.getC();
    
    Value* vb = getRK(base, constants, b);
    Value* vc = getRK(base, constants, c);
    
    // 首先尝试直接数值运算
    if (vb && vc && vb->isNumber() && vc->isNumber()) {
        double result = vb->asNumber() + vc->asNumber();
        base[a] = Value(result);
        return;
    }
    
    // 如果不是数值，尝试元方法
    // TODO: 实现元方法查找和调用
    // 1. 检查操作数的元表
    // 2. 查找 __add 元方法
    // 3. 调用元方法
    // 4. 处理返回值
    
    // 临时实现：抛出错误
    typeError(L, vb ? *vb : Value(), "perform arithmetic on");
}
```

### 5. 完善SELF指令实现

#### 问题描述
当前SELF指令只实现了第一部分，缺少表索引操作。

#### 官方Lua 5.1.5实现参考
```c
case OP_SELF: {
  StkId rb = RB(i);
  setobjs2s(L, ra+1, rb);
  Protect(luaV_gettable(L, rb, RKC(i), ra));
  continue;
}
```

#### 修复方案
```cpp
void VMExecutor::handleSelf(LuaState* L, Instruction instr, Value* base, 
                           const Vec<Value>& constants) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    u16 c = instr.getC();
    
    // R(A+1) := R(B)
    base[a + 1] = base[b];
    
    // R(A) := R(B)[RK(C)]
    Value table = base[b];
    if (!table.isTable()) {
        vmError(L, "attempt to index a non-table value in SELF");
        return;
    }
    
    Value* key = getRK(base, constants, c);
    if (!key) {
        vmError(L, "invalid key in SELF");
        return;
    }
    
    // 获取表中的方法
    Value method = table.asTable()->get(*key);
    base[a] = method;
}
```

## P2 - 中优先级修复项

### 6. 实现上值系统基础框架

#### 上值数据结构
```cpp
// 在适当的头文件中定义
class UpValue {
private:
    Value* value;  // 指向实际值的指针
    Value closed;  // 关闭时的值副本
    bool isClosed; // 是否已关闭
    
public:
    UpValue(Value* val) : value(val), isClosed(false) {}
    
    Value* getValue() {
        return isClosed ? &closed : value;
    }
    
    void close() {
        if (!isClosed) {
            closed = *value;
            isClosed = true;
        }
    }
};
```

#### GETUPVAL/SETUPVAL实现
```cpp
void VMExecutor::handleGetUpval(LuaState* L, Instruction instr, Value* base) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    
    // 获取当前函数的上值
    CallInfo* ci = L->getCurrentCI();
    if (!ci || !ci->func->isFunction()) {
        vmError(L, "invalid function in GETUPVAL");
        return;
    }
    
    auto func = ci->func->asFunction();
    if (b >= func->getUpValueCount()) {
        vmError(L, "upvalue index out of range");
        return;
    }
    
    UpValue* upval = func->getUpValue(b);
    base[a] = *upval->getValue();
}

void VMExecutor::handleSetUpval(LuaState* L, Instruction instr, Value* base) {
    u8 a = instr.getA();
    u16 b = instr.getB();
    
    // 类似的实现，但是设置上值
    // TODO: 完整实现
}
```

## 测试验证

### 比较指令测试
```lua
-- 测试修复后的比较指令
local function test_comparisons()
    if 1 == 1 then
        print("EQ: 1 == 1 works")
    else
        print("EQ: 1 == 1 failed")
    end
    
    if 1 < 2 then
        print("LT: 1 < 2 works")
    else
        print("LT: 1 < 2 failed")
    end
    
    if 1 <= 1 then
        print("LE: 1 <= 1 works")
    else
        print("LE: 1 <= 1 failed")
    end
end

test_comparisons()
```

### 字符串连接测试
```lua
-- 测试修复后的CONCAT指令
local function test_concat()
    local s1 = "a"
    local s2 = "b"
    local s3 = "c"
    local result = s1 .. s2 .. s3
    print("Concat result: " .. result) -- 应该输出 "abc"
end

test_concat()
```

## 实施步骤

1. **第一阶段**: 修复比较指令语义（1-2天）
2. **第二阶段**: 移除调试代码，完善CONCAT（1天）
3. **第三阶段**: 实现SELF指令完整功能（1-2天）
4. **第四阶段**: 建立元方法支持框架（3-5天）
5. **第五阶段**: 实现上值系统基础（5-7天）

每个阶段完成后都应该运行完整的测试套件验证功能正确性。
