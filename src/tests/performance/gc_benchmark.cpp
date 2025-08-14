#include "gc_benchmark.hpp"
#include "../../api/lua51_gc_api.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace Lua {
namespace Performance {

    GCBenchmark::GCBenchmark() : luaState_(nullptr) {
        // 初始化全局状态
        globalState_ = std::make_unique<GlobalState>();
        luaState_ = globalState_->newThread();
    }
    
    GCBenchmark::~GCBenchmark() {
        cleanupTest();
    }
    
    void GCBenchmark::runAllBenchmarks() {
        std::cout << "=== Lua 5.1 GC Performance Benchmark Suite ===" << std::endl;
        std::cout << "Testing incremental GC implementation..." << std::endl << std::endl;
        
        TestConfig defaultConfig;
        
        // 1. 增量GC性能测试
        std::cout << "1. Incremental GC Performance Test" << std::endl;
        auto incrementalResult = testIncrementalGC(defaultConfig);
        printResult(incrementalResult, "Incremental GC");
        std::cout << std::endl;
        
        // 2. 完整GC性能测试
        std::cout << "2. Full GC Performance Test" << std::endl;
        auto fullGCResult = testFullGC(defaultConfig);
        printResult(fullGCResult, "Full GC");
        std::cout << std::endl;
        
        // 3. GC模式对比
        std::cout << "3. GC Mode Comparison" << std::endl;
        compareGCModes(defaultConfig);
        std::cout << std::endl;
        
        // 4. 内存分配性能
        std::cout << "4. Memory Allocation Performance" << std::endl;
        auto allocResult = testAllocationPerformance(defaultConfig);
        printResult(allocResult, "Memory Allocation");
        std::cout << std::endl;
        
        // 5. 字符串创建性能
        std::cout << "5. String Creation Performance" << std::endl;
        auto stringResult = testStringCreationPerformance(defaultConfig);
        printResult(stringResult, "String Creation");
        std::cout << std::endl;
        
        // 6. 表操作性能
        std::cout << "6. Table Operation Performance" << std::endl;
        auto tableResult = testTableOperationPerformance(defaultConfig);
        printResult(tableResult, "Table Operations");
        std::cout << std::endl;
        
        // 7. 压力测试
        std::cout << "7. Stress Test" << std::endl;
        TestConfig stressConfig = defaultConfig;
        stressConfig.objectCount = 50000;
        stressConfig.iterations = 50;
        auto stressResult = stressTest(stressConfig);
        printResult(stressResult, "Stress Test");
        std::cout << std::endl;
        
        std::cout << "=== Benchmark Suite Completed ===" << std::endl;
    }
    
    GCBenchmark::BenchmarkResult GCBenchmark::testIncrementalGC(const TestConfig& config) {
        setupTest(config);
        BenchmarkResult result;
        
        startMeasurement();
        
        for (u32 i = 0; i < config.iterations; ++i) {
            // 创建对象
            createTestObjects(config.objectCount / config.iterations);
            
            // 触发增量GC
            auto gcStart = std::chrono::high_resolution_clock::now();
            luaC_step(luaState_);
            auto gcEnd = std::chrono::high_resolution_clock::now();
            
            double pauseTime = std::chrono::duration<double, std::milli>(gcEnd - gcStart).count();
            recordPause(pauseTime);
            recordMemorySnapshot();
            
            result.incrementalSteps++;
        }
        
        result.totalTimeMs = endMeasurement();
        
        // 计算统计信息
        if (!pauseTimes_.empty()) {
            auto [minPause, maxPause, avgPause] = calculateStats(pauseTimes_);
            result.minPauseMs = minPause;
            result.maxPauseMs = maxPause;
            result.avgPauseMs = avgPause;
            result.gcTimeMs = std::accumulate(pauseTimes_.begin(), pauseTimes_.end(), 0.0);
        }
        
        if (!memorySnapshots_.empty()) {
            result.maxMemoryUsage = *std::max_element(memorySnapshots_.begin(), memorySnapshots_.end());
            result.avgMemoryUsage = std::accumulate(memorySnapshots_.begin(), memorySnapshots_.end(), 0ULL) / memorySnapshots_.size();
        }
        
        result.gcOverhead = (result.gcTimeMs / result.totalTimeMs) * 100.0;
        result.allocationsPerSecond = (config.objectCount * config.iterations) / (result.totalTimeMs / 1000.0);
        
        cleanupTest();
        return result;
    }
    
    GCBenchmark::BenchmarkResult GCBenchmark::testFullGC(const TestConfig& config) {
        setupTest(config);
        BenchmarkResult result;
        
        startMeasurement();
        
        for (u32 i = 0; i < config.iterations; ++i) {
            // 创建对象
            createTestObjects(config.objectCount / config.iterations);
            
            // 触发完整GC
            auto gcStart = std::chrono::high_resolution_clock::now();
            luaC_fullgc(luaState_);
            auto gcEnd = std::chrono::high_resolution_clock::now();
            
            double pauseTime = std::chrono::duration<double, std::milli>(gcEnd - gcStart).count();
            recordPause(pauseTime);
            recordMemorySnapshot();
            
            result.fullGCCount++;
        }
        
        result.totalTimeMs = endMeasurement();
        
        // 计算统计信息
        if (!pauseTimes_.empty()) {
            auto [minPause, maxPause, avgPause] = calculateStats(pauseTimes_);
            result.minPauseMs = minPause;
            result.maxPauseMs = maxPause;
            result.avgPauseMs = avgPause;
            result.gcTimeMs = std::accumulate(pauseTimes_.begin(), pauseTimes_.end(), 0.0);
        }
        
        if (!memorySnapshots_.empty()) {
            result.maxMemoryUsage = *std::max_element(memorySnapshots_.begin(), memorySnapshots_.end());
            result.avgMemoryUsage = std::accumulate(memorySnapshots_.begin(), memorySnapshots_.end(), 0ULL) / memorySnapshots_.size();
        }
        
        result.gcOverhead = (result.gcTimeMs / result.totalTimeMs) * 100.0;
        result.allocationsPerSecond = (config.objectCount * config.iterations) / (result.totalTimeMs / 1000.0);
        
        cleanupTest();
        return result;
    }
    
    void GCBenchmark::compareGCModes(const TestConfig& config) {
        std::cout << "Comparing Incremental GC vs Full GC:" << std::endl;
        
        auto incrementalResult = testIncrementalGC(config);
        auto fullGCResult = testFullGC(config);
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Metric                    | Incremental GC | Full GC       | Improvement" << std::endl;
        std::cout << "--------------------------|----------------|---------------|------------" << std::endl;
        
        // 最大暂停时间对比
        double pauseImprovement = ((fullGCResult.maxPauseMs - incrementalResult.maxPauseMs) / fullGCResult.maxPauseMs) * 100.0;
        std::cout << "Max Pause Time (ms)       | " << std::setw(14) << incrementalResult.maxPauseMs 
                  << " | " << std::setw(13) << fullGCResult.maxPauseMs 
                  << " | " << std::setw(10) << pauseImprovement << "%" << std::endl;
        
        // 平均暂停时间对比
        double avgPauseImprovement = ((fullGCResult.avgPauseMs - incrementalResult.avgPauseMs) / fullGCResult.avgPauseMs) * 100.0;
        std::cout << "Avg Pause Time (ms)       | " << std::setw(14) << incrementalResult.avgPauseMs 
                  << " | " << std::setw(13) << fullGCResult.avgPauseMs 
                  << " | " << std::setw(10) << avgPauseImprovement << "%" << std::endl;
        
        // 总GC时间对比
        double gcTimeImprovement = ((fullGCResult.gcTimeMs - incrementalResult.gcTimeMs) / fullGCResult.gcTimeMs) * 100.0;
        std::cout << "Total GC Time (ms)        | " << std::setw(14) << incrementalResult.gcTimeMs 
                  << " | " << std::setw(13) << fullGCResult.gcTimeMs 
                  << " | " << std::setw(10) << gcTimeImprovement << "%" << std::endl;
        
        // 吞吐量对比
        double throughputImprovement = ((incrementalResult.allocationsPerSecond - fullGCResult.allocationsPerSecond) / fullGCResult.allocationsPerSecond) * 100.0;
        std::cout << "Allocations/sec           | " << std::setw(14) << incrementalResult.allocationsPerSecond 
                  << " | " << std::setw(13) << fullGCResult.allocationsPerSecond 
                  << " | " << std::setw(10) << throughputImprovement << "%" << std::endl;
    }
    
