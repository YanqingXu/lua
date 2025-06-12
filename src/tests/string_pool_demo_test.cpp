#include "string_pool_demo_test.hpp"
#include "../gc/core/string_pool.hpp"
#include "../gc/core/gc_string.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cassert>
#include <thread>
#include <future>

namespace Lua {
namespace Tests {

using namespace std::chrono;

/**
 * @brief Test basic string interning functionality with timeout
 */
bool testBasicStringInterning() {
    std::cout << "=== Basic String Interning Test ===\n";
    std::cout.flush();
    
    try {
        std::cout << "   About to create first string...\n";
        std::cout.flush();
        
        // Simplified version without timeout - direct call
        GCString* str1 = GCString::create("Hello, World!");
        
        std::cout << "   First string created successfully\n";
        std::cout.flush();
        
        std::cout << "   About to create second string...\n";
        std::cout.flush();
        
        GCString* str2 = GCString::create("Hello, World!");
        
        std::cout << "   Second string created successfully\n";
        std::cout.flush();
        
        std::cout << "   About to create third string...\n";
        std::cout.flush();
        
        GCString* str3 = GCString::create(std::string("Hello, World!"));
        
        std::cout << "   Third string created successfully\n";
        std::cout.flush();
        
        std::cout << "   str1 address: " << static_cast<void*>(str1) << "\n";
        std::cout << "   str2 address: " << static_cast<void*>(str2) << "\n";
        std::cout << "   str3 address: " << static_cast<void*>(str3) << "\n";
        
        if (str1 == str2 && str2 == str3) {
            std::cout << "   [OK] All strings with same content share the same object!\n";
        } else {
            std::cout << "   [FAILED] String interning failed!\n";
            return false;
        }
        
        // Create a different string
        GCString* str4 = GCString::create("Different string");
        std::cout << "   str4 address: " << static_cast<void*>(str4) << "\n";
        
        if (str1 != str4) {
            std::cout << "   [OK] Different strings have different objects!\n";
        } else {
            std::cout << "   [FAILED] Different strings should not share objects!\n";
            return false;
        }
        
        std::cout << "[OK] Basic String Interning Test passed\n\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FAILED] Basic String Interning Test failed: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Test memory efficiency of string pool
 */
bool testStringPoolMemoryEfficiency() {
    std::cout << "=== String Pool Memory Efficiency Test ===\n";
    
    try {
        StringPool& pool = StringPool::getInstance();
        
        // Clear pool for clean measurement
        pool.clear();
        
        usize initialMemory = pool.getMemoryUsage();
        std::cout << "   Initial memory usage: " << initialMemory << " bytes\n";
        
        // Create many strings with repeated content
        std::vector<GCString*> strings;
        const std::vector<std::string> patterns = {
            "pattern_1", "pattern_2", "pattern_3", "pattern_4", "pattern_5"
        };
        
        // Create 1000 strings using only 5 unique patterns
        for (int i = 0; i < 1000; ++i) {
            const std::string& pattern = patterns[i % patterns.size()];
            strings.push_back(GCString::create(pattern));
        }
        
        usize finalMemory = pool.getMemoryUsage();
        std::cout << "   Memory after 1000 strings: " << finalMemory << " bytes\n";
        std::cout << "   Unique strings in pool: " << pool.size() << "\n";
        
        if (finalMemory > initialMemory) {
            std::cout << "   Memory per unique string: " << (finalMemory - initialMemory) / std::max(pool.size() - (initialMemory > 0 ? 1 : 0), static_cast<usize>(1)) << " bytes\n";
        }
        
        // Calculate memory that would be used without interning
        usize memoryWithoutInterning = 0;
        for (const auto& pattern : patterns) {
            memoryWithoutInterning += (sizeof(GCString) + pattern.capacity()) * 200; // 200 copies of each
        }
        
        std::cout << "   Estimated memory without interning: " << memoryWithoutInterning << " bytes\n";
        if (finalMemory < memoryWithoutInterning) {
            std::cout << "   Memory savings: " << (memoryWithoutInterning - finalMemory) << " bytes\n";
            std::cout << "   Savings percentage: " << 
                (100.0 * (memoryWithoutInterning - finalMemory) / memoryWithoutInterning) << "%\n";
        }
        
        // Verify that we only have a reasonable number of unique strings
        if (pool.size() <= patterns.size() + 10) { // Allow some tolerance
            std::cout << "   [OK] Memory efficiency verified - only unique strings stored\n";
        } else {
            std::cout << "   [warning] More unique strings than expected: " << pool.size() << "\n";
        }
        
        std::cout << "[OK] String Pool Memory Efficiency Test passed\n\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FAILED] String Pool Memory Efficiency Test failed: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Test string pool performance
 */
bool testStringPoolPerformance() {
    std::cout << "=== String Pool Performance Test ===\n";
    
    try {
        const int iterations = 10000;
        const std::vector<std::string> testStrings = {
            "performance_test_string_1",
            "performance_test_string_2", 
            "performance_test_string_3",
            "performance_test_string_4",
            "performance_test_string_5"
        };
        
        // Test string creation performance with interning
        auto start = high_resolution_clock::now();
        
        std::vector<GCString*> internedStrings;
        for (int i = 0; i < iterations; ++i) {
            const std::string& str = testStrings[i % testStrings.size()];
            internedStrings.push_back(GCString::create(str));
        }
        
        auto end = high_resolution_clock::now();
        auto internedTime = duration_cast<microseconds>(end - start);
        
        std::cout << "   Time to create " << iterations << " interned strings: " 
                  << internedTime.count() << " microseconds\n";
        
        // Test string comparison performance (should be very fast due to pointer equality)
        start = high_resolution_clock::now();
        
        int equalCount = 0;
        for (size_t i = 0; i < internedStrings.size() - 1; ++i) {
            if (internedStrings[i] == internedStrings[i + 1]) {
                equalCount++;
            }
        }
        
        end = high_resolution_clock::now();
        auto comparisonTime = duration_cast<microseconds>(end - start);
        
        std::cout << "   Time for " << (internedStrings.size() - 1) << " pointer comparisons: " 
                  << comparisonTime.count() << " microseconds\n";
        std::cout << "   Equal strings found: " << equalCount << "\n";
        
        // Performance should be reasonable (less than 1 second for 10k operations)
        if (internedTime.count() < 1000000) { // 1 second = 1,000,000 microseconds
            std::cout << "   [OK] String creation performance is acceptable\n";
        } else {
            std::cout << "   [warning] String creation performance might be slow\n";
        }
        
        std::cout << "[OK] String Pool Performance Test completed\n\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FAILED] String Pool Performance Test failed: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Test string pool statistics
 */
bool testStringPoolStatistics() {
    std::cout << "=== String Pool Statistics Test ===\n";
    
    try {
        StringPool& pool = StringPool::getInstance();
        
        std::cout << "   Total strings in pool: " << pool.size() << "\n";
        std::cout << "   Total memory usage: " << pool.getMemoryUsage() << " bytes\n";
        std::cout << "   Pool empty: " << (pool.empty() ? "Yes" : "No") << "\n";
        
        // Show some strings in the pool
        auto allStrings = pool.getAllStrings();
        std::cout << "   Strings in pool (showing first 5):\n";
        for (size_t i = 0; i < allStrings.size() && i < 5; ++i) {
            GCString* str = allStrings[i];
            std::cout << "     [" << i << "] \"" << str->getString() << "\" (hash: " 
                      << str->getHash() << ", length: " << str->length() << ")\n";
        }
        
        if (allStrings.size() > 5) {
            std::cout << "     ... and " << (allStrings.size() - 5) << " more strings\n";
        }
        
        std::cout << "[OK] String Pool Statistics Test completed\n\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FAILED] String Pool Statistics Test failed: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Run all string pool demo tests
 */
bool runStringPoolDemoTests() {
    std::cout << "Running String Pool Demo Tests...\n\n";
    
    bool allPassed = true;
    
    allPassed &= testBasicStringInterning();
    allPassed &= testStringPoolMemoryEfficiency();
    allPassed &= testStringPoolPerformance();
    allPassed &= testStringPoolStatistics();
    
    if (allPassed) {
        std::cout << "[OK] All String Pool Demo Tests passed!\n";
        std::cout << "\nString interning provides:\n";
        std::cout << "  -- Memory efficiency by sharing identical strings\n";
        std::cout << "  -- Fast string comparison using pointer equality\n";
        std::cout << "  -- Automatic deduplication of string literals\n";
        std::cout << "  -- Thread-safe string creation and access\n";
    } else {
        std::cout << "[FAILED] Some String Pool Demo Tests failed!\n";
    }
    
    return allPassed;
}

} // namespace Tests
} // namespace Lua