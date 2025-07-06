# Lua 测试框架 2.0

## 概述

这是一个专门为 Lua 解释器项目设计的测试框架，提供了完整的测试基础设施和工具。Lua测试框架2.0是一个现代化、模块化的C++测试框架，提供了丰富的测试功能、内存安全检测、美观的输出格式和灵活的配置选项。

## 框架结构

```
test_framework/
├── README.md                 # 框架说明文档
├── core/                     # 核心框架代码
│   ├── test_macros.hpp       # 测试宏定义
│   ├── test_utils.hpp        # 测试工具类
│   └── test_memory.hpp       # 内存测试工具
├── formatting/               # 格式化模块
│   ├── format_formatter.hpp  # 测试格式化器
│   ├── format_formatter.cpp  # 格式化器实现
│   ├── format_config.hpp     # 配置管理
│   ├── format_config.cpp     # 配置实现
│   ├── format_colors.hpp     # 颜色定义
│   ├── format_colors.cpp     # 颜色实现
│   └── format_strategies.hpp # 格式化策略
└── tools/                    # 测试工具
    ├── check_naming.py       # 命名规范检查
    ├── check_naming_simple.py # 简化版命名检查
    └── check_naming.bat      # 批处理脚本
```

## 设计原则

1. **职责分离**: 框架代码与测试代码完全分离
2. **可复用性**: 框架可以被其他项目复用
3. **模块化**: 各个组件独立，便于维护
4. **易用性**: 提供简洁的API和宏定义
5. **扩展性**: 支持自定义格式化和配置

## 快速开始

### 1. 基本使用

```cpp
#include "../test_framework/core/test_utils.hpp"
#include "../test_framework/core/test_macros.hpp"

using namespace Lua::TestFramework;

class MyTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Tests", runBasicTests);
    }
    
private:
    static void runBasicTests() {
        RUN_TEST(MyTestClass, testAddition);
        RUN_TEST(MyTestClass, testSubtraction);
    }
};

class MyTestClass {
public:
    static void testAddition() {
        int result = 2 + 3;
        if (result != 5) {
            throw std::runtime_error("Addition test failed");
        }
    }
    
    static void testSubtraction() {
        int result = 5 - 3;
        if (result != 2) {
            throw std::runtime_error("Subtraction test failed");
        }
    }
};

int main() {
    RUN_TEST_SUITE(MyTestSuite);
    return 0;
}
```

### 2. 测试组织

```cpp
// 运行测试套件
RUN_TEST_SUITE(MyTestSuite);

// 运行测试模块
RUN_TEST_MODULE("Parser Module", ParserTestSuite);

// 运行测试组
RUN_TEST_GROUP("Basic Tests", runBasicTests);

// 运行单个测试
RUN_TEST(TestClass, testMethod);

// 运行安全测试（带超时和内存检测）
SAFE_RUN_TEST(TestClass, testMethod, 5000);
```

## 核心概念

### 测试层次结构

测试框架支持多层次的测试组织：

1. **MAIN** - 主测试级别（整个测试套件）
2. **MODULE** - 模块级别（如lexer、parser等）
3. **SUITE** - 测试套件级别（一组相关测试）
4. **GROUP** - 测试组级别（功能分组）
5. **INDIVIDUAL** - 单个测试级别

### 测试宏

#### 基础测试宏

```cpp
// 运行单个测试方法
RUN_TEST(TestClass, testMethod);

// 运行主测试函数
RUN_MAIN_TEST(MainTestClass, runAllTests);

// 运行模块测试
RUN_TEST_MODULE(ModuleTestClass, runModuleTests);

// 运行测试套件
RUN_TEST_SUITE(TestSuiteClass);

// 运行测试组
RUN_TEST_GROUP("Group Name", TestClass::runGroupTests);
```

#### 安全测试宏

```cpp
// 带超时和内存检测的安全测试
SAFE_RUN_TEST(TestClass, testMethod, timeoutMs);
```

