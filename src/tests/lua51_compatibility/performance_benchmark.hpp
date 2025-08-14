#pragma once

#include "../../common/types.hpp"
#include "../../vm/lua_state.hpp"
#include <chrono>
#include <string>
#include <vector>

namespace Lua {
namespace Testing {

    /**
     * @brief Performance Benchmark Suite for Lua 5.1 Interpreter
     * 
     * Comprehensive performance testing framework to measure and validate
     * the performance characteristics of our Lua 5.1 implementation against
     * baseline requirements and official Lua performance.
     */
    
    // Benchmark result structure
    struct BenchmarkResult {
        std::string benchmarkName;
        double executionTime;      // in milliseconds
        double operationsPerSecond; // operations/sec
        double memoryUsage;        // in MB
        bool passed;               // whether benchmark passed threshold
        std::string notes;
        
        BenchmarkResult(const std::string& name)
            : benchmarkName(name), executionTime(0.0), operationsPerSecond(0.0),
              memoryUsage(0.0), passed(false) {}
    };
    
    // Performance thresholds
    struct PerformanceThresholds {
        double vmExecutionOpsPerSec = 1000000.0;    // 1M ops/sec minimum
        double memoryAllocOpsPerSec = 100000.0;     // 100K allocs/sec minimum
        double tableOpsPerSec = 500000.0;           // 500K table ops/sec minimum
        double functionCallsPerSec = 100000.0;      // 100K calls/sec minimum
        double maxMemoryUsageMB = 100.0;            // 100MB maximum
        double maxGCPauseMs = 10.0;                 // 10ms maximum GC pause
    };
    
    /**
     * @brief Main Performance Benchmark Manager
     */
    class PerformanceBenchmarkSuite {
    public:
        PerformanceBenchmarkSuite();
        ~PerformanceBenchmarkSuite();
        
        // Benchmark execution
        /**
         * @brief Run all performance benchmarks
         * @return bool True if all benchmarks pass thresholds
         */
        bool runAllBenchmarks();
        
        /**
         * @brief Run specific benchmark by name
         * @param benchmarkName Name of benchmark to run
         * @return BenchmarkResult Benchmark execution result
         */
        BenchmarkResult runBenchmark(const std::string& benchmarkName);
        
        // Configuration
        /**
         * @brief Set performance thresholds
         * @param thresholds Performance threshold configuration
         */
        void setThresholds(const PerformanceThresholds& thresholds);
        
        /**
         * @brief Get current performance thresholds
         * @return PerformanceThresholds Current thresholds
         */
        const PerformanceThresholds& getThresholds() const { return thresholds_; }
        
        // Results and reporting
        /**
         * @brief Get all benchmark results
         * @return std::vector<BenchmarkResult> All benchmark results
         */
        const std::vector<BenchmarkResult>& getResults() const { return results_; }
        
        /**
         * @brief Generate performance report
         * @return std::string Formatted performance report
         */
        std::string generatePerformanceReport() const;
        
        /**
         * @brief Get performance statistics
         * @param totalBenchmarks Output: total number of benchmarks
         * @param passedBenchmarks Output: number of passed benchmarks
         * @param failedBenchmarks Output: number of failed benchmarks
         */
        void getStatistics(i32& totalBenchmarks, i32& passedBenchmarks, i32& failedBenchmarks) const;
        
    private:
        std::vector<BenchmarkResult> results_;
        PerformanceThresholds thresholds_;
        LuaState* benchmarkState_;
        
        // Internal helper methods
        void initializeBenchmarkSuite_();
        void cleanupBenchmarkSuite_();
        
        // Individual benchmark implementations
        BenchmarkResult benchmarkVMExecution_();
        BenchmarkResult benchmarkMemoryAllocation_();
        BenchmarkResult benchmarkTableOperations_();
        BenchmarkResult benchmarkFunctionCalls_();
        BenchmarkResult benchmarkGarbageCollection_();
        BenchmarkResult benchmarkStackOperations_();
        BenchmarkResult benchmarkStringOperations_();
        BenchmarkResult benchmarkDebugHookOverhead_();
        
        // Utility methods
        double measureOperationsPerSecond_(std::function<void()> operation, i32 iterations);
        double getCurrentMemoryUsageMB_();
        void warmupVM_();
    };
    
    /**
     * @brief VM Execution Performance Benchmark
     */
    class VMExecutionBenchmark {
    public:
        static BenchmarkResult benchmarkArithmeticOperations(LuaState* L);
        static BenchmarkResult benchmarkControlFlow(LuaState* L);
        static BenchmarkResult benchmarkVariableAccess(LuaState* L);
        static BenchmarkResult benchmarkInstructionThroughput(LuaState* L);
    };
    
    /**
     * @brief Memory Management Performance Benchmark
     */
    class MemoryBenchmark {
    public:
        static BenchmarkResult benchmarkAllocationSpeed(LuaState* L);
        static BenchmarkResult benchmarkDeallocationSpeed(LuaState* L);
        static BenchmarkResult benchmarkGCThroughput(LuaState* L);
        static BenchmarkResult benchmarkMemoryFragmentation(LuaState* L);
    };
    
    /**
     * @brief Table Operations Performance Benchmark
     */
    class TableBenchmark {
    public:
        static BenchmarkResult benchmarkTableCreation(LuaState* L);
        static BenchmarkResult benchmarkTableAccess(LuaState* L);
        static BenchmarkResult benchmarkTableIteration(LuaState* L);
        static BenchmarkResult benchmarkTableResize(LuaState* L);
    };
    
    /**
     * @brief Function Call Performance Benchmark
     */
    class FunctionCallBenchmark {
    public:
        static BenchmarkResult benchmarkLuaFunctionCalls(LuaState* L);
        static BenchmarkResult benchmarkCFunctionCalls(LuaState* L);
        static BenchmarkResult benchmarkRecursiveCalls(LuaState* L);
        static BenchmarkResult benchmarkTailCalls(LuaState* L);
    };
    
    // Global benchmark utilities
    /**
     * @brief Measure execution time with high precision
     * @param func Function to measure
     * @return double Execution time in milliseconds
     */
    double measureHighPrecisionTime(std::function<void()> func);
    
    /**
     * @brief Run operation multiple times and get average time
     * @param operation Operation to run
     * @param iterations Number of iterations
     * @return double Average execution time in milliseconds
     */
    double measureAverageTime(std::function<void()> operation, i32 iterations);
    
    /**
     * @brief Get current memory usage
     * @return double Memory usage in MB
     */
    double getCurrentMemoryUsage();
    
    /**
     * @brief Calculate operations per second
     * @param totalTime Total execution time in milliseconds
     * @param operations Number of operations performed
     * @return double Operations per second
     */
    double calculateOpsPerSecond(double totalTime, i32 operations);
    
    /**
     * @brief Format benchmark result for display
     * @param result Benchmark result
     * @return std::string Formatted result string
     */
    std::string formatBenchmarkResult(const BenchmarkResult& result);
    
    /**
     * @brief Compare performance against baseline
     * @param current Current performance value
     * @param baseline Baseline performance value
     * @param tolerance Acceptable tolerance (percentage)
     * @return bool True if performance is acceptable
     */
    bool comparePerformance(double current, double baseline, double tolerance = 0.1);
    
    /**
     * @brief Generate performance summary
     * @param results Vector of benchmark results
     * @return std::string Performance summary
     */
    std::string generatePerformanceSummary(const std::vector<BenchmarkResult>& results);
    
} // namespace Testing
} // namespace Lua
