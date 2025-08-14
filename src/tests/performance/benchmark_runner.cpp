#include "gc_benchmark.hpp"
#include <iostream>
#include <exception>

using namespace Lua::Performance;

int main() {
    try {
        std::cout << "Starting Lua 5.1 GC Performance Benchmark..." << std::endl;
        std::cout << "=============================================" << std::endl << std::endl;
        
        GCBenchmark benchmark;
        benchmark.runAllBenchmarks();
        
        std::cout << std::endl << "Benchmark completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed with error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Benchmark failed with unknown error" << std::endl;
        return 1;
    }
}