    GCBenchmark::BenchmarkResult GCBenchmark::testAllocationPerformance(const TestConfig& config) {
        setupTest(config);
        BenchmarkResult result;
        
        startMeasurement();
        
        // 测试纯分配性能
        for (u32 i = 0; i < config.objectCount; ++i) {
            // 创建各种类型的对象
            Value intVal(static_cast<double>(i));
            Value boolVal(i % 2 == 0);
            Value nilVal;
            
            recordMemorySnapshot();
        }
        
        result.totalTimeMs = endMeasurement();
        result.allocationsPerSecond = config.objectCount / (result.totalTimeMs / 1000.0);
        
        if (!memorySnapshots_.empty()) {
            result.maxMemoryUsage = *std::max_element(memorySnapshots_.begin(), memorySnapshots_.end());
            result.avgMemoryUsage = std::accumulate(memorySnapshots_.begin(), memorySnapshots_.end(), 0ULL) / memorySnapshots_.size();
        }
        
        cleanupTest();
        return result;
    }
    
    void GCBenchmark::printResult(const BenchmarkResult& result, const std::string& testName) {
        std::cout << "--- " << testName << " Results ---" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total Time:           " << result.totalTimeMs << " ms" << std::endl;
        std::cout << "GC Time:              " << result.gcTimeMs << " ms" << std::endl;
        std::cout << "Max Pause:            " << result.maxPauseMs << " ms" << std::endl;
        std::cout << "Avg Pause:            " << result.avgPauseMs << " ms" << std::endl;
        std::cout << "Min Pause:            " << result.minPauseMs << " ms" << std::endl;
        std::cout << "Max Memory:           " << (result.maxMemoryUsage / 1024.0) << " KB" << std::endl;
        std::cout << "Avg Memory:           " << (result.avgMemoryUsage / 1024.0) << " KB" << std::endl;
        std::cout << "GC Overhead:          " << result.gcOverhead << "%" << std::endl;
        std::cout << "Allocations/sec:      " << result.allocationsPerSecond << std::endl;
        std::cout << "Incremental Steps:    " << result.incrementalSteps << std::endl;
        std::cout << "Full GC Count:        " << result.fullGCCount << std::endl;
    }

    // === 私有方法实现 ===

    void GCBenchmark::setupTest(const TestConfig& config) {
        pauseTimes_.clear();
        memorySnapshots_.clear();

        // 配置GC参数
        if (luaState_) {
            luaC_setpause(luaState_, config.gcPause);
            luaC_setstepmul(luaState_, config.gcStepMul);
        }
    }

    void GCBenchmark::cleanupTest() {
        if (luaState_) {
            // 执行完整GC清理
            luaC_fullgc(luaState_);
        }
        pauseTimes_.clear();
        memorySnapshots_.clear();
    }

    void GCBenchmark::startMeasurement() {
        startTime_ = std::chrono::high_resolution_clock::now();
    }

    double GCBenchmark::endMeasurement() {
        endTime_ = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(endTime_ - startTime_).count();
    }

    void GCBenchmark::recordPause(double pauseMs) {
        pauseTimes_.push_back(pauseMs);
    }

    void GCBenchmark::recordMemorySnapshot() {
        usize currentUsage = getCurrentMemoryUsage();
        memorySnapshots_.push_back(currentUsage);
    }

    void GCBenchmark::createTestObjects(u32 count) {
        for (u32 i = 0; i < count; ++i) {
            // 创建不同类型的对象来模拟真实使用场景
            if (i % 4 == 0) {
                Value stringVal("test_string_" + std::to_string(i));
                luaState_->push(stringVal);
            } else if (i % 4 == 1) {
                auto table = make_gc_table();
                table->set(Value("key"), Value(static_cast<double>(i)));
                Value tableVal(table);
                luaState_->push(tableVal);
            } else if (i % 4 == 2) {
                Value numberVal(static_cast<double>(i));
                luaState_->push(numberVal);
            } else {
                Value boolVal(i % 2 == 0);
                luaState_->push(boolVal);
            }
        }
    }

    void GCBenchmark::createTestStrings(u32 count) {
        for (u32 i = 0; i < count; ++i) {
            std::string str = "benchmark_string_" + std::to_string(i) + "_with_some_content";
            Value stringVal(str);
            luaState_->push(stringVal);
        }
    }

    void GCBenchmark::createTestTables(u32 count) {
        for (u32 i = 0; i < count; ++i) {
            auto table = make_gc_table();

            // 添加一些键值对
            for (u32 j = 0; j < 10; ++j) {
                table->set(Value(static_cast<double>(j)), Value("value_" + std::to_string(j)));
            }

            Value tableVal(table);
            luaState_->push(tableVal);
        }
    }