**SAFE_RUN_TEST 宏特性：**
- **双重保护**：同时提供内存泄漏检测和超时监控
- **异常安全**：自动捕获和处理测试中的异常
- **完整报告**：提供详细的测试执行报告
- **灵活配置**：可自定义超时时间
- **易于使用**：与 RUN_TEST 宏使用方式相同，只需添加超时参数

**使用示例：**
```cpp
// 基础用法：5秒超时
SAFE_RUN_TEST(MyTestClass, testBasicFunction, 5000);

// 长时间测试：30秒超时
SAFE_RUN_TEST(MyTestClass, testComplexOperation, 30000);

// 快速测试：1秒超时
SAFE_RUN_TEST(MyTestClass, testQuickCheck, 1000);
```

#### 内存检测宏

```cpp
// 内存泄漏检测守卫
MEMORY_LEAK_TEST_GUARD("Test Name");
```

## 高级功能

### 1. 内存安全测试

```cpp
class MemoryTestClass {
public:
    static void testMemoryAllocation() {
        MEMORY_LEAK_TEST_GUARD("Memory Allocation Test");
        
        // 分配内存
        int* ptr = new int[100];
        
        // 使用内存
        for (int i = 0; i < 100; ++i) {
            ptr[i] = i;
        }
        
        // 释放内存
        delete[] ptr;
        
        // 框架会自动检测内存泄漏
    }
    
    static void testWithTimeout() {
        // 使用安全测试宏，提供超时和内存检测
        SAFE_RUN_TEST(MemoryTestClass, testLongOperation, 3000);
    }
    
    static void testLongOperation() {
        // 执行可能耗时的操作
        // 如果超过3秒会自动终止
    }
};
```

### 2. 自定义输出和格式化

```cpp
// 设置颜色主题
TestUtils::setTheme(ColorTheme::MODERN);
TestUtils::setTheme(ColorTheme::CLASSIC);
TestUtils::setTheme(ColorTheme::MINIMAL);

// 启用/禁用颜色输出
TestUtils::setColorEnabled(true);
TestUtils::setColorEnabled(false);

// 打印不同级别的信息
TestUtils::printInfo("Information message");
TestUtils::printWarning("Warning message");
TestUtils::printError("Error message");
TestUtils::printException("Exception message");

// 打印层次化标题
TestUtils::printLevelHeader(TestLevel::MODULE, "Parser Module");
TestUtils::printLevelFooter(TestLevel::MODULE, "Parser Module Completed");
```

### 3. 测试统计和报告

```cpp
// 获取测试统计信息
auto stats = TestUtils::getTestStatistics();
std::cout << "Total tests: " << stats.totalTests << std::endl;
std::cout << "Passed: " << stats.passedTests << std::endl;
std::cout << "Failed: " << stats.failedTests << std::endl;

// 生成测试报告
TestUtils::generateTestReport();
```

## 最佳实践

### 1. 测试组织

```cpp
// 推荐的测试组织结构
class ParserTestSuite {
public:
    static void runAllTests() {
        // 按功能分组
        RUN_TEST_GROUP("Expression Parsing", runExpressionTests);
        RUN_TEST_GROUP("Statement Parsing", runStatementTests);
        RUN_TEST_GROUP("Error Recovery", runErrorRecoveryTests);
        
        // 内存安全测试单独分组
        RUN_TEST_GROUP("Memory Safety", runMemorySafetyTests);
    }
    
private:
    static void runExpressionTests() {
        RUN_TEST(ExpressionParser, testBinaryExpression);
        RUN_TEST(ExpressionParser, testUnaryExpression);
        RUN_TEST(ExpressionParser, testFunctionCall);
    }
    
    // ... 其他测试组
};
```

### 2. 错误处理

