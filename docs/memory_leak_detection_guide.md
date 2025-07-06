# 内存泄漏检测宏使用指南

本指南介绍如何使用项目中的内存泄漏检测宏来快速识别和定位内存泄漏问题。

## 快速开始

### 1. 基本使用

在任何测试函数的入口处添加以下宏即可启用内存泄漏检测：

```cpp
#include "../../src/test_framework/core/test_memory.hpp"

void MyTest::testSomeFunction() {
    AUTO_MEMORY_LEAK_TEST_GUARD();  // 添加这一行
    
    // 你的测试代码
    std::string source = "local x = 1; return x";
    bool result = compileAndCheckError(source);
    
    // 函数结束时自动检测并报告内存泄漏
}
```

### 2. 自定义测试名称

```cpp
void MyTest::testComplexScenario() {
    MEMORY_LEAK_TEST_GUARD("Complex Scenario Memory Test");
    
    // 测试代码
}
```

## 可用的宏函数

### 主要检测宏

| 宏名称 | 功能 | 使用场景 |
|--------|------|----------|
| `AUTO_MEMORY_LEAK_TEST_GUARD()` | 自动使用函数名作为测试名 | 大多数测试函数 |
| `MEMORY_LEAK_TEST_GUARD(name)` | 使用自定义测试名称 | 需要特定标识的测试 |
| `MEMORY_CHECKPOINT(desc)` | 在代码中设置内存检查点 | 复杂测试的中间检查 |
| `ASSERT_NO_MEMORY_LEAKS()` | 断言当前没有内存泄漏 | 关键测试点的验证 |

### 内存分配跟踪宏

| 宏名称 | 功能 | 替代函数 |
|--------|------|----------|
| `LEAK_TRACKED_MALLOC(size)` | 跟踪的内存分配 | `malloc()` |
| `LEAK_TRACKED_FREE(ptr)` | 跟踪的内存释放 | `free()` |
| `LEAK_TRACKED_NEW(type)` | 跟踪的对象创建 | `new type` |

## 输出示例

### 正常情况（无泄漏）
```
[MEMORY TEST] Starting: testVariableOutOfScope
[TEST START] testVariableOutOfScope - Memory leak detection enabled
[MEMORY CHECKPOINT] Before compilation: 0 bytes allocated
[MEMORY CHECKPOINT] After compilation: 0 bytes allocated
Variable out of scope detection: PASS
[MEMORY TEST] Finished: testVariableOutOfScope (15ms)
[MEMORY TEST] Peak usage: 2048 bytes
[MEMORY TEST] No leaks detected in: testVariableOutOfScope
```

### 检测到泄漏
```
[MEMORY TEST] Starting: testWithIntentionalLeak
[TEST START] testWithIntentionalLeak - Memory leak detection enabled
Test with intentional leak: PASS
[MEMORY TEST] Finished: testWithIntentionalLeak (8ms)
[MEMORY TEST] Peak usage: 1024 bytes
[MEMORY LEAK] Detected in test: testWithIntentionalLeak
[MEMORY LEAK] Leaked: 1024 bytes

=== MEMORY LEAK REPORT ===
Total leaks: 1
Total leaked bytes: 1024

Location: memory_leak_test_example.cpp:45 in testWithIntentionalLeak
  Count: 1 allocations
  Total: 1024 bytes
    - 1024 bytes (alive for 8ms)
```

## 高级用法

### 1. 在现有测试中集成

修改现有的测试函数：

```cpp
// 原始测试函数
void CompilerErrorTest::testVariableOutOfScope() {
    std::string source = R"(
        do
            local x = 1
        end
        return x  -- x is out of scope
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Variable out of scope detection", hasError);
}

// 添加内存检测后
void CompilerErrorTest::testVariableOutOfScope() {
    AUTO_MEMORY_LEAK_TEST_GUARD();  // 只需添加这一行
    
    std::string source = R"(
        do
            local x = 1
        end
        return x  -- x is out of scope
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Variable out of scope detection", hasError);
}
```

### 2. 批量测试内存检测

```cpp
void CompilerErrorTest::runAllTestsWithMemoryDetection() {
    std::vector<std::function<void()>> tests = {
        [this]() { testVariableOutOfScope(); },
        [this]() { testInvalidAssignments(); },
        [this]() { testFunctionScopeErrors(); }
    };
    
    for (auto& test : tests) {
        try {
            test();
        } catch (const std::exception& e) {
            std::cerr << "Test failed with exception: " << e.what() << std::endl;
        }
    }
}
```

### 3. 性能基准测试