    std::tuple<double, double, double> GCBenchmark::calculateStats(const std::vector<double>& values) {
        if (values.empty()) {
            return {0.0, 0.0, 0.0};
        }

        double minVal = *std::min_element(values.begin(), values.end());
        double maxVal = *std::max_element(values.begin(), values.end());
        double avgVal = std::accumulate(values.begin(), values.end(), 0.0) / values.size();

        return {minVal, maxVal, avgVal};
    }

    usize GCBenchmark::getCurrentMemoryUsage() {
        if (globalState_) {
            return globalState_->getTotalBytes();
        }
        return 0;
    }

    double GCBenchmark::triggerGCAndMeasure(bool incremental) {
        auto start = std::chrono::high_resolution_clock::now();

        if (incremental) {
            luaC_step(luaState_);
        } else {
            luaC_fullgc(luaState_);
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    // === 剩余测试方法实现 ===

    GCBenchmark::BenchmarkResult GCBenchmark::testStringCreationPerformance(const TestConfig& config) {
        setupTest(config);
        BenchmarkResult result;

        startMeasurement();
        createTestStrings(config.objectCount);
        result.totalTimeMs = endMeasurement();

        result.allocationsPerSecond = config.objectCount / (result.totalTimeMs / 1000.0);
        result.maxMemoryUsage = getCurrentMemoryUsage();

        cleanupTest();
        return result;
    }

    GCBenchmark::BenchmarkResult GCBenchmark::testTableOperationPerformance(const TestConfig& config) {
        setupTest(config);
        BenchmarkResult result;

        startMeasurement();
        createTestTables(config.objectCount);
        result.totalTimeMs = endMeasurement();

        result.allocationsPerSecond = config.objectCount / (result.totalTimeMs / 1000.0);
        result.maxMemoryUsage = getCurrentMemoryUsage();

        cleanupTest();
        return result;
    }

    GCBenchmark::BenchmarkResult GCBenchmark::stressTest(const TestConfig& config) {
        setupTest(config);
        BenchmarkResult result;

        startMeasurement();

        for (u32 i = 0; i < config.iterations; ++i) {
            // 创建大量对象
            createTestObjects(config.objectCount / config.iterations);

            // 定期触发GC
            if (i % 10 == 0) {
                double pauseTime = triggerGCAndMeasure(config.useIncrementalGC);
                recordPause(pauseTime);
                recordMemorySnapshot();

                if (config.useIncrementalGC) {
                    result.incrementalSteps++;
                } else {
                    result.fullGCCount++;
                }
            }
        }

        result.totalTimeMs = endMeasurement();

        // 计算统计信息
        if (!pauseTimes_.empty()) {
            auto [minPause, maxPause, avgPause] = calculateStats(pauseTimes_);
            result.minPauseMs = minPause;
            result.maxPauseMs = maxPause;
            result.avgPauseMs = avgPause;
            result.gcTimeMs = std::accumulate(pauseTimes_.begin(), pauseTimes_.end(), 0.0);
        }

        if (!memorySnapshots_.empty()) {
            result.maxMemoryUsage = *std::max_element(memorySnapshots_.begin(), memorySnapshots_.end());
            result.avgMemoryUsage = std::accumulate(memorySnapshots_.begin(), memorySnapshots_.end(), 0ULL) / memorySnapshots_.size();
        }

        result.gcOverhead = (result.gcTimeMs / result.totalTimeMs) * 100.0;
        result.allocationsPerSecond = (config.objectCount * config.iterations) / (result.totalTimeMs / 1000.0);

        cleanupTest();
        return result;
    }

    void GCBenchmark::saveResultToFile(const BenchmarkResult& result, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        file << "GC Benchmark Results\n";
        file << "===================\n";
        file << "Total Time (ms): " << result.totalTimeMs << "\n";
        file << "GC Time (ms): " << result.gcTimeMs << "\n";
        file << "Max Pause (ms): " << result.maxPauseMs << "\n";
        file << "Avg Pause (ms): " << result.avgPauseMs << "\n";
        file << "Min Pause (ms): " << result.minPauseMs << "\n";
        file << "Max Memory (bytes): " << result.maxMemoryUsage << "\n";
        file << "Avg Memory (bytes): " << result.avgMemoryUsage << "\n";
        file << "GC Overhead (%): " << result.gcOverhead << "\n";
        file << "Allocations/sec: " << result.allocationsPerSecond << "\n";
        file << "Incremental Steps: " << result.incrementalSteps << "\n";
        file << "Full GC Count: " << result.fullGCCount << "\n";

        file.close();
        std::cout << "Results saved to: " << filename << std::endl;
    }

} // namespace Performance
} // namespace Lua
