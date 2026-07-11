# Function Proto Generation — 函数原型生成

## 1. 这个模块解决什么问题？

编译函数定义时，如何生成子函数的 Proto 并绑定 Upvalue。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/function_compiler.cpp` | 函数级编译 |

## 3. 函数编译流程

```
compileFunction(functionExpr, isLocal, name):
  
  1. 创建新的 FunctionCompiler（独立的作用域和寄存器）
  
  2. 分配参数寄存器
     for each param: allocReg()
  
  3. 编译函数体
     for each stmt in body:
       compileStatement(stmt)
  
  4. 确定 upvalue 描述
     for each captured outer variable:
       UpvalueDesc{stackLevel, index, inStack}
  
  5. 生成 Proto
     Proto* subProto = new Proto(
       code, constants, locals, maxStackSize,
       numParams, isVararg, source
     )
  
  6. 添加到父 Proto 的 subProtos
  
  7. 在父函数中发射 CLOSURE 指令
     CLOSURE R(A) subProtoIdx
```

## 4. Upvalue 捕获流程

```lua
local x = 1
local function f()
    return x  -- x 是 upvalue
end
```

```
编译 f 时:
  1. 解析 NameExpr("x")
  2. 在 f 的作用域查找 x → 没找到
  3. 在父作用域查找 x → 找到! (local x)
  4. 确定:
     - stackLevel: 1 (在上一层函数的作用域)
     - index: 0 (在 parent 的 R(0))
     - inStack: true (还在栈上)
  5. 生成 UpvalueDesc{1, 0, true}
  6. 在 Proto::upvalueDescs 中添加

父函数编译时:
  CLOSURE 指令会读取 upvalueDescs:
    for each UpvalueDesc:
      Upvalue* uv = findOrCreateUpvalue(desc)
      closure->addUpvalue(uv)
```

## 5. 嵌套函数示例

```lua
function outer()
    local a = 1
    function inner()
        local b = 2
        function innermost()
            return a + b  -- a=upvalue(2层), b=upvalue(1层)
        end
        return innermost
    end
    return inner
end
```

```
Proto 树:
  outer Proto
    ├── upvalueDescs: []
    └── subProtos[0] = inner Proto
          ├── upvalueDescs: [{level:1, idx:0}]  ← 捕获 a
          └── subProtos[0] = innermost Proto
                ├── upvalueDescs: [{level:2, idx:0},  ← 捕获 a
                │                  {level:1, idx:1}]  ← 捕获 b
```