```cpp
class TestClass {
public:
    static void testWithProperErrorHandling() {
        try {
            // 执行可能失败的操作
            performRiskyOperation();
            
            // 验证结果
            if (!verifyResult()) {
                throw std::runtime_error("Result verification failed");
            }
            
            TestUtils::printInfo("Test passed successfully");
            
        } catch (const std::exception& e) {
            TestUtils::printError("Test failed: " + std::string(e.what()));
            throw; // 重新抛出异常让框架处理
        }
    }
};
```

### 3. 内存管理

```cpp
class MemoryAwareTest {
public:
    static void testWithRAII() {
        MEMORY_LEAK_TEST_GUARD("RAII Test");
        
        // 使用RAII管理资源
        {
            std::unique_ptr<int[]> ptr(new int[1000]);
            // 使用ptr
            ptr[0] = 42;
        } // 自动释放
        
        // 或使用自定义RAII类
        {
            ResourceManager manager;
            manager.allocateResources();
            // 使用资源
        } // 析构函数自动清理
    }
};
```

## 配置选项

### 1. 颜色主题

```cpp
// 现代主题（默认）
TestUtils::setTheme(ColorTheme::MODERN);

// 经典主题
TestUtils::setTheme(ColorTheme::CLASSIC);

// 最小主题
TestUtils::setTheme(ColorTheme::MINIMAL);
```

### 2. 输出控制

```cpp
// 详细输出模式
TestUtils::setVerbose(true);

// 静默模式（仅显示错误）
TestUtils::setSilent(true);

// 禁用颜色（适用于CI/CD）
TestUtils::setColorEnabled(false);
```

### 3. 内存检测配置

```cpp
// 设置内存检测超时
MemoryTestUtils::setDefaultTimeout(5000); // 5秒

// 启用/禁用内存检测
MemoryTestUtils::setMemoryCheckEnabled(true);

// 设置内存泄漏阈值
MemoryTestUtils::setLeakThreshold(1024); // 1KB
```

## 迁移指南

### 概述

本指南将帮助您从旧的测试结构迁移到新的模块化测试框架。新框架提供了更好的职责分离、可维护性和可扩展性。

### 主要变化

#### 1. 目录结构变化

**旧结构:**
```
src/tests/
├── test_main.hpp          # 主测试入口
├── test_utils.hpp          # 测试工具
├── formatting/             # 格式化模块
├── check_naming.py         # 命名检查工具
└── [各模块测试]/
```

**新结构:**
```
src/
├── test_framework/         # 独立的测试框架
│   ├── core/              # 核心框架组件
│   ├── formatting/        # 格式化模块
│   └── tools/             # 测试工具
└── tests/                 # 纯测试代码
    ├── test_main_new.hpp  # 新的主测试入口
    └── [各模块测试]/
```

#### 2. 命名空间变化

**旧命名空间:**
```cpp
namespace Lua {
namespace Tests {
    class TestUtils { ... };
}
}
```

**新命名空间:**
```cpp
namespace Lua {
namespace TestFramework {
    class TestUtils { ... };
}
namespace Tests {
    // 具体的测试代码
}
}
```

#### 3. 包含路径变化

**旧包含方式:**
```cpp
#include "test_utils.hpp"
#include "formatting/test_formatter.hpp"
```

**新包含方式:**
```cpp
#include "../test_framework/core/test_utils.hpp"
#include "../test_framework/formatting/test_formatter.hpp"
```

### 迁移步骤

#### 步骤 1: 更新包含路径

在您的测试文件中，将旧的包含路径替换为新的路径：

```cpp
// 旧的
#include "test_utils.hpp"

// 新的
#include "../test_framework/core/test_utils.hpp"
```

#### 步骤 2: 更新命名空间

```cpp
// 旧的
using namespace Lua::Tests;
TestUtils::printInfo("message");

// 新的
using namespace Lua::TestFramework;
TestUtils::printInfo("message");
```

#### 步骤 3: 使用新的测试入口

