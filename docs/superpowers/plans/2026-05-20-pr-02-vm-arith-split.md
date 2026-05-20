# PR-02 VM 算术分支拆分实施计划

> **给 agentic workers：** 必须使用子技能：推荐使用 `superpowers:subagent-driven-development`，也可使用 `superpowers:executing-plans`，并按任务逐项执行本计划。步骤使用复选框（`- [ ]`）语法跟踪进度。

**目标：** 将 `src/vm/vm.cpp` 主 dispatch switch 中 `ADD/SUB/MUL/DIV/MOD/POW` 的 case 体抽取到 `src/vm/vm_arith.cpp`，保持 VM 行为和现有 `VM::detail::arith(...)` 签名不变。

**架构：** 新增执行层 helper `VM::detail::execArithmetic(...)`，由它负责读取 RK 操作数、调用既有 `arith(...)`、刷新 `base` 并写回目标寄存器。`vm.cpp` 继续保留 opcode dispatch，只将算术分支委托给 helper。

**技术栈：** C++20/C++23、MSVC 项目文件、CMake 静态库目标 `lua_core`、自定义 `lua_test.exe` 测试运行器。

---

## 文件结构

- 新建：`src/vm/vm_arith.cpp`
  - 实现 `VM::detail::execArithmetic(LuaState*, Proto*, Value*&, i32, i32, i32, OpCode)`。
  - 保留文件内局部 `getRK(...)` 与 `refreshBase(...)` helper，避免扩大公共接口。
- 修改：`src/vm/vm_internal.hpp`
  - 声明 `execArithmetic(...)`。
- 修改：`src/vm/vm.cpp`
  - 将 `ADD/SUB/MUL/DIV/MOD/POW` case 体替换为 `VM::detail::execArithmetic(...)`。
- 修改：`tests/unit/vm/test_vm_internal_boundaries.cpp`
  - 增加 `execArithmetic(...)` 的编译期签名检查。
- 修改：`CMakeLists.txt`
  - 将 `src/vm/vm_arith.cpp` 加入 `LUA_CORE_SOURCES`。
- 修改：`lua.vcxproj`
  - 将 `src\vm\vm_arith.cpp` 加入核心静态库项目。
- 修改：`lua.vcxproj.filters`
  - 将 `src\vm\vm_arith.cpp` 挂到 `src\vm` filter。

## 任务

### 任务 1：新增算术执行 helper

**文件：**
- 新建：`src/vm/vm_arith.cpp`
- 修改：`src/vm/vm_internal.hpp`

- [ ] **步骤 1：声明 helper**

在 `src/vm/vm_internal.hpp` 中 `arith(...)` 附近加入：

```cpp
void execArithmetic(LuaState* L, Proto* proto, Value*& base, i32 a, i32 b, i32 c, OpCode op);
```

- [ ] **步骤 2：实现 helper**

在 `src/vm/vm_arith.cpp` 中实现：

```cpp
void execArithmetic(LuaState* L, Proto* proto, Value*& base, i32 a, i32 b, i32 c, OpCode op) {
    Value left = getRK(proto, base, b);
    Value right = getRK(proto, base, c);
    Value result;
    arith(L, result, left, right, op);
    base = refreshBase(L);
    base[a] = result;
}
```

预期：行为与原 `vm.cpp` 算术 case 体一致。

### 任务 2：简化主 switch 算术 case

**文件：**
- 修改：`src/vm/vm.cpp`

- [ ] **步骤 1：替换 case 体**

将 `ADD/SUB/MUL/DIV/MOD/POW` case 体替换为：

```cpp
VM::detail::execArithmetic(L, proto, base, a, b, c, op);
break;
```

预期：主循环仍负责 dispatch，算术执行细节进入 `vm_arith.cpp`。

### 任务 3：更新构建与边界测试

**文件：**
- 修改：`CMakeLists.txt`
- 修改：`lua.vcxproj`
- 修改：`lua.vcxproj.filters`
- 修改：`tests/unit/vm/test_vm_internal_boundaries.cpp`

- [ ] **步骤 1：同步构建清单**

将 `src/vm/vm_arith.cpp` 加入 CMake 和 VS 静态库项目。

- [ ] **步骤 2：增加签名检查**

在 `testOperationHelpersExposeStableSignatures` 中增加：

```cpp
static_assert(std::is_same_v<decltype(&VM::detail::execArithmetic),
                             void (*)(LuaState*, Proto*, Value*&, i32, i32, i32, OpCode)>);
```

### 任务 4：验证

**文件：**
- 运行：`tools/run_quality_gate.ps1`
- 运行：`bin/lua_test.exe`

- [ ] **步骤 1：构建并跑质量门**

```powershell
.\tools\run_quality_gate.ps1
```

预期：文档漂移、MSBuild、单元测试通过；本地没有 `clang-format` / `clang-tidy` 时允许明确跳过。

- [ ] **步骤 2：显式运行完整测试**

```powershell
.\bin\lua_test.exe
```

预期：退出码为 `0`，最终汇总包含 `Failed: 0`。
