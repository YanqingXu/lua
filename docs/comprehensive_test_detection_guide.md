# 综合测试检测指南

本指南介绍如何使用增强的测试检测系统来检测内存泄漏、无限递归、死循环和超时问题。

## 概述

新的综合检测系统可以检测以下问题：

1. **内存泄漏** - 未释放的内存分配
2. **无限递归** - 超过最大递归深度的函数调用
3. **死循环/无限循环** - 长时间无进展的循环
4. **超时** - 执行时间超过预设限制的测试
5. **死锁** - 程序停止响应的情况

## 快速开始

### 基本用法

```cpp
#include "../../src/test_framework/core/test_memory.hpp"

void MyTest::testSomeFunction() {
    AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT();  // 30秒超时，包含所有检测
    
    // 你的测试代码
    std::string source = "local x = 1; return x";
    bool result = compileAndCheckError(source);
    
    // 函数结束时自动检测所有问题
}
```

### 自定义超时时间

```cpp
void MyTest::testLongRunningFunction() {
    AUTO_COMPREHENSIVE_TEST_GUARD(60000);  // 60秒超时
    
    // 长时间运行的测试代码
}
```

## 可用的宏和功能

### 主要检测宏

| 宏名称 | 功能 | 参数 |
|--------|------|------|
| `AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT()` | 默认30秒超时的综合检测 | 无 |
| `AUTO_COMPREHENSIVE_TEST_GUARD(timeoutMs)` | 自定义超时的综合检测 | 超时毫秒数 |
| `COMPREHENSIVE_TEST_GUARD(name, timeoutMs)` | 自定义名称和超时的检测 | 测试名称, 超时毫秒数 |
| `RECURSION_GUARD()` | 单独的递归深度检测 | 无 |

### 操作记录宏（用于死循环检测）

| 宏名称 | 功能 | 使用场景 |
|--------|------|----------|
| `RECORD_OPERATION()` | 记录一次操作 | 在可能的死循环点 |
| `LOOP_OPERATION_RECORD(counter)` | 每1000次循环记录一次 | 在循环内部 |

## 检测机制详解

### 1. 无限递归检测

**检测原理**：跟踪线程本地的递归深度，超过1000层时抛出异常。

**示例**：
```cpp
void testInfiniteRecursion() {
    AUTO_COMPREHENSIVE_TEST_GUARD(5000);  // 5秒超时
    
    try {
        infiniteFunction();  // 这会触发递归检测
    } catch (const std::exception& e) {
        std::cout << "递归检测成功: " << e.what() << std::endl;
    }
}

void infiniteFunction() {
    RECURSION_GUARD();  // 必须在递归函数中添加
    infiniteFunction(); // 无限递归
}
```

**输出示例**：
```
[COMPREHENSIVE TEST] Starting: testInfiniteRecursion
[COMPREHENSIVE TEST] Timeout: 5000ms
Recursion depth: 999
Recursion depth: 1000
递归检测成功: Maximum recursion depth exceeded: 1001
[COMPREHENSIVE TEST] Max recursion depth reached: 1001
```

### 2. 死循环检测

**检测原理**：监控操作计数器，如果15秒内没有操作记录，判定为死循环。

**示例**：
```cpp
void testDeadLoop() {
    AUTO_COMPREHENSIVE_TEST_GUARD(20000);  // 20秒超时
    
    // 正确的循环 - 不会被误判
    for (int i = 0; i < 10000; ++i) {
        LOOP_OPERATION_RECORD(i);  // 记录操作
        // 正常的循环体
    }
    
    // 危险的循环 - 会被检测为死循环
    /*
    while (true) {
        // 没有 RECORD_OPERATION() 调用
        // 会在15秒后被检测为死循环
    }
    */
}
```

**输出示例**：
```
[DEADLOCK WARNING] No progress detected in 'testDeadLoop' for 5000ms
[DEADLOCK WARNING] No progress detected in 'testDeadLoop' for 10000ms
[DEADLOCK WARNING] No progress detected in 'testDeadLoop' for 15000ms
[DEADLOCK ERROR] Test 'testDeadLoop' appears to be in a deadlock or infinite loop!
[DEADLOCK ERROR] No operations detected for 15000ms
```

### 3. 超时检测

**检测原理**：启动独立的监控线程，超过指定时间强制终止测试。

**示例**：
```cpp
void testTimeout() {
    AUTO_COMPREHENSIVE_TEST_GUARD(3000);  // 3秒超时
    
    // 模拟长时间运行的操作
    std::this_thread::sleep_for(std::chrono::seconds(5));  // 会超时
}
```

**输出示例**：
```
[COMPREHENSIVE TEST] Starting: testTimeout
[COMPREHENSIVE TEST] Timeout: 3000ms
[TIMEOUT ERROR] Test 'testTimeout' exceeded timeout of 3000ms
[TIMEOUT ERROR] Possible infinite loop or recursion detected!
[TIMEOUT ERROR] Current recursion depth: 0
```

### 4. 内存泄漏检测

继承了原有的内存泄漏检测功能，详见 `memory_leak_detection_guide.md`。

## 实际应用示例

### 修复 testVariableOutOfScope 无限循环

```cpp
// 原始问题函数
void CompilerErrorTest::testVariableOutOfScope() {
    AUTO_COMPREHENSIVE_TEST_GUARD(10000);  // 10秒超时
    
    std::string source = R"(
        do
            local x = 1
        end
        return x  -- x is out of scope
    )";
    
    // 在可能的循环点记录操作
    RECORD_OPERATION();
    
    bool hasError = compileAndCheckError(source);
    
    RECORD_OPERATION();
    
    printTestResult("Variable out of scope detection", hasError);
}

// 如果 compileAndCheckError 内部有循环，也需要添加记录
bool compileAndCheckError(const std::string& source) {
    RECURSION_GUARD();  // 防止递归编译
    
    // 解析循环
    while (parser.hasMoreTokens()) {
        RECORD_OPERATION();  // 记录解析进度
        
        if (!parser.parseNext()) {
            break;
        }
    }
    
    return parser.hasErrors();
}
```