```cpp
// 旧的
#include "test_main.hpp"
void runTests() {
    Lua::Tests::runAllTests();
}

// 新的
#include "test_main_new.hpp"
void runTests() {
    Lua::Tests::runAllTests(); // 使用新框架的实现
}
```

#### 步骤 4: 更新测试宏使用

测试宏的使用方式保持不变，但现在它们来自新的框架：

```cpp
// 这些宏的使用方式不变
RUN_TEST_MODULE("Parser Module", ParserTestSuite);
RUN_TEST_SUITE(ExprTestSuite);
RUN_TEST_GROUP("Binary Expression Tests", testBinaryExpressions);
RUN_TEST(BinaryExprTest, testAddition);

// 新增的安全测试宏
SAFE_RUN_TEST(TestClass, testMethod, 5000);
```

### 向后兼容性

为了确保平滑迁移，我们提供了向后兼容性支持：

1. **旧的 `test_utils.hpp`** 现在作为包装器，重定向到新的框架
2. **旧的命名空间** 通过别名继续可用
3. **旧的测试宏** 继续工作

### 新功能

新的测试框架提供了以下增强功能：

#### 1. 改进的内存检测

```cpp
// 自动内存泄漏检测
MEMORY_LEAK_TEST_GUARD("TestName");

// 带超时和内存检测的安全测试
SAFE_RUN_TEST(TestClass, testMethod, 5000);
```

#### 2. 测试统计

```cpp
// 获取测试统计信息
auto stats = TestUtils::getStatistics();
std::cout << "Pass rate: " << stats.getPassRate() << "%" << std::endl;

// 打印统计报告
TestUtils::printStatisticsReport();
```

#### 3. 模块化测试运行

```cpp
// 运行特定模块
runModuleTests("parser");

// 运行快速测试
runQuickTests();
```

#### 4. 改进的配置

```cpp
// 配置颜色输出
TestUtils::setColorEnabled(true);

// 配置内存检测
TestUtils::setMemoryCheckEnabled(true);

// 设置主题
TestUtils::setTheme("dark");
```

### 最佳实践

#### 1. 逐步迁移

建议逐个模块进行迁移，而不是一次性迁移所有代码：

1. 先迁移一个小模块（如 lexer）
2. 测试确保功能正常
3. 逐步迁移其他模块

#### 2. 利用新功能

在迁移过程中，考虑使用新框架的增强功能：

- 使用内存检测宏确保测试的内存安全
- 利用统计功能监控测试质量
- 使用模块化运行功能提高开发效率

新框架鼓励将测试逻辑与框架代码分离：

- 测试类应该专注于测试逻辑
- 框架相关的配置应该在测试入口点进行
- 避免在测试代码中直接操作框架内部

## 故障排除

### 常见问题

1. **编译错误**：
   - 确保包含了正确的头文件
   - 检查命名空间是否正确
   - 验证测试类和方法是否为静态
   - 检查包含路径是否正确
   - 确保新的测试框架文件已正确放置

2. **链接错误：未定义的符号**
   - 确保包含了必要的实现文件
   - 检查命名空间是否正确

3. **内存检测误报**：
   - 检查是否有未释放的资源
   - 确保使用了正确的内存管理模式
   - 考虑调整内存泄漏阈值

4. **测试超时**：
   - 增加超时时间
   - 优化测试代码性能
   - 检查是否有死循环

5. **运行时错误：测试失败**
   - 检查测试逻辑是否需要适配新框架
   - 确认内存检测设置是否合适

### 调试技巧

```cpp
// 启用详细输出
TestUtils::setVerbose(true);

// 添加调试信息
TestUtils::printInfo("Debug: Entering test function");
TestUtils::printInfo("Debug: Variable value = " + std::to_string(value));

// 使用内存泄漏检测
MEMORY_LEAK_TEST_GUARD("Debug Test");

// 使用安全测试进行调试
SAFE_RUN_TEST(TestClass, debugTest, 10000);

// 禁用内存检测进行调试
TestUtils::setMemoryCheckEnabled(false);

// 使用模块测试
RUN_TEST_MODULE("Problematic Module", ProblematicTestSuite);
```

