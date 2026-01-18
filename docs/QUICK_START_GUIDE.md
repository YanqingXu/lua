# 快速开始指南：lua_in_cpp开发

> **适用对象**: 新加入项目的开发者或AI助手  
> **更新日期**: 2026-01-18  
> **预计阅读时间**: 10分钟

---

## 🎯 项目概览

**lua_in_cpp** 是一个使用现代C++17实现的Lua 5.1.5解释器。

**当前状态**：
- ✅ 核心功能完成度：**78%**
- ✅ 编译器和VM：**100%**
- 🔄 标准库：**50%**
- ⏳ 剩余工作量：**17人日**

---

## 📁 项目结构

```
lua/
├── src/                          # 源代码（20,572行）
│   ├── common/                   # 通用定义（types.hpp, config.hpp）
│   ├── core/                     # 核心类型（Value, Table, Function, GCString）
│   ├── gc/                       # 垃圾回收器
│   ├── vm/                       # 虚拟机（LuaState, VM, Stack）
│   ├── compiler/                 # 编译器（Lexer, Parser, CodeGen）
│   ├── io/                       # I/O系统
│   └── lib/                      # 标准库（base, math, io）
├── docs/                         # 文档
│   ├── PROGRESS_REPORT_2026_01_18.md    # 进度报告
│   ├── DEVELOPMENT_ROADMAP_2026.md      # 开发路线图
│   └── QUICK_START_GUIDE.md             # 本文档
└── tests/                        # 测试代码
```

---

## 🚀 立即开始

### 第1步：了解当前状态

**阅读关键文档**（15分钟）：
1. `docs/PROGRESS_REPORT_2026_01_18.md` - 了解整体进度
2. `docs/DEVELOPMENT_ROADMAP_2026.md` - 了解开发计划
3. `README.md` - 了解项目架构

**关键数据**：
- 总代码行数：20,572行
- 已完成：78%
- 待完成：22%（17人日）
- 核心功能：100%完成
- 标准库：50%完成

---

### 第2步：选择任务

**优先级P0**（本周必须完成）：
1. **任务1**：补充LuaState初始化步骤（2人日）
2. **任务2**：实现脚本执行功能（1人日）
3. **任务3**：修复高优先级TODO（1人日）

**优先级P1**（下周目标）：
4. **任务4**：扩展基础库（3人日，16个函数）
5. **任务5**：实现字符串库（2人日，14个函数）
6. **任务6**：实现表库（1.5人日，7个函数）

**优先级P2**（后续优化）：
7. **任务7**：完善I/O库（2人日，10个函数）
8. **任务8**：实现OS库（1.5人日，11个函数）
9. **任务9**：修复所有TODO（2人日，21处）
10. **任务10**：集成测试和性能优化（2人日）

---

### 第3步：开始开发

#### 推荐起点：任务1（LuaState初始化）

**为什么从这里开始**：
- 影响所有后续功能
- 相对独立，不依赖其他任务
- 工作量明确（2人日）

**具体步骤**：

**子任务1.1：字符串表初始化**（0.5人日）
```cpp
// 文件：src/core/string_pool.hpp
class StringPool {
public:
    // 添加此方法
    void resize(usize newSize);
};

// 文件：src/vm/global_state.cpp
GlobalState::GlobalState() {
    // 在构造函数中添加
    stringPool.resize(32);  // 初始化字符串表大小
}
```

**参考实现**：
- `lua_c_analysis/src/lstring.c:luaS_resize`（第82-115行）

---

**子任务1.2：元方法名称初始化**（0.5人日）
```cpp
// 文件：src/vm/global_state.hpp
class GlobalState {
private:
    // 添加元方法名称数组
    GCString* tmname[TM_N];  // 17个元方法
};

// 文件：src/vm/global_state.cpp
void GlobalState::initMetamethodNames() {
    const char* names[] = {
        "__index", "__newindex", "__gc", "__mode", "__eq",
        "__add", "__sub", "__mul", "__div", "__mod",
        "__pow", "__unm", "__len", "__lt", "__le",
        "__concat", "__call"
    };
    
    for (int i = 0; i < TM_N; i++) {
        tmname[i] = stringPool.intern(names[i]);
        tmname[i]->markFixed();  // 标记为固定，防止GC回收
    }
}
```

**参考实现**：
- `lua_c_analysis/src/ltm.c:luaT_init`（第29-40行）

---

**子任务1.3：保留字初始化**（0.5人日）
```cpp
// 文件：src/compiler/lexer.cpp
void Lexer::initReservedWords() {
    const char* keywords[] = {
        "and", "break", "do", "else", "elseif", "end",
        "false", "for", "function", "if", "in", "local",
        "nil", "not", "or", "repeat", "return", "then",
        "true", "until", "while"
    };
    
    for (const char* kw : keywords) {
        GCString* str = stringPool.intern(kw);
        str->markFixed();  // 标记为固定
    }
}
```

**参考实现**：
- `lua_c_analysis/src/llex.c:luaX_init`（第62-75行）

---

**子任务1.4：内存错误消息固定**（0.25人日）
```cpp
// 文件：src/vm/global_state.cpp
GlobalState::GlobalState() {
    // 固定内存错误消息
    memerrmsg = stringPool.intern("not enough memory");
    memerrmsg->markFixed();
}
```

---