```cpp
void performanceTestWithMemoryTracking() {
    MEMORY_LEAK_TEST_GUARD("Performance Benchmark");
    
    const int iterations = 1000;
    
    MEMORY_CHECKPOINT("Benchmark start");
    
    for (int i = 0; i < iterations; ++i) {
        std::string source = "local x = " + std::to_string(i) + "; return x";
        compileAndCheckError(source);
        
        if (i % 100 == 0) {
            MEMORY_CHECKPOINT("Iteration " + std::to_string(i));
        }
    }
    
    MEMORY_CHECKPOINT("Benchmark end");
}
```

## 配置选项

### 编译时配置

在Debug模式下，内存跟踪功能自动启用：

```cpp
#ifdef _DEBUG
    // 内存跟踪宏被激活
#else
    // 内存跟踪宏被禁用，性能不受影响
#endif
```

### 运行时控制

```cpp
// 手动启用/禁用检测
Lua::MemoryLeakDetector::enable();
Lua::MemoryLeakDetector::disable();

// 重置统计信息
Lua::MemoryLeakDetector::reset();

// 获取当前状态
size_t current = Lua::MemoryLeakDetector::getCurrentAllocated();
size_t peak = Lua::MemoryLeakDetector::getPeakAllocated();
```

## 最佳实践

### 1. 测试函数命名

使用描述性的函数名，因为`AUTO_MEMORY_LEAK_TEST_GUARD()`会使用函数名作为测试标识：

```cpp
// 好的命名
void testParserMemoryLeakOnInvalidSyntax() {
    AUTO_MEMORY_LEAK_TEST_GUARD();
    // ...
}

// 不够描述性
void test1() {
    AUTO_MEMORY_LEAK_TEST_GUARD();
    // ...
}
```

### 2. 关键点检查

在内存使用量可能发生显著变化的地方添加检查点：

```cpp
void testComplexCompilation() {
    AUTO_MEMORY_LEAK_TEST_GUARD();
    
    MEMORY_CHECKPOINT("Before parsing");
    auto ast = parser.parse(source);
    
    MEMORY_CHECKPOINT("After parsing");
    auto bytecode = compiler.compile(ast);
    
    MEMORY_CHECKPOINT("After compilation");
    // ...
}
```

### 3. 异常安全

内存检测器使用RAII模式，即使在异常情况下也能正确报告：

```cpp
void testWithPossibleException() {
    AUTO_MEMORY_LEAK_TEST_GUARD();
    
    try {
        // 可能抛出异常的代码
        riskyOperation();
    } catch (const std::exception& e) {
        // 即使发生异常，析构函数仍会检测内存泄漏
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
    // 函数结束时自动检测内存泄漏
}
```

## 故障排除

### 常见问题

1. **误报泄漏**：某些全局对象或静态变量可能被误认为泄漏
   - 解决方案：在测试开始前调用`MemoryLeakDetector::reset()`

2. **性能影响**：在Release模式下检测器被禁用
   - 确认：检查`_DEBUG`宏是否定义

3. **多线程问题**：检测器使用互斥锁保护
   - 注意：在多线程测试中可能有轻微性能影响

### 调试技巧

```cpp
// 手动生成详细报告
std::string report = Lua::MemoryLeakDetector::generateLeakReport();
std::cout << report << std::endl;

// 获取具体的泄漏信息
auto leaks = Lua::MemoryLeakDetector::getLeaks();
for (const auto& leak : leaks) {
    std::cout << "Leak: " << leak.size << " bytes at " 
              << leak.file << ":" << leak.line << std::endl;
}
```

## 与现有项目集成

### 修改现有测试

1. 在测试文件中包含头文件：
```cpp
#include "../../src/test_framework/core/test_memory.hpp"
```

2. 在每个测试函数开头添加宏：
```cpp
AUTO_MEMORY_LEAK_TEST_GUARD();
```

3. 编译并运行测试，观察输出

### 批量修改脚本

可以使用以下PowerShell脚本批量修改测试文件：

```powershell
# 为所有测试函数添加内存检测
$files = Get-ChildItem -Path "src/tests" -Filter "*.cpp" -Recurse

foreach ($file in $files) {
    $content = Get-Content $file.FullName
    $newContent = @()
    
    foreach ($line in $content) {
        if ($line -match "^\s*void\s+\w+Test::\w+\(\)\s*\{\s*$") {
            $newContent += $line
            $newContent += "    AUTO_MEMORY_LEAK_TEST_GUARD();"
        } else {
            $newContent += $line
        }
    }
    
    Set-Content -Path $file.FullName -Value $newContent
}
```

这个内存泄漏检测系统提供了一个简单而强大的方式来识别和定位内存问题，只需要在测试函数入口处添加一行宏即可获得详细的内存使用报告。