## 测试统计功能开发规划

### 当前状态分析

#### 已有功能
- **基础统计结构**: `test_utils.hpp` 中已定义 `TestStatistics` 结构
- **运行器统计**: `test_runner.hpp` 中已定义 `TestStats` 结构
- **基础统计方法**: `getStatistics()`、`resetStatistics()`、`printStatisticsReport()` 等
- **格式化支持**: `format_config.hpp` 中已有 `showStatistics` 配置项

#### 存在的问题
1. **统计数据分散**: `TestStatistics` 和 `TestStats` 结构重复，缺乏统一管理
2. **功能不完整**: 缺少详细的时间统计、内存使用统计、性能分析等
3. **报告功能简陋**: 只有基础的文本输出，缺少丰富的报告格式
4. **实时统计缺失**: 无法实时查看测试进度和统计信息

### 开发阶段规划

#### 第一阶段：统计数据结构重构 (1-2天)

**1.1 创建统一的统计管理器**

新建文件：`src/test_framework/core/test_statistics.hpp`

```cpp
namespace Lua::TestFramework {
    class TestStatisticsManager {
    public:
        struct DetailedStats {
            // 基础统计
            int totalTests = 0;
            int passedTests = 0;
            int failedTests = 0;
            int skippedTests = 0;
            int errorTests = 0;
            
            // 模块统计
            int totalModules = 0;
            int passedModules = 0;
            int failedModules = 0;
            
            // 时间统计
            double totalExecutionTime = 0.0;
            double averageTestTime = 0.0;
            double longestTestTime = 0.0;
            double shortestTestTime = 0.0;
            std::string slowestTestName;
            std::string fastestTestName;
            
            // 内存统计
            size_t totalMemoryUsed = 0;
            size_t peakMemoryUsage = 0;
            size_t memoryLeaks = 0;
            
            // 性能统计
            std::map<std::string, double> moduleExecutionTimes;
            std::map<std::string, int> moduleTestCounts;
            
            // 计算方法
            double getPassRate() const;
            double getFailureRate() const;
            bool allPassed() const;
            std::string getExecutionSummary() const;
        };
        
        // 单例模式
        static TestStatisticsManager& getInstance();
        
        // 统计记录方法
        void recordTestStart(const std::string& testName, const std::string& module = "");
        void recordTestEnd(const std::string& testName, bool passed, double executionTime = 0.0);
        void recordTestSkipped(const std::string& testName, const std::string& reason = "");
        void recordTestError(const std::string& testName, const std::string& error);
        
        // 模块统计
        void recordModuleStart(const std::string& moduleName);
        void recordModuleEnd(const std::string& moduleName, bool passed);
        
        // 内存统计
        void recordMemoryUsage(size_t currentUsage);
        void recordMemoryLeak(size_t leakSize);
        
        // 获取统计信息
        const DetailedStats& getDetailedStats() const;
        DetailedStats getModuleStats(const std::string& moduleName) const;
        
        // 重置和管理
        void reset();
        void resetModule(const std::string& moduleName);
        
        // 实时统计
        void enableRealTimeUpdates(bool enabled);
        void setUpdateInterval(int milliseconds);
    };
}
```

**1.2 集成现有统计结构**
- 重构 `TestStats` 使其使用新的统计管理器
- 更新 `TestStatistics` 为新统计管理器的别名

#### 第二阶段：报告生成系统 (2-3天)

**2.1 创建报告生成器**

新建文件：`src/test_framework/reporting/test_reporter.hpp`