**子任务1.5：GC阈值设置**（0.25人日）
```cpp
// 文件：src/vm/global_state.cpp
GlobalState::GlobalState() {
    // 设置初始GC阈值
    gc.setThreshold(LUAI_GCPAUSE * gc.getTotalBytes() / 100);
}
```

**参考实现**：
- `lua_c_analysis/src/lgc.c`（GC阈值计算）

---

**验收标准**：
- [ ] 所有5个子任务完成
- [ ] 编译通过，无警告
- [ ] 通过单元测试
- [ ] 代码符合项目规范

---

## 📚 关键参考资源

### 主要参考

**lua_c_analysis**（Lua 5.1.5源码+详细注释）：
- 路径：`../lua_c_analysis/src/`
- 用途：实现参考、算法理解
- 关键文件：
  - `lstring.c` - 字符串管理
  - `ltm.c` - 元方法
  - `llex.c` - 词法分析
  - `lbaselib.c` - 基础库
  - `lstrlib.c` - 字符串库
  - `ltablib.c` - 表库

**lua_with_cpp**（现代C++实现）：
- 路径：`../lua_with_cpp/src/`
- 用途：架构参考、最佳实践
- 关键文件：
  - `lib/base/base_lib.cpp` - 基础库实现
  - `lib/string/string_lib.cpp` - 字符串库实现
  - `lib/table/table_lib.cpp` - 表库实现

---

### 文档资源

**项目文档**：
- `docs/PROGRESS_REPORT_2026_01_18.md` - 详细进度报告
- `docs/DEVELOPMENT_ROADMAP_2026.md` - 完整开发路线图
- `docs/ARCHITECTURE.md` - 架构设计文档

**Lua官方文档**：
- Lua 5.1 Reference Manual
- Programming in Lua (第3版)

---

## 🔧 开发工具和规范

### 编译器要求

```bash
# C++17标准
g++ -std=c++17 -Wall -Wextra -Werror

# Debug构建
g++ -std=c++17 -Wall -Wextra -Werror -g -O0 -fsanitize=address

# Release构建
g++ -std=c++17 -Wall -Wextra -Werror -O3 -DNDEBUG
```

### 代码规范

**命名规范**：
- 类名：PascalCase（如`LuaState`）
- 函数名：camelCase（如`pushValue`）
- 变量名：camelCase（如`stackTop`）
- 常量名：UPPER_CASE（如`MAX_STACK_SIZE`）

**注释规范**：
```cpp
/**
 * @brief 函数简要描述
 * @param name 参数描述
 * @return 返回值描述
 * @throws 异常描述
 */
```

---

## ✅ 质量保证

### 测试要求

**单元测试**：
- 每个新函数必须有对应的单元测试
- 测试覆盖率目标：>90%
- 使用Google Test框架

**集成测试**：
- 每周至少运行一次完整集成测试
- 测试覆盖所有标准库函数

### 代码审查清单

- [ ] 代码符合C++17标准
- [ ] 符合项目编码规范
- [ ] 有完整的文档注释
- [ ] 通过所有单元测试
- [ ] 无内存泄漏
- [ ] 无编译警告

---

## 📈 进度跟踪

### 每日进度报告模板

```markdown
## 2026-01-XX 进度报告

### 完成任务
- [x] 任务1.1 - 字符串表初始化

### 遇到问题
- 问题：字符串表resize时内存分配失败
- 解决：增加错误处理，使用try-catch

### 明日计划
- [ ] 任务1.2 - 元方法名称初始化
```

---

## 🎯 里程碑

### M6：标准库完成（2周后）

**目标**：
- ✅ 基础库100%（24/24函数）
- ✅ 字符串库100%（14/14函数）
- ✅ 表库100%（7/7函数）
- ✅ I/O库100%（18/18函数）
- ✅ OS库100%（11/11函数）
- ✅ 数学库100%（20/20函数，已完成）

**工作量**：10.5人日

---

### M7：1.0发布（4-6周后）

**目标**：
- ✅ 所有TODO修复完成
- ✅ 集成测试通过率>90%
- ✅ 性能达标（Lua 5.1.5的70%以上）
- ✅ 文档完善
- ✅ 发布v1.0.0

**工作量**：17人日

---

## 💡 常见问题

**Q1：从哪里开始？**
A1：建议从任务1（LuaState初始化）开始，这是所有后续功能的基础。

**Q2：如何查找参考实现？**
A2：在`lua_c_analysis/src/`目录下查找对应的C文件，所有代码都有详细的中文注释。

**Q3：遇到问题怎么办？**
A3：
1. 查看`docs/PROGRESS_REPORT_2026_01_18.md`中的TODO分析
2. 参考`lua_c_analysis`中的实现
3. 查看`lua_with_cpp`中的现代C++实现

**Q4：如何验证实现正确？**
A4：
1. 编写单元测试
2. 运行集成测试
3. 对比Lua 5.1.5的行为

---

## 🚀 下一步行动

**立即开始**：
1. 阅读`docs/DEVELOPMENT_ROADMAP_2026.md`
2. 选择任务1（LuaState初始化）
3. 查看参考实现（`lua_c_analysis/src/lstring.c`）
4. 开始编码

**预计时间**：
- 全职开发：2-3周完成所有任务
- 兼职开发：4-6周完成所有任务

---

**快速开始指南完成** ✅

**祝开发顺利！** 🎉
