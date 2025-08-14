#pragma once

#include "../../gc/core/garbage_collector.hpp"
#include "../../vm/lua_state.hpp"
#include "../../vm/global_state.hpp"
#include "../../common/types.hpp"
#include <chrono>
#include <vector>
#include <string>
#include <memory>

namespace Lua {
namespace Performance {

    /**
     * @brief GC性能基准测试框架
     * 
     * 测量增量GC的性能特征，包括：
     * - GC暂停时间
     * - 内存使用效率
     * - 吞吐量对比
     * - 增量步进效果
     */
    class GCBenchmark {
    public:
        /**
         * @brief 性能测试结果
         */
        struct BenchmarkResult {
            // 时间测量
            double totalTimeMs = 0.0;          // 总执行时间(毫秒)
            double gcTimeMs = 0.0;              // GC总时间(毫秒)
            double maxPauseMs = 0.0;            // 最大暂停时间(毫秒)
            double avgPauseMs = 0.0;            // 平均暂停时间(毫秒)
            double minPauseMs = 0.0;            // 最小暂停时间(毫秒)
            
            // 内存测量
            usize maxMemoryUsage = 0;           // 最大内存使用(字节)
            usize avgMemoryUsage = 0;           // 平均内存使用(字节)
            usize totalAllocated = 0;           // 总分配内存(字节)
            usize totalFreed = 0;               // 总释放内存(字节)
            
            // GC统计
            u32 gcCycles = 0;                   // GC周期数
            u32 incrementalSteps = 0;           // 增量步骤数
            u32 fullGCCount = 0;                // 完整GC次数
            
            // 吞吐量
            double allocationsPerSecond = 0.0;  // 每秒分配次数
            double objectsPerSecond = 0.0;      // 每秒创建对象数
            
            // 效率指标
            double gcOverhead = 0.0;            // GC开销百分比
            double memoryEfficiency = 0.0;     // 内存效率(有效内存/总分配)
        };
        
        /**
         * @brief 测试配置
         */
        struct TestConfig {
            u32 objectCount = 10000;            // 创建对象数量
            u32 iterations = 100;               // 测试迭代次数
            bool useIncrementalGC = true;       // 是否使用增量GC
            bool enableProfiling = true;        // 是否启用性能分析
            u32 gcStepSize = 1024;              // GC步长大小
            i32 gcPause = 200;                  // GC暂停参数
            i32 gcStepMul = 200;                // GC步长倍数
        };
        
    private:
        std::unique_ptr<GlobalState> globalState_;
        LuaState* luaState_;
        std::vector<double> pauseTimes_;
        std::vector<usize> memorySnapshots_;
        std::chrono::high_resolution_clock::time_point startTime_;
        std::chrono::high_resolution_clock::time_point endTime_;
        
    public:
        GCBenchmark();
        ~GCBenchmark();
        
        /**
         * @brief 运行所有基准测试
         */
        void runAllBenchmarks();
        
        /**
         * @brief 测试增量GC性能
         * @param config 测试配置
         * @return 测试结果
         */
        BenchmarkResult testIncrementalGC(const TestConfig& config);
        
        /**
         * @brief 测试完整GC性能
         * @param config 测试配置
         * @return 测试结果
         */
        BenchmarkResult testFullGC(const TestConfig& config);
        
        /**
         * @brief 对比增量GC vs 完整GC
         * @param config 测试配置
         */
        void compareGCModes(const TestConfig& config);
        
        /**
         * @brief 测试内存分配性能
         * @param config 测试配置
         * @return 测试结果
         */
        BenchmarkResult testAllocationPerformance(const TestConfig& config);
        
        /**
         * @brief 测试字符串创建性能
         * @param config 测试配置
         * @return 测试结果
         */
        BenchmarkResult testStringCreationPerformance(const TestConfig& config);
        
        /**
         * @brief 测试表操作性能
         * @param config 测试配置
         * @return 测试结果
         */
        BenchmarkResult testTableOperationPerformance(const TestConfig& config);
        
        /**
         * @brief 压力测试 - 大量对象创建和回收
         * @param config 测试配置
         * @return 测试结果
         */
        BenchmarkResult stressTest(const TestConfig& config);
        
        /**
         * @brief 打印测试结果
         * @param result 测试结果
         * @param testName 测试名称
         */
        void printResult(const BenchmarkResult& result, const std::string& testName);
        
        /**
         * @brief 保存测试结果到文件
         * @param result 测试结果
         * @param filename 文件名
         */
        void saveResultToFile(const BenchmarkResult& result, const std::string& filename);
        
    private:
        /**
         * @brief 初始化测试环境
         * @param config 测试配置
         */
        void setupTest(const TestConfig& config);
        
        /**
         * @brief 清理测试环境
         */
        void cleanupTest();
        
        /**
         * @brief 开始性能测量
         */
        void startMeasurement();
        
        /**
         * @brief 结束性能测量
         * @return 经过的时间(毫秒)
         */
        double endMeasurement();
        
        /**
         * @brief 记录GC暂停时间
         * @param pauseMs 暂停时间(毫秒)
         */
        void recordPause(double pauseMs);
        
        /**
         * @brief 记录内存快照
         */
        void recordMemorySnapshot();
        
        /**
         * @brief 创建测试对象
         * @param count 对象数量
         */
        void createTestObjects(u32 count);
        
        /**
         * @brief 创建测试字符串
         * @param count 字符串数量
         */
        void createTestStrings(u32 count);
        
        /**
         * @brief 创建测试表
         * @param count 表数量
         */
        void createTestTables(u32 count);
        
        /**
         * @brief 计算统计信息
         * @param values 数值向量
         * @return {min, max, avg}
         */
        std::tuple<double, double, double> calculateStats(const std::vector<double>& values);
        
        /**
         * @brief 获取当前内存使用量
         * @return 内存使用量(字节)
         */
        usize getCurrentMemoryUsage();
        
        /**
         * @brief 触发GC并测量时间
         * @param incremental 是否使用增量GC
         * @return GC时间(毫秒)
         */
        double triggerGCAndMeasure(bool incremental = true);
    };

} // namespace Performance
} // namespace Lua