```cpp
namespace Lua::TestFramework::Reporting {
    enum class ReportFormat {
        CONSOLE,
        HTML,
        JSON,
        XML,
        CSV,
        MARKDOWN
    };
    
    class TestReporter {
    public:
        // 报告生成
        void generateReport(ReportFormat format, const std::string& outputPath = "");
        void generateSummaryReport();
        void generateDetailedReport();
        void generateModuleReport(const std::string& moduleName);
        
        // 实时报告
        void enableLiveReporting(bool enabled);
        void updateLiveReport();
        
        // 自定义报告
        void addCustomSection(const std::string& title, const std::string& content);
        void setReportTemplate(const std::string& templatePath);
        
        // 报告配置
        void setIncludeTimings(bool include);
        void setIncludeMemoryStats(bool include);
        void setIncludeErrorDetails(bool include);
        void setIncludeGraphs(bool include);
    };
}
```

**2.2 报告模板系统**
- HTML模板：包含图表、进度条、详细统计
- JSON模板：便于CI/CD集成
- Markdown模板：便于文档集成

#### 第三阶段：实时统计显示 (1-2天)

**3.1 实时进度显示**

扩展格式化器功能：

```cpp
namespace Lua::Tests::TestFormatting {
    class ProgressDisplay {
    public:
        void showProgressBar(int current, int total);
        void showRealTimeStats();
        void showModuleProgress(const std::string& moduleName);
        void updateDisplay();
        
        // 配置选项
        void setProgressBarStyle(const std::string& style);
        void setUpdateFrequency(int milliseconds);
        void enableAnimations(bool enabled);
    };
}
```

**3.2 控制台增强**
- 彩色进度条
- 实时统计更新
- 模块执行状态显示
- 内存使用监控

#### 第四阶段：性能分析功能 (2-3天)

**4.1 性能分析器**

新建文件：`src/test_framework/profiling/test_profiler.hpp`

```cpp
namespace Lua::TestFramework::Profiling {
    class TestProfiler {
    public:
        struct PerformanceMetrics {
            double cpuUsage;
            size_t memoryUsage;
            double executionTime;
            int functionCalls;
            std::map<std::string, double> hotspots;
        };
        
        void startProfiling(const std::string& testName);
        void stopProfiling(const std::string& testName);
        PerformanceMetrics getMetrics(const std::string& testName);
        
        // 性能比较
        void compareWithBaseline(const std::string& baselinePath);
        void saveBaseline(const std::string& outputPath);
        
        // 性能警告
        void setPerformanceThresholds(double maxTime, size_t maxMemory);
        std::vector<std::string> getPerformanceWarnings();
    };
}
```

**4.2 基准测试支持**
- 性能基准保存和比较
- 回归检测
- 性能趋势分析

#### 第五阶段：高级统计功能 (2-3天)

**5.1 统计分析**

扩展统计管理器：

```cpp
class TestStatisticsManager {
public:
    // 高级分析
    struct TrendAnalysis {
        std::vector<double> passRateHistory;
        std::vector<double> executionTimeHistory;
        double passRateTrend;  // 正数表示改善，负数表示恶化
        double performanceTrend;
    };
    
    TrendAnalysis analyzeTrends(int historyDays = 30);
    std::map<std::string, double> getModuleReliability();
    std::vector<std::string> getFlakeyTests(double threshold = 0.1);
    
    // 预测分析
    double predictTestDuration(const std::string& testName);
    int estimateRemainingTime();
};
```

**5.2 数据持久化**
- 历史数据存储（SQLite或JSON文件）
- 趋势分析
- 测试稳定性评估

#### 第六阶段：集成和优化 (1-2天)

**6.1 框架集成**
- 更新 `test_runner.hpp` 集成新统计功能
- 更新 `test_macros.hpp` 添加统计宏
- 更新配置系统支持统计选项

**6.2 向后兼容**
- 保持现有API兼容性
- 提供迁移指南
- 添加兼容性测试

### 技术实现细节

#### 数据结构设计

