# Lua 测试框架

## 概述

这是一个专门为 Lua 解释器项目设计的测试框架，提供了完整的测试基础设施和工具。

## 框架结构

```
test_framework/
├── README.md                 # 框架说明文档
├── core/                     # 核心框架代码
│   ├── test_runner.hpp       # 测试运行器
│   ├── test_macros.hpp       # 测试宏定义
│   ├── test_utils.hpp        # 测试工具类
│   └── test_memory.hpp       # 内存测试工具
├── formatting/               # 格式化模块
│   ├── test_formatter.hpp    # 测试格式化器
│   ├── test_formatter.cpp    # 格式化器实现
│   ├── test_config.hpp       # 配置管理
│   ├── test_config.cpp       # 配置实现
│   ├── test_colors.hpp       # 颜色定义
│   ├── test_colors.cpp       # 颜色实现
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

## 使用方法

### 基本用法

```cpp
#include "test_framework/core/test_runner.hpp"

int main() {
    Lua::TestFramework::TestRunner runner;
    runner.runAllTests();
    return 0;
}
```

### 创建测试

```cpp
#include "test_framework/core/test_macros.hpp"

class MyTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Tests", testBasicFunctionality);
    }
    
private:
    static void testBasicFunctionality() {
        RUN_TEST(MyTestClass, testMethod1);
        RUN_TEST(MyTestClass, testMethod2);
    }
};
```

## 迁移指南

从旧的测试结构迁移到新框架：

1. 将框架相关代码移动到 `test_framework/` 目录
2. 将具体测试代码保留在 `tests/` 目录
3. 更新包含路径，使用新的框架头文件
4. 使用新的命名空间 `Lua::TestFramework`

## 配置选项

框架支持多种配置选项：

- 颜色输出控制
- 格式化主题选择
- 内存检测开关
- 详细程度控制

详细配置说明请参考各模块的文档。

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