# 下一步开发指导（2026-01-18）

> **更新日期**：2026-01-18  
> **当前完成度**：80%  
> **剩余工作量**：15人日（20%）  
> **下一个里程碑**：M6（标准库完成）- 预计2周后

---

## 📊 当前状态总结

### 最新完成（2026-01-18）

✅ **P0任务1：LuaState初始化系统** - 已完成
- 字符串表初始化
- 元方法名称初始化（17个）
- 保留字初始化（21个）
- 内存错误消息固定
- GC FIXED标志机制实现
- 修复2个关键GC bug
- 新增20个单元测试

**详细报告**：`docs/P0_TASK1_COMPLETION_REPORT.md`

### 整体进度

- **整体完成度**：80% ⬆️ +2%
- **核心功能**：97% ⬆️ +2%
- **GC系统**：95% ⬆️ +5%
- **测试数量**：399个 ⬆️ +20
- **剩余工作**：15人日 ⬇️ -2人日

---

## 🎯 下一步任务：P0任务2 - 脚本执行功能（增强版）

### 任务概述

**优先级**：P0（关键任务）⭐⭐⭐
**预计工作量**：0.5人日（大部分已完成）
**目标**：完善Lua脚本文件执行功能，添加命令行参数支持

### 当前状态

**已完成**：✅
- ✅ `main.cpp`框架完整
- ✅ `executeScript()`函数完整实现
- ✅ 文件读取逻辑（`readFileContents()`）
- ✅ 编译执行流程完整集成（Lexer→Parser→CodeGen→VM）
- ✅ 错误处理完整（语法错误、运行时错误、文件错误）
- ✅ 命令行参数解析（-v, -h, -t, -i, 脚本文件）

**待实现**：❌
- ❌ 命令行参数传递给脚本（`arg`表）
- ❌ 端到端测试脚本验证

### 实现步骤

#### ✅ 步骤1-3：核心功能已完成

**已实现内容**（见`src/main.cpp`）：
- ✅ 文件读取：`readFileContents(filename)`
- ✅ 编译流程：Parser → CodeGenerator → Proto
- ✅ VM执行：Function → VM::execute()
- ✅ 错误处理：ParseError、runtime_error、exception

**当前实现**（`src/main.cpp:291-341`）：
```cpp
int executeScript(LuaState* L, const char* filename) {
    try {
        // 1. 读取文件
        Str source = readFileContents(filename);

        // 2. 解析AST
        Parser parser(source);
        Chunk chunk = parser.parse();

        // 3. 生成字节码
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        // 4. 创建函数并注册GC
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());

        // 5. 执行
        VM vm(L);
        vm.execute(func);

        delete proto;
        return 0;

    } catch (const ParseError& e) {
        REPL::reportError(filename, e.getLine(), e.what());
        return 1;
    } catch (const std::runtime_error& e) {
        REPL::reportError(e.what());
        return 1;
    }
}
```

---

#### 步骤4：添加命令行参数支持（0.3人日）⭐ 待实现

**目标**：实现Lua标准的`arg`表，使脚本能够访问命令行参数

**修改文件**：`src/main.cpp`

**实现要点**：

1. **添加`setupArgTable()`函数**：

```cpp
/**
 * @brief 设置arg表（参考lua_c_analysis/src/lua.c:createargtable）
 *
 * arg表结构：
 * - arg[-n] ... arg[-1]: 脚本名之前的参数（如 -e 代码）
 * - arg[0]: 脚本名
 * - arg[1] ... arg[n]: 脚本参数
 *
 * 示例：lua -e "print(1)" script.lua a b
 * - arg[-1] = "print(1)"
 * - arg[0] = "script.lua"
 * - arg[1] = "a"
 * - arg[2] = "b"
 */
void setupArgTable(LuaState* L, int argc, char* argv[], int scriptIndex) {
    // 创建arg表
    Table* argTable = new Table();
    L->getGlobalState().getGC().registerObject(argTable);

    // arg[0] = 脚本名
    GCString* scriptName = L->getGlobalState().getStringPool().intern(argv[scriptIndex]);
    argTable->set(Value(0.0), Value(scriptName));

    // arg[1], arg[2], ... = 脚本参数
    int argIndex = 1;
    for (int i = scriptIndex + 1; i < argc; i++) {
        GCString* argStr = L->getGlobalState().getStringPool().intern(argv[i]);
        argTable->set(Value(static_cast<double>(argIndex)), Value(argStr));
        argIndex++;
    }

    // 设置全局变量arg
    GCString* argName = L->getGlobalState().getStringPool().intern("arg");
    L->getGlobalTable()->set(Value(argName), Value(argTable));
}
```