```cpp
// 核心数据结构
struct TestExecutionRecord {
    std::string testName;
    std::string moduleName;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    TestResult result;
    std::string errorMessage;
    size_t memoryUsage;
    std::map<std::string, std::any> customMetrics;
};
```

#### 性能考虑
- 使用轻量级计时器避免影响测试性能
- 异步统计更新减少阻塞
- 内存池管理避免频繁分配
- 可配置的统计级别（基础/详细/完整）

#### 配置扩展

```cpp
// 扩展现有配置系统
struct StatisticsConfig {
    bool enableDetailedStats = true;
    bool enableRealTimeDisplay = true;
    bool enablePerformanceProfiling = false;
    bool enableMemoryTracking = true;
    bool enableTrendAnalysis = false;
    int historyRetentionDays = 30;
    std::string reportOutputPath = "./test_reports";
    ReportFormat defaultReportFormat = ReportFormat::HTML;
};
```

### 预期成果

1. **完整的统计系统**: 涵盖测试执行、性能、内存等各个方面
2. **丰富的报告格式**: 支持多种输出格式，满足不同需求
3. **实时监控能力**: 提供测试执行过程的实时反馈
4. **性能分析工具**: 帮助识别性能瓶颈和回归
5. **趋势分析功能**: 支持长期的测试质量监控
6. **易于集成**: 与现有框架无缝集成，保持向后兼容

### 目录结构更新

实施后的框架结构将扩展为：

```
test_framework/
├── README.md                 # 框架说明文档
├── core/                     # 核心框架代码
│   ├── test_runner.hpp       # 测试运行器
│   ├── test_macros.hpp       # 测试宏定义
│   ├── test_utils.hpp        # 测试工具类
│   ├── test_memory.hpp       # 内存测试工具
│   └── test_statistics.hpp   # 统计管理器 (新增)
├── reporting/                # 报告生成模块 (新增)
│   ├── test_reporter.hpp     # 报告生成器
│   ├── test_reporter.cpp     # 报告生成器实现
│   └── templates/            # 报告模板
│       ├── html_template.html
│       ├── json_template.json
│       └── markdown_template.md
├── profiling/                # 性能分析模块 (新增)
│   ├── test_profiler.hpp     # 性能分析器
│   └── test_profiler.cpp     # 性能分析器实现
├── formatting/               # 格式化模块
│   ├── format_formatter.hpp  # 测试格式化器
│   ├── format_formatter.cpp  # 格式化器实现
│   ├── format_config.hpp     # 配置管理
│   ├── format_config.cpp     # 配置实现
│   ├── format_colors.hpp     # 颜色定义
│   ├── format_colors.cpp     # 颜色实现
│   ├── format_strategies.hpp # 格式化策略
│   └── progress_display.hpp  # 进度显示 (新增)
└── tools/                    # 测试工具
    ├── check_naming.py       # 命名规范检查
    ├── check_naming_simple.py # 简化版命名检查
    └── check_naming.bat      # 批处理脚本
```

这个规划将显著提升测试框架的可观测性和分析能力，为项目的质量保证提供强有力的支持。

## 示例项目

查看 `examples/example_test.hpp` 文件获取完整的使用示例。

## 版本信息

当前版本：2.0.0

要查看版本信息：
```cpp
Lua::TestFramework::Version::printVersion();
```

## 支持和贡献

如果遇到问题或有改进建议，请：
1. 查看本文档和迁移指南
2. 检查示例代码
3. 提交issue或pull request

如果在迁移过程中遇到问题，请：

1. 查看本指南的故障排除部分
2. 检查新框架的文档和示例
3. 在项目中创建 issue 报告问题

## 未来计划

- 在所有模块成功迁移后，旧的兼容性代码将被标记为废弃
- 计划在下一个主要版本中移除向后兼容性支持
- 持续改进测试框架的功能和性能

---

*本文档会随着框架的更新而持续改进。*
