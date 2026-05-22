# Lua C++ 单元测试框架文档

本文档详细介绍 `lua/tests/unit` 目录下轻量级单元测试框架的架构设计、使用的设计模式、以及如何添加新的测试用例。

---

## 目录

1. [设计模式详解](#1-设计模式详解)
2. [框架架构深度剖析](#2-框架架构深度剖析)
3. [添加新测试用例完整指南](#3-添加新测试用例完整指南)
4. [运行与调试测试](#4-运行与调试测试)
5. [最佳实践与常见陷阱](#5-最佳实践与常见陷阱)
6. [扩展框架功能](#6-扩展框架功能)

---

## 1. 设计模式详解

测试框架采用多种经典设计模式，既保证代码的可维护性，又提供良好的扩展性。

### 1.1 注册表模式 (Registry Pattern)

**实现位置**: `LuaTest::TestRegistry` 类

**核心思想**: 
- 提供一个全局单例，收集所有测试用例的元数据（套件名、测试名、回调函数）
- 解耦测试定义与测试执行，测试可以在任意源文件中注册
- 支持延迟注册（静态初始化时期完成）

**实现细节**:
```cpp
class TestRegistry {
    struct TestEntry {
        std::string suiteName;   // 测试套件名称
        std::string testName;    // 测试用例名称
        TestFunction func;       // 测试函数回调
    };
    std::vector<TestEntry> tests_;  // 所有已注册测试的集合
};
```

**优势**:
- **集中管理**: 所有测试通过统一接口注册，便于追踪和管理
- **动态扩展**: 新增测试无需修改框架核心代码
- **执行控制**: 可以按套件名、测试名过滤或重排执行顺序（未来扩展）

### 1.2 命令模式 (Command Pattern)

**实现位置**: `std::function<void(TestSuite&)>` 类型的测试回调

**核心思想**:
- 将每个测试用例封装为一个可调用对象
- 测试的具体逻辑对注册表透明，只需知道如何调用
- 支持参数化（通过 `TestSuite&` 传递上下文）

**实现细节**:
```cpp
using TestFunction = std::function<void(TestSuite&)>;

// 测试定义示例
void myTest(TestSuite& suite) {
    // 测试逻辑 + 断言
}

// 注册为命令对象
registry.registerTest("Suite", "Test", myTest);
```

**优势**:
- **解耦执行者与接收者**: 注册表只需调用 `func(suite)`，不关心内部实现
- **支持 Lambda**: 可以使用 lambda 表达式快速定义简单测试
- **便于异常处理**: 框架统一捕获异常并记录为测试失败

### 1.3 测试夹具模式 (Test Fixture Pattern)

**实现位置**: `LuaStdLibTestContext` 类

**核心思想**:
- 提供测试所需的可复用环境（Setup）和清理逻辑（Teardown）
- 使用 RAII 自动管理资源生命周期
- 减少测试代码中的样板代码（boilerplate）

**实现细节**:
```cpp
class LuaStdLibTestContext {
public:
    explicit LuaStdLibTestContext(StdLibOpenFunction openFunc);
    ~LuaStdLibTestContext();  // 自动清理 LuaState
    
    // 提供便捷接口
    Lua::LuaState* getState() const;
    void clearStack() const;
    Lua::Value getGlobal(const char* name) const;
    bool ensureGlobalFunction(const char* name, TestSuite& suite, 
                             const std::string& message) const;
    int invoke(const char* name, 
               const std::function<void(Lua::LuaState*)>& pushArgs) const;
    
private:
    Lua::LuaState* state_;  // 管理的 Lua 状态机
};
```

**使用示例**:
```cpp
void testPrint(TestSuite& suite) {
    // 自动创建 LuaState 并调用 openBaseLib
    LuaStdLibTestContext ctx(openBaseLib);
    
    // 验证 print 函数存在
    if (!ctx.ensureGlobalFunction("print", suite, "print exists")) {
        return;  // 早期退出，避免空指针访问
    }
    
    // 调用 print 并检查返回值
    int ret = ctx.invoke("print", [](LuaState* L) {
        L->pushString(L->getGlobalState().getStringPool().intern("Hello"));
    });
    ASSERT_EQ(suite, ret, 0, "print returns 0");
    
    // ctx 析构时自动释放 LuaState
}
```

**优势**:
- **资源安全**: RAII 保证异常情况下也能正确清理
- **代码复用**: 多个测试共享相同的初始化逻辑
- **隔离性**: 每个测试使用独立的 Lua 状态，避免相互污染
- **便捷接口**: 封装常见操作（获取全局变量、调用函数、清栈等）

### 1.4 模板方法模式 (Template Method Pattern)

**实现位置**: 断言宏 `ASSERT_TRUE`、`ASSERT_EQ`、`ASSERT_FALSE`

**核心思想**:
- 定义算法骨架（评估条件 → 记录结果 → 添加到套件）
- 将具体条件判断延迟到宏调用处
- 统一错误报告格式

**实现细节**:
```cpp
#define ASSERT_TRUE(suite, condition, testName) \
    do { \
        bool result = (condition); \
        suite.addResult(LuaTest::TestResult(testName, result, \
                       result ? "" : "Expected true")); \
    } while(0)

#define ASSERT_EQ(suite, expected, actual, testName) \
    do { \
        bool result = ((expected) == (actual)); \
        suite.addResult(LuaTest::TestResult(testName, result, \
                       result ? "" : "Values not equal")); \
    } while(0)
```

**优势**:
- **一致性**: 所有断言使用相同的报告机制
- **可扩展**: 添加新断言宏时继承相同模式
- **类型安全**: 使用 `do-while(0)` 确保宏像语句一样使用

### 1.5 外观模式 (Facade Pattern)

**实现位置**: `test_runner.cpp`

**核心思想**:
- 为复杂的测试注册和执行流程提供简单统一的入口
- 隐藏内部注册表、套件管理等细节
- 便于集成到构建系统和 CI/CD 流程

**实现细节**:
```cpp
int main() {
    printHeader();
    
    // 1. 注册所有测试（外观简化了注册流程）
    registerValueTests();
    registerGCStringTests();
    // ... 更多注册函数
    
    // 2. 统一执行（隐藏了迭代、异常处理等细节）
    LuaTest::TestRegistry& registry = LuaTest::TestRegistry::getInstance();
    int failedTests = registry.runAllTests();
    
    // 3. 生成报告
    printSummary(totalTests, failedTests);
    
    return failedTests > 0 ? 1 : 0;
}
```

**优势**:
- **简化接口**: 用户只需关注注册函数和运行结果
- **统一入口**: 便于脚本化和自动化测试
- **易于维护**: 修改执行逻辑不影响测试代码

---

## 2. 框架架构深度剖析

### 2.1 目录结构与文件职责

```
lua/tests/unit/
├── framework/                    # 测试框架核心
│   ├── test_framework.hpp       # 核心框架头文件
│   │   ├── TestResult           # 单个测试结果结构
│   │   ├── TestSuite            # 测试套件类
│   │   ├── TestRegistry         # 全局测试注册表（单例）
│   │   ├── LuaStdLibTestContext # Lua 标准库测试夹具
│   │   └── 断言宏定义            # ASSERT_TRUE, ASSERT_EQ 等
│   │
│   ├── test_framework.cpp       # 框架实现文件
│   │   └── LuaStdLibTestContext 实现
│   │
│   ├── test_registry.hpp        # 测试注册函数声明
│   │   └── 所有 register*Tests() 函数的前向声明
│   │
│   ├── test_runner.cpp          # 独立可执行测试入口
│   │   ├── main() 函数
│   │   ├── 调用所有注册函数
│   │   └── 生成测试报告
│   │
│   └── README.md                # 本文档
│
├── core/                        # 核心组件测试
│   ├── test_value.cpp           # Value 类测试
│   ├── test_gc_string.cpp       # GCString 和 StringPool 测试
│   ├── test_table.cpp           # Table 类测试
│   └── test_function.cpp        # Function/Proto 测试
│
├── gc/                          # 垃圾回收测试
│   └── test_gc.cpp              # GC 系统测试
│
├── vm/                          # 虚拟机测试
│   └── test_vm_core.cpp         # VM 核心组件测试
│
├── compiler/                    # 编译器测试
│   ├── test_binary_unary_expr.cpp   # 表达式代码生成测试
│   ├── test_function_codegen.cpp    # 函数代码生成测试
│   └── test_lua_functions.cpp       # Lua 文件编译测试
│
├── stdlib/                      # 标准库测试
│   └── test_baselib.cpp         # 基础库函数测试
│
└── metamethod/                  # 元方法测试
    ├── test_metamethod_arith.cpp    # 算术元方法测试
    └── test_metamethod_complete.cpp # 完整元方法测试
```

### 2.2 核心组件详解

#### 2.2.1 TestResult - 测试结果封装

```cpp
struct TestResult {
    std::string testName;  // 测试名称（用于报告）
    bool passed;           // 是否通过
    std::string message;   // 失败时的错误信息
    
    TestResult(const std::string& name, bool pass, 
               const std::string& msg = "")
        : testName(name), passed(pass), message(msg) {}
};
```

**职责**: 封装单个断言的结果，便于统一处理和报告

#### 2.2.2 TestSuite - 测试套件管理器

```cpp
class TestSuite {
public:
    TestSuite(const std::string& name);
    
    // 添加测试结果（由断言宏调用）
    void addResult(const TestResult& result);
    
    // 打印本套件的测试报告
    void printReport() const;
    
    // 统计信息
    int getFailCount() const;
    int getPassCount() const;
    
private:
    std::string suiteName_;           // 套件名称
    std::vector<TestResult> results_; // 所有结果
    int passCount_;                   // 通过数量
    int failCount_;                   // 失败数量
};
```

**关键方法**:
- `addResult()`: 记录断言结果，更新统计信息
- `printReport()`: 格式化输出测试报告
  ```
  ========================================
  Test Suite: Base Library
  ========================================
    [PASS] print function exists
    [FAIL] type returns string - Expected true
  ----------------------------------------
  Total: 10 | Pass: 9 | Fail: 1
  ========================================
  ```

#### 2.2.3 TestRegistry - 全局测试注册中心

```cpp
class TestRegistry {
public:
    using TestFunction = std::function<void(TestSuite&)>;
    
    // 获取单例实例
    static TestRegistry& getInstance();
    
    // 注册测试用例
    void registerTest(const std::string& suiteName, 
                     const std::string& testName, 
                     TestFunction func);
    
    // 运行所有已注册的测试
    int runAllTests();
    
private:
    struct TestEntry {
        std::string suiteName;
        std::string testName;
        TestFunction func;
    };
    
    std::vector<TestEntry> tests_;
};
```

**执行流程** (`runAllTests()`):
1. 按套件名分组测试
2. 为每个套件创建 `TestSuite` 对象
3. 依次调用测试函数，捕获异常
4. 打印套件报告
5. 累计失败数量并返回

**异常处理**:
```cpp
try {
    test.func(*suite);  // 调用测试函数
} catch (const std::exception& e) {
    // 将异常记录为测试失败
    suite->addResult(TestResult(test.testName, false, 
                                std::string("Exception: ") + e.what()));
}
```

#### 2.2.4 LuaStdLibTestContext - Lua 测试环境管理

**构造与析构**:
```cpp
LuaStdLibTestContext::LuaStdLibTestContext(StdLibOpenFunction openFunc)
    : state_(Lua::LuaState::newState()) {
    if (state_ && openFunc) {
        openFunc(state_);  // 打开标准库（如 openBaseLib）
    }
}

LuaStdLibTestContext::~LuaStdLibTestContext() {
    if (state_) {
        delete state_;  // 自动释放 Lua 状态
        state_ = nullptr;
    }
}
```

**核心方法**:

1. **ensureGlobalFunction**: 验证函数存在并记录结果
   ```cpp
   bool LuaStdLibTestContext::ensureGlobalFunction(
       const char* name, TestSuite& suite, const std::string& message) const {
       bool ok = getGlobal(name).isFunction();
       suite.addResult(TestResult(message, ok, 
           ok ? "" : ("missing function: " + std::string(name))));
       return ok;
   }
   ```

2. **invoke**: 简化函数调用流程
   ```cpp
   int LuaStdLibTestContext::invoke(
       const char* name, 
       const std::function<void(Lua::LuaState*)>& pushArgs) const {
       
       Lua::Value func = getGlobal(name);
       if (!func.isFunction()) return -1;
       
       state_->getStack().clear();
       state_->pushFunction(func.asFunction());
       
       if (pushArgs) pushArgs(state_);  // 用户提供参数
       
       return func.asFunction()->getCFunction()(state_);
   }
   ```

**使用场景**:
- 测试 Lua 标准库函数（base, string, math 等）
- 需要干净的 Lua 环境的测试
- 需要频繁调用 Lua 函数的测试

### 2.3 断言宏系统

**设计原则**:
- 宏名称清晰表达预期行为（`ASSERT_TRUE`, `ASSERT_EQ`）
- 自动记录结果到 `TestSuite`
- 提供自定义测试名称（用于报告）

**现有断言**:

| 宏 | 用途 | 示例 |
|----|------|------|
| `ASSERT_TRUE(suite, cond, name)` | 断言条件为真 | `ASSERT_TRUE(suite, x > 0, "x is positive")` |
| `ASSERT_FALSE(suite, cond, name)` | 断言条件为假 | `ASSERT_FALSE(suite, ptr == nullptr, "ptr not null")` |
| `ASSERT_EQ(suite, exp, act, name)` | 断言相等 | `ASSERT_EQ(suite, 42, result, "answer is 42")` |

**扩展断言示例**:
```cpp
// 断言不等
#define ASSERT_NE(suite, expected, actual, testName) \
    do { \
        bool result = ((expected) != (actual)); \
        suite.addResult(LuaTest::TestResult(testName, result, \
                       result ? "" : "Values are equal")); \
    } while(0)

// 断言近似相等（浮点数）
#define ASSERT_NEAR(suite, expected, actual, epsilon, testName) \
    do { \
        bool result = (std::abs((expected) - (actual)) < (epsilon)); \
        suite.addResult(LuaTest::TestResult(testName, result, \
                       result ? "" : "Values differ by more than epsilon")); \
    } while(0)
```

---

## 3. 添加新测试用例完整指南

### 3.1 命名规范

**文件命名**: `test_<模块名>.cpp`
- 使用小写字母和下划线
- 清晰描述测试的模块或功能
- 例如: `test_string_lib.cpp`, `test_coroutine.cpp`

**函数命名**: `test<功能描述>Wrapper` 或 `test<功能>`
- 使用驼峰命名
- 清晰描述测试的具体功能
- 例如: `testStringLenWrapper`, `testTableInsert`

**注册函数命名**: `register<模块>Tests`
- 例如: `registerStringLibTests`, `registerCoroutineTests`

### 3.2 文件结构模板

```cpp
/**
 * @file test_my_feature.cpp
 * @brief 我的功能模块测试
 */

#include "../framework/test_framework.hpp"  // 注意相对路径
#include "my_feature.hpp"  // 被测试模块的头文件
#include "vm/state/lua_state.hpp"
#include "core/value.hpp"

using namespace Lua;
using namespace LuaTest;

// 匿名命名空间，避免符号冲突
namespace {

// 可选：定义套件常量
constexpr const char* kSuiteName = "My Feature";

// 辅助函数（可选）
void setupHelper(LuaState* L) {
    // 公共初始化逻辑
}

// 测试函数 1
void testBasicFunctionality(TestSuite& suite) {
    // 测试实现
}

// 测试函数 2
void testEdgeCases(TestSuite& suite) {
    // 测试实现
}

// 更多测试...

} // namespace

// 注册函数（必须在全局命名空间）
void registerMyFeatureTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest(kSuiteName, "basic functionality", 
                         testBasicFunctionality);
    registry.registerTest(kSuiteName, "edge cases", 
                         testEdgeCases);
    // 注册更多测试...
}
```

### 3.3 不同类型测试的实现模式

#### 3.3.1 基础单元测试（不依赖 Lua）

```cpp
void testValueCreation(TestSuite& suite) {
    // 测试 Value 类的基本功能
    Value nilVal;
    ASSERT_TRUE(suite, nilVal.isNil(), "default value is nil");
    
    Value numVal(42.0);
    ASSERT_TRUE(suite, numVal.isNumber(), "number value type check");
    ASSERT_EQ(suite, 42.0, numVal.asNumber(), "number value content");
    
    Value boolVal(true);
    ASSERT_TRUE(suite, boolVal.isBoolean(), "boolean type check");
    ASSERT_TRUE(suite, boolVal.asBoolean(), "boolean value is true");
}
```

#### 3.3.2 Lua 标准库测试（使用 LuaStdLibTestContext）

```cpp
void testStringLen(TestSuite& suite) {
    // 创建测试环境，自动打开 string 库
    LuaStdLibTestContext ctx(openStringLib);
    
    // 验证 string.len 函数存在
    if (!ctx.ensureGlobalFunction("string", suite, "string table exists")) {
        return;
    }
    
    LuaState* L = ctx.getState();
    Value stringTable = ctx.getGlobal("string");
    
    if (!stringTable.isTable()) {
        suite.addResult(TestResult("string is table", false, "not a table"));
        return;
    }
    
    // 获取 string.len 函数
    Value lenFunc = stringTable.asTable()->get(
        Value(L->getGlobalState().getStringPool().intern("len"))
    );
    
    ASSERT_TRUE(suite, lenFunc.isFunction(), "string.len exists");
    
    // 测试调用
    L->getStack().clear();
    L->pushFunction(lenFunc.asFunction());
    L->pushString(L->getGlobalState().getStringPool().intern("hello"));
    
    int ret = lenFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "string.len returns 1 value");
    
    Value result = L->top();
    ASSERT_TRUE(suite, result.isNumber(), "string.len returns number");
    ASSERT_EQ(suite, 5.0, result.asNumber(), "len('hello') == 5");
}
```

#### 3.3.3 VM 功能测试（需要执行 Lua 代码）

```cpp
void testLocalVariables(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    // 执行 Lua 代码片段
    const char* code = R"(
        local x = 10
        local y = 20
        local z = x + y
        return z
    )";
    
    try {
        // 编译并执行
        Lexer lexer(code);
        Parser parser(lexer);
        ASTNode* ast = parser.parseChunk();
        
        CodeGen codegen(L->getGlobalState());
        Proto* proto = codegen.generate(ast);
        
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        
        // 调用函数
        L->getStack().clear();
        L->pushFunction(func);
        VM::callFunction(L, 0, 1);
        
        Value result = L->top();
        ASSERT_TRUE(suite, result.isNumber(), "result is number");
        ASSERT_EQ(suite, 30.0, result.asNumber(), "10 + 20 == 30");
        
        delete ast;
    } catch (const std::exception& e) {
        suite.addResult(TestResult("local variables", false, 
                                   std::string("Exception: ") + e.what()));
    }
    
    delete L;
}
```

#### 3.3.4 GC 系统测试

```cpp
void testGCCollection(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    
    // 记录初始对象数量
    size_t initialCount = gc.getObjectCount();
    
    // 创建一些 GC 对象
    for (int i = 0; i < 100; ++i) {
        Table* t = new Table();
        gc.registerObject(t);
        // 不保存引用，这些对象应该被回收
    }
    
    ASSERT_TRUE(suite, gc.getObjectCount() > initialCount, 
               "objects created");
    
    // 触发 GC
    gc.collect();
    
    // 验证对象被回收
    ASSERT_EQ(suite, initialCount, gc.getObjectCount(), 
             "objects collected");
    
    delete L;
}
```

### 3.4 集成到构建系统

#### 步骤 1: 在 `test_registry.hpp` 中添加声明

```cpp
/**
 * @brief 注册我的功能模块测试
 */
void registerMyFeatureTests();
```

#### 步骤 2: 在 `test_runner.cpp` 中调用注册函数

```cpp
int main() {
    printHeader();
    
    // ... 现有注册调用 ...
    
    registerMyFeatureTests();  // 添加这一行
    
    // ... 执行测试 ...
}
```

#### 步骤 3: 更新构建文件

**对于 CMake** (`CMakeLists.txt`):
```cmake
# 在测试源文件列表中添加（根据功能模块分类）
set(TEST_SOURCES
    tests/unit/core/test_value.cpp
    tests/unit/core/test_table.cpp
    tests/unit/core/test_my_feature.cpp  # 新增到相应类别
    tests/unit/stdlib/test_baselib.cpp
    # ... 其他测试文件 ...
)
```

**对于 Visual Studio** (`lua.vcxproj`):
```xml
<ItemGroup>
  <!-- 根据功能分类添加 -->
  <ClCompile Include="tests\unit\core\test_my_feature.cpp" />
</ItemGroup>
```

并在 `lua.vcxproj.filters` 中添加：
```xml
<ClCompile Include="tests\unit\core\test_my_feature.cpp">
  <Filter>tests\unit\core</Filter>
</ClCompile>
```xml
<ClCompile Include="tests\unit\test_my_feature.cpp">
  <Filter>tests\unit</Filter>
</ClCompile>
```

### 3.5 完整示例：从零实现一个测试文件

假设我们要测试新实现的 `math` 库：

```cpp
/**
 * @file test_mathlib.cpp
 * @brief 数学库函数测试
 */

#include "test_framework.hpp"
#include "lib/mathlib.hpp"
#include "vm/state/lua_state.hpp"
#include "core/string_pool.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Math Library";

// 辅助函数：获取 math 表中的函数
Value getMathFunction(LuaState* L, const char* name) {
    Value mathTable = L->getGlobal("math");
    if (!mathTable.isTable()) {
        return Value();
    }
    
    GCString* key = L->getGlobalState().getStringPool().intern(name);
    return mathTable.asTable()->get(Value(key));
}

void testMathAbs(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();
    
    Value absFunc = getMathFunction(L, "abs");
    if (!absFunc.isFunction()) {
        suite.addResult(TestResult("math.abs exists", false, 
                                   "function not found"));
        return;
    }
    
    // 测试正数
    L->getStack().clear();
    L->pushFunction(absFunc.asFunction());
    L->pushNumber(42.0);
    int ret = absFunc.asFunction()->getCFunction()(L);
    
    ASSERT_EQ(suite, ret, 1, "abs returns 1 value");
    Value result = L->top();
    ASSERT_EQ(suite, 42.0, result.asNumber(), "abs(42) == 42");
    
    // 测试负数
    L->getStack().clear();
    L->pushFunction(absFunc.asFunction());
    L->pushNumber(-42.0);
    ret = absFunc.asFunction()->getCFunction()(L);
    
    result = L->top();
    ASSERT_EQ(suite, 42.0, result.asNumber(), "abs(-42) == 42");
    
    // 测试零
    L->getStack().clear();
    L->pushFunction(absFunc.asFunction());
    L->pushNumber(0.0);
    ret = absFunc.asFunction()->getCFunction()(L);
    
    result = L->top();
    ASSERT_EQ(suite, 0.0, result.asNumber(), "abs(0) == 0");
}

void testMathSqrt(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();
    
    Value sqrtFunc = getMathFunction(L, "sqrt");
    ASSERT_TRUE(suite, sqrtFunc.isFunction(), "math.sqrt exists");
    
    // 测试完全平方数
    int ret = ctx.invoke("sqrt", [&](LuaState* s) {
        // 注意：这里需要通过 math 表调用
        Value mathTable = s->getGlobal("math");
        Value sqrtFunc = getMathFunction(s, "sqrt");
        s->getStack().clear();
        s->pushFunction(sqrtFunc.asFunction());
        s->pushNumber(16.0);
    });
    
    // 或者更简单的方式（如果 invoke 支持表方法）
    // ... 具体实现取决于框架设计
}

void testMathConstants(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();
    
    Value mathTable = L->getGlobal("math");
    ASSERT_TRUE(suite, mathTable.isTable(), "math table exists");
    
    // 检查 math.pi
    GCString* piKey = L->getGlobalState().getStringPool().intern("pi");
    Value piVal = mathTable.asTable()->get(Value(piKey));
    ASSERT_TRUE(suite, piVal.isNumber(), "math.pi is number");
    
    // 验证 pi 的精度（近似相等）
    double pi = piVal.asNumber();
    bool piCorrect = std::abs(pi - 3.14159265358979323846) < 1e-10;
    ASSERT_TRUE(suite, piCorrect, "math.pi value correct");
    
    // 检查 math.huge
    GCString* hugeKey = L->getGlobalState().getStringPool().intern("huge");
    Value hugeVal = mathTable.asTable()->get(Value(hugeKey));
    ASSERT_TRUE(suite, hugeVal.isNumber(), "math.huge is number");
    ASSERT_TRUE(suite, std::isinf(hugeVal.asNumber()), 
               "math.huge is infinity");
}

} // namespace

void registerMathLibTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest(kSuiteName, "abs function", testMathAbs);
    registry.registerTest(kSuiteName, "sqrt function", testMathSqrt);
    registry.registerTest(kSuiteName, "constants", testMathConstants);
}
```

---

## 4. 运行与调试测试

### 4.1 使用 CMake 构建和运行

```bash
# 1. 配置构建（首次或修改 CMakeLists.txt 后）
cd lua
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 2. 编译测试
cmake --build build --config Debug --target test_runner

# 3. 运行测试
# Windows
build\Debug\test_runner.exe

# Linux/macOS
./build/test_runner

# 4. 只运行特定配置
cmake --build build --config Release
build\Release\test_runner.exe
```

**CMake 高级选项**:
```bash
# 并行编译（使用 4 个线程）
cmake --build build -j 4

# 详细输出（查看编译命令）
cmake --build build --verbose

# 清理重建
cmake --build build --clean-first
```

### 4.2 使用 Visual Studio

1. **打开项目**: 打开 `lua.sln` 或 直接打开 `lua` 文件夹（CMake 项目）

2. **设置启动项**:
   - 右键点击 Solution Explorer 中的 `test_runner` 项目
   - 选择 "Set as Startup Project"

3. **配置调试选项**:
   - 右键 `test_runner` → Properties
   - Configuration Properties → Debugging
   - 可以添加命令行参数（未来如果支持测试过滤）

4. **编译与运行**:
   - `Ctrl + F5`: 运行但不调试
   - `F5`: 调试模式运行
   - `Ctrl + Shift + B`: 仅编译

5. **断点调试**:
   - 在测试函数中设置断点
   - F5 启动调试
   - 使用 Watch 窗口检查变量
   - 使用 Call Stack 追踪调用链

### 4.3 解读测试输出

**正常输出示例**:
```
========================================
Lua C++ Interpreter - Unit Test Suite
========================================
Test Framework: Custom Lightweight Framework
Date: 2025-11-23
========================================

[INFO] Registering tests...
[INFO] All tests registered.
[INFO] Starting test execution...

========================================
Test Suite: Base Library
========================================
  [PASS] print function exists
  [PASS] print returns 0
  [PASS] type function exists
  [PASS] type returns 1 value
  [PASS] type returns string
  [PASS] type(42) == 'number'
  [PASS] type('hello') == 'string'
  [PASS] type(nil) == 'nil'
----------------------------------------
Total: 8 | Pass: 8 | Fail: 0
========================================

========================================
Test Suite: Value
========================================
  [PASS] default value is nil
  [PASS] number value type check
  [FAIL] number value content - Values not equal
----------------------------------------
Total: 3 | Pass: 2 | Fail: 1
========================================

========================================
Test Summary
========================================
Total Tests: 11
Passed: 10
Failed: 1
========================================

✗ SOME TESTS FAILED!
```

**异常情况处理**:
```
========================================
Test Suite: Crash Test
========================================
  [PASS] normal test
  [FAIL] crash test - Exception: Access violation at 0x00000000
  [PASS] recovery test
----------------------------------------
Total: 3 | Pass: 2 | Fail: 1
========================================
```

### 4.4 调试失败的测试

**步骤 1: 定位失败测试**
- 查看输出中的 `[FAIL]` 标记
- 注意测试名称和错误消息

**步骤 2: 在测试函数中设置断点**
```cpp
void testProblem(TestSuite& suite) {
    int x = calculateSomething();  // 在这里设置断点
    ASSERT_EQ(suite, 42, x, "calculation correct");
}
```

**步骤 3: 检查局部变量**
- 使用 IDE 的 Watch 窗口
- 检查 Lua 栈状态 (`L->getStack()`)
- 检查全局变量 (`L->getGlobal()`)

**步骤 4: 添加诊断输出**
```cpp
void testDebug(TestSuite& suite) {
    Value result = computeValue();
    
    // 临时添加诊断信息
    std::cout << "DEBUG: result type = " << result.getType() << std::endl;
    std::cout << "DEBUG: result value = " << result.toString() << std::endl;
    
    ASSERT_TRUE(suite, result.isNumber(), "result is number");
}
```

**步骤 5: 隔离问题**
- 将复杂测试分解为多个小测试
- 使用二分法定位问题代码段
- 检查测试依赖（是否依赖其他测试的状态）

---

## 5. 最佳实践与常见陷阱

### 5.1 测试设计原则

#### 5.1.1 单一职责原则
每个测试只验证一个功能点：
```cpp
// ❌ 不好：测试太多功能
void testEverything(TestSuite& suite) {
    testAddition();
    testSubtraction();
    testMultiplication();
    testDivision();
}

// ✅ 好：每个测试专注一个功能
void testAddition(TestSuite& suite) { /* ... */ }
void testSubtraction(TestSuite& suite) { /* ... */ }
```

#### 5.1.2 独立性原则
测试之间不应相互依赖：
```cpp
// ❌ 不好：依赖全局状态
LuaState* globalState = nullptr;

void testSetup(TestSuite& suite) {
    globalState = LuaState::newState();
}

void testFeature(TestSuite& suite) {
    // 依赖 testSetup 先执行
    globalState->doSomething();
}

// ✅ 好：每个测试独立创建环境
void testFeature(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    ctx.getState()->doSomething();
}
```

#### 5.1.3 可重复性原则
测试结果应该稳定可重复：
```cpp
// ❌ 不好：依赖随机性或时间
void testUnstable(TestSuite& suite) {
    int random = rand() % 100;
    ASSERT_TRUE(suite, random < 50, "random < 50");
}

// ✅ 好：使用固定种子或不依赖随机性
void testStable(TestSuite& suite) {
    srand(12345);  // 固定种子
    int value = deterministicFunction();
    ASSERT_EQ(suite, 42, value, "deterministic result");
}
```

### 5.2 常见陷阱与解决方案

#### 5.2.1 内存泄漏

**问题**:
```cpp
void testLeak(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    Table* t = new Table();
    L->getGlobalState().getGC().registerObject(t);
    
    // 测试逻辑...
    
    // 忘记清理！
}
```

**解决方案**:
```cpp
void testNoLeak(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    {
        Table* t = new Table();
        L->getGlobalState().getGC().registerObject(t);
        // 测试逻辑...
    }
    // 确保在函数结束前清理
    delete L;  // GC 会自动回收 t
}

// 或者使用 RAII
void testRaii(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);  // 自动清理
    // 测试逻辑...
}
```

#### 5.2.2 栈溢出

**问题**:
```cpp
void testStackOverflow(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    
    for (int i = 0; i < 10000; ++i) {
        L->pushNumber(i);  // 不清栈，导致溢出
    }
    
    delete L;
}
```

**解决方案**:
```cpp
void testStackSafe(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    LuaState* L = ctx.getState();
    
    for (int i = 0; i < 10000; ++i) {
        L->pushNumber(i);
        // 处理...
        ctx.clearStack();  // 定期清栈
    }
}
```

#### 5.2.3 空指针解引用

**问题**:
```cpp
void testNullPtr(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    
    Value func = ctx.getGlobal("nonexistent");
    // 直接使用，没有检查
    func.asFunction()->getCFunction()(ctx.getState());  // 崩溃！
}
```

**解决方案**:
```cpp
void testSafe(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    
    // 使用 ensureGlobalFunction 验证
    if (!ctx.ensureGlobalFunction("myFunc", suite, "function exists")) {
        return;  // 早期退出
    }
    
    // 或手动检查
    Value func = ctx.getGlobal("myFunc");
    if (!func.isFunction()) {
        suite.addResult(TestResult("function valid", false, 
                                   "not a function"));
        return;
    }
    
    // 安全使用
    func.asFunction()->getCFunction()(ctx.getState());
}
```

#### 5.2.4 浮点数比较

**问题**:
```cpp
void testFloatBad(TestSuite& suite) {
    double result = 0.1 + 0.2;
    ASSERT_EQ(suite, 0.3, result, "0.1 + 0.2 == 0.3");  // 可能失败！
}
```

**解决方案**:
```cpp
// 使用 epsilon 比较
void testFloatGood(TestSuite& suite) {
    double result = 0.1 + 0.2;
    double expected = 0.3;
    double epsilon = 1e-10;
    
    bool nearEqual = std::abs(result - expected) < epsilon;
    ASSERT_TRUE(suite, nearEqual, "0.1 + 0.2 ≈ 0.3");
}

// 或者定义专用断言宏（如前述的 ASSERT_NEAR）
```

### 5.3 性能测试建议

```cpp
void testPerformance(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    
    // 预热（避免首次调用开销）
    for (int i = 0; i < 100; ++i) {
        performOperation(L);
    }
    
    // 计时
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        performOperation(L);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    std::cout << "Performance: " << duration << " ms for 10000 iterations" 
              << std::endl;
    
    // 性能阈值检查（可选）
    bool performanceOk = duration < 1000;  // 1秒内完成
    ASSERT_TRUE(suite, performanceOk, "performance within threshold");
    
    delete L;
}
```

---

## 6. 扩展框架功能

### 6.1 添加新的断言宏

```cpp
// 在 test_framework.hpp 中添加

// 断言字符串包含子串
#define ASSERT_CONTAINS(suite, haystack, needle, testName) \
    do { \
        std::string h = (haystack); \
        std::string n = (needle); \
        bool result = h.find(n) != std::string::npos; \
        suite.addResult(LuaTest::TestResult(testName, result, \
                       result ? "" : "String does not contain substring")); \
    } while(0)

// 断言抛出特定异常
#define ASSERT_THROWS(suite, expression, exceptionType, testName) \
    do { \
        bool caught = false; \
        try { \
            expression; \
        } catch (const exceptionType&) { \
            caught = true; \
        } catch (...) { \
        } \
        suite.addResult(LuaTest::TestResult(testName, caught, \
                       caught ? "" : "Expected exception not thrown")); \
    } while(0)

// 断言不抛出异常
#define ASSERT_NO_THROW(suite, expression, testName) \
    do { \
        bool noThrow = true; \
        try { \
            expression; \
        } catch (...) { \
            noThrow = false; \
        } \
        suite.addResult(LuaTest::TestResult(testName, noThrow, \
                       noThrow ? "" : "Unexpected exception thrown")); \
    } while(0)
```

### 6.2 支持测试过滤

在 `TestRegistry` 中添加过滤功能：

```cpp
class TestRegistry {
public:
    // 添加过滤方法
    void setFilter(const std::string& pattern) {
        filterPattern_ = pattern;
    }
    
    int runAllTests() {
        for (const auto& test : tests_) {
            // 检查是否匹配过滤器
            if (!filterPattern_.empty()) {
                std::string fullName = test.suiteName + "::" + test.testName;
                if (fullName.find(filterPattern_) == std::string::npos) {
                    continue;  // 跳过不匹配的测试
                }
            }
            
            // 运行测试...
        }
    }
    
private:
    std::string filterPattern_;
};
```

在 `test_runner.cpp` 中使用：
```cpp
int main(int argc, char* argv[]) {
    auto& registry = TestRegistry::getInstance();
    
    // 解析命令行参数
    if (argc > 1) {
        registry.setFilter(argv[1]);  // 例如: --filter="Base Library"
    }
    
    return registry.runAllTests();
}
```

### 6.3 添加测试夹具基类

```cpp
// 在 test_framework.hpp 中添加

template<typename T>
class TestFixture {
public:
    TestFixture() {
        derived()->setUp();
    }
    
    ~TestFixture() {
        derived()->tearDown();
    }
    
    // CRTP 模式获取派生类
    T* derived() { return static_cast<T*>(this); }
    
    // 默认实现（派生类可覆盖）
    void setUp() {}
    void tearDown() {}
};

// 使用示例
class MyTestFixture : public TestFixture<MyTestFixture> {
public:
    LuaState* L;
    
    void setUp() {
        L = LuaState::newState();
        openBaseLib(L);
    }
    
    void tearDown() {
        delete L;
        L = nullptr;
    }
};

void testWithFixture(TestSuite& suite) {
    MyTestFixture fixture;
    
    // 使用 fixture.L 进行测试
    Value func = fixture.L->getGlobal("print");
    ASSERT_TRUE(suite, func.isFunction(), "print exists");
}
```

### 6.4 生成 XML 报告（用于 CI）

```cpp
// 在 TestSuite 中添加方法
void TestSuite::generateXMLReport(std::ostream& out) const {
    out << "<testsuite name=\"" << suiteName_ << "\" "
        << "tests=\"" << (passCount_ + failCount_) << "\" "
        << "failures=\"" << failCount_ << "\">" << std::endl;
    
    for (const auto& result : results_) {
        out << "  <testcase name=\"" << result.testName << "\"";
        if (result.passed) {
            out << " />" << std::endl;
        } else {
            out << ">" << std::endl;
            out << "    <failure message=\"" << result.message << "\" />" 
                << std::endl;
            out << "  </testcase>" << std::endl;
        }
    }
    
    out << "</testsuite>" << std::endl;
}
```

---

## 总结

本测试框架采用了多种设计模式，提供了清晰的架构和易于扩展的接口。通过遵循本文档的指南，开发者可以：

1. **理解框架设计**: 掌握注册表、命令、夹具等模式的应用
2. **编写高质量测试**: 遵循最佳实践，避免常见陷阱
3. **快速添加新测试**: 使用模板和示例代码快速上手
4. **调试测试问题**: 利用 IDE 和诊断工具定位问题
5. **扩展框架功能**: 根据项目需求添加新特性

测试是保证代码质量的重要手段，希望本文档能帮助您更好地使用和扩展测试框架！