2. **在`main()`函数中调用**：

在`src/main.cpp`的第425-427行之间添加：

```cpp
if (scriptFile) {
    // 设置arg表（新增）
    setupArgTable(L.get(), argc, argv, i);  // i是scriptFile的索引

    // 脚本执行模式
    status = executeScript(L.get(), scriptFile);
}
```

3. **修改命令行解析逻辑**：

需要记录脚本文件的索引位置：

```cpp
int scriptIndex = -1;  // 新增：记录脚本文件索引

for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-v") == 0) {
        showVersion = true;
    } else if (std::strcmp(argv[i], "-h") == 0) {
        showHelp = true;
    } else if (std::strcmp(argv[i], "-t") == 0) {
        enableTestMode = true;
    } else if (std::strcmp(argv[i], "-i") == 0) {
        enableTestMode = false;
    } else if (argv[i][0] != '-') {
        enableTestMode = false;
        scriptFile = argv[i];
        scriptIndex = i;  // 记录索引
        break;
    }
}
```

**参考实现**：
- `lua_c_analysis/src/lua.c:createargtable()`
- `lua_c_analysis/src/lua.c:collectargs()`

---

#### 步骤5：创建测试脚本验证（0.2人日）

创建测试脚本目录：`lua/test_scripts/`

### 验收标准

- [ ] 可以从命令行执行Lua脚本文件：`lua.exe script.lua`
- [ ] 可以传递命令行参数：`lua.exe script.lua arg1 arg2`
- [ ] 编译错误有清晰的错误消息（文件名、行号、错误描述）
- [ ] 运行时错误有清晰的错误消息和栈回溯
- [ ] 通过至少5个端到端测试脚本
- [ ] 代码注释完整，符合项目规范

### 测试用例

创建以下测试脚本验证功能：

**test1.lua** - 基础输出：
```lua
print("Hello, World!")
```

**test2.lua** - 变量和运算：
```lua
local a = 10
local b = 20
print(a + b)
```

**test3.lua** - 函数定义：
```lua
function add(x, y)
    return x + y
end
print(add(5, 3))
```

**test4.lua** - 命令行参数：
```lua
print("Script name:", arg[0])
for i = 1, #arg do
    print("Argument " .. i .. ":", arg[i])
end
```

**test5.lua** - 错误处理：
```lua
-- 故意的语法错误
local x = 
```

### 可能遇到的问题

#### 问题1：文件路径问题

**现象**：无法找到文件或打开文件失败

**解决方案**：
- 使用绝对路径或相对于当前工作目录的路径
- 添加详细的错误消息，显示尝试打开的完整路径
- 检查文件权限

#### 问题2：内存管理问题

**现象**：程序崩溃或内存泄漏

**解决方案**：
- 确保所有GC对象都注册到GC
- 使用RAII模式管理LuaState生命周期
- 在异常情况下也要正确清理资源

#### 问题3：错误信息不清晰

**现象**：错误消息难以理解或定位问题

**解决方案**：
- 包含文件名、行号、列号
- 显示出错的代码行
- 提供修复建议

---

## 📚 参考资源

### 关键文件

1. **已有实现参考**：
   - `tests/unit/compiler/test_lua_file_compilation.cpp` - 完整的编译流程示例
   - `tests/unit/vm/test_function_call.cpp` - VM执行示例
   - `src/repl.cpp` - REPL实现，包含编译执行流程

2. **Lua 5.1.5参考**：
   - `lua_c_analysis/src/lua.c` - 主程序实现
   - `lua_c_analysis/src/lauxlib.c` - 辅助库（文件加载）
   - `lua_c_analysis/src/ldo.c` - 执行控制

### 相关文档

- `docs/ARCHITECTURE.md` - 架构设计
- `docs/DEVELOPMENT_GUIDE.md` - 开发规范
- `docs/IMPLEMENTATION_PLAN.md` - 实施计划

---

## 🚀 后续任务预览

### P0任务3：修复高优先级TODO（1人日）

**任务清单**：
- 修复VM的CodeGenerator hack
- 完善Upvalues处理（2处）
- 实现nil键错误抛出

### P1任务：扩展标准库（6.5人日）

**任务清单**：
- 扩展基础库（16个函数，3人日）
- 实现字符串库（14个函数，2人日）
- 实现表库（7个函数，1.5人日）

---

**准备好开始了吗？** 🚀

从`executeScript()`函数开始，一步一步实现完整的脚本执行功能！