### 编译器解析器的安全包装

```cpp
class SafeParser {
public:
    bool parseWithDetection(const std::string& source) {
        AUTO_COMPREHENSIVE_TEST_GUARD(15000);  // 15秒超时
        
        int tokenPosition = 0;
        int lastPosition = -1;
        int stuckCount = 0;
        
        while (tokenPosition < source.length()) {
            RECORD_OPERATION();
            
            // 检测解析器是否卡住
            if (tokenPosition == lastPosition) {
                stuckCount++;
                if (stuckCount > 3) {
                    throw std::runtime_error("Parser stuck at position " + std::to_string(tokenPosition));
                }
            } else {
                stuckCount = 0;
            }
            
            lastPosition = tokenPosition;
            
            // 实际的解析逻辑
            tokenPosition = parseToken(tokenPosition);
            
            MEMORY_CHECKPOINT("Parsed to position " + std::to_string(tokenPosition));
        }
        
        return true;
    }
};
```

## 配置和调优

### 递归深度限制

```cpp
// 在 RecursionDetector 类中修改
static constexpr int MAX_RECURSION_DEPTH = 1000;  // 默认1000层
```

### 死循环检测间隔

```cpp
// 在 DeadlockDetector 类中修改
static constexpr size_t CHECK_INTERVAL_MS = 5000;  // 5秒检查一次
static constexpr size_t MAX_STALL_COUNT = 3;       // 最多3次无变化
```

### 默认超时时间

```cpp
// 修改默认超时
#define AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT() \
    AUTO_COMPREHENSIVE_TEST_GUARD(60000)  // 改为60秒
```

## 输出解读

### 正常完成的测试

```
[COMPREHENSIVE TEST] Starting: testNormalFunction
[COMPREHENSIVE TEST] Timeout: 30000ms
[TEST START] testNormalFunction - Memory leak detection enabled
[MEMORY CHECKPOINT] Processing step 0: 0 bytes allocated
[MEMORY CHECKPOINT] Processing step 100: 0 bytes allocated
[TIMEOUT CHECK] Test 'testNormalFunction' completed within timeout
[COMPREHENSIVE TEST] Completed: testNormalFunction in 1250ms
[COMPREHENSIVE TEST] Max recursion depth reached: 1
[MEMORY TEST] No leaks detected in: testNormalFunction
```

### 检测到问题的测试

```
[COMPREHENSIVE TEST] Starting: testProblematicFunction
[COMPREHENSIVE TEST] Timeout: 10000ms
[DEADLOCK WARNING] No progress detected in 'testProblematicFunction' for 5000ms
[MEMORY LEAK] Detected in test: testProblematicFunction
[MEMORY LEAK] Leaked: 2048 bytes
[TIMEOUT ERROR] Test 'testProblematicFunction' exceeded timeout of 10000ms
```

## 最佳实践

### 1. 选择合适的超时时间

- **快速单元测试**：5-10秒
- **集成测试**：30-60秒
- **压力测试**：2-5分钟

### 2. 在关键位置记录操作

```cpp
// 在循环中
for (int i = 0; i < largeNumber; ++i) {
    LOOP_OPERATION_RECORD(i);  // 每1000次记录一次
    // 循环体
}

// 在可能阻塞的操作前后
RECORD_OPERATION();
blockingOperation();
RECORD_OPERATION();
```

### 3. 递归函数保护

```cpp
void recursiveFunction(int depth) {
    RECURSION_GUARD();  // 必须在函数开头
    
    if (depth <= 0) return;
    
    recursiveFunction(depth - 1);
}
```

### 4. 异常处理

```cpp
void testWithExceptionHandling() {
    AUTO_COMPREHENSIVE_TEST_GUARD(15000);
    
    try {
        riskyOperation();
    } catch (const std::runtime_error& e) {
        // 递归或死循环异常
        std::cout << "Detected issue: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // 其他异常
        std::cout << "Other exception: " << e.what() << std::endl;
    }
}
```

## 故障排除

### 常见问题

1. **误报死循环**
   - 确保在长时间运行的循环中调用 `LOOP_OPERATION_RECORD()`
   - 检查是否有阻塞的I/O操作没有记录

2. **递归深度不够**
   - 调整 `MAX_RECURSION_DEPTH` 常量
   - 检查是否真的需要如此深的递归

3. **超时时间不合理**
   - 根据测试复杂度调整超时时间
   - 考虑将复杂测试拆分为多个简单测试

### 调试技巧

```cpp
// 手动检查当前状态
std::cout << "Current recursion depth: " << RecursionDetector::getCurrentDepth() << std::endl;
std::cout << "Current memory usage: " << MemoryLeakDetector::getCurrentAllocated() << std::endl;

// 强制记录操作
RECORD_OPERATION();
MEMORY_CHECKPOINT("Debug checkpoint");
```

## 与现有项目集成

### 批量修改现有测试

```cpp
// 将现有的内存检测宏替换为综合检测宏
// 原来：
// AUTO_MEMORY_LEAK_TEST_GUARD();

// 现在：
AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT();
```

### 渐进式集成

1. **第一阶段**：在问题测试中使用综合检测
2. **第二阶段**：在所有新测试中使用
3. **第三阶段**：逐步替换现有测试的检测宏

这个综合检测系统能够有效识别和定位无限递归、死循环、超时和内存泄漏问题，为您的测试提供全面的保护！
