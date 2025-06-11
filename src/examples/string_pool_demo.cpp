#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include "../gc/core/string_pool.hpp"
#include "../gc/core/gc_string.hpp"

using namespace Lua;
using namespace std::chrono;

/**
 * @brief Demonstrate string pool functionality and performance benefits
 */
class StringPoolDemo {
public:
    static void runDemo() {
        std::cout << "=== String Pool (String Interning) Demo ===\n\n";
        
        demonstrateBasicInterning();
        demonstrateMemoryEfficiency();
        demonstratePerformance();
        demonstrateStatistics();
    }
    
private:
    static void demonstrateBasicInterning() {
        std::cout << "1. Basic String Interning:\n";
        
        // Create multiple strings with the same content
        GCString* str1 = GCString::create("Hello, World!");
        GCString* str2 = GCString::create("Hello, World!");
        GCString* str3 = GCString::create(std::string("Hello, World!"));
        
        std::cout << "   str1 address: " << static_cast<void*>(str1) << "\n";
        std::cout << "   str2 address: " << static_cast<void*>(str2) << "\n";
        std::cout << "   str3 address: " << static_cast<void*>(str3) << "\n";
        
        if (str1 == str2 && str2 == str3) {
            std::cout << "   ✓ All strings with same content share the same object!\n";
        } else {
            std::cout << "   ✗ String interning failed!\n";
        }
        
        // Create a different string
        GCString* str4 = GCString::create("Different string");
        std::cout << "   str4 address: " << static_cast<void*>(str4) << "\n";
        
        if (str1 != str4) {
            std::cout << "   ✓ Different strings have different objects!\n";
        }
        
        std::cout << "\n";
    }
    
    static void demonstrateMemoryEfficiency() {
        std::cout << "2. Memory Efficiency:\n";
        
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
        std::cout << "   Memory per unique string: " << (finalMemory - initialMemory) / pool.size() << " bytes\n";
        
        // Calculate memory that would be used without interning
        usize memoryWithoutInterning = 0;
        for (const auto& pattern : patterns) {
            memoryWithoutInterning += (sizeof(GCString) + pattern.capacity()) * 200; // 200 copies of each
        }
        
        std::cout << "   Estimated memory without interning: " << memoryWithoutInterning << " bytes\n";
        std::cout << "   Memory savings: " << (memoryWithoutInterning - (finalMemory - initialMemory)) << " bytes\n";
        std::cout << "   Savings percentage: " << 
            (100.0 * (memoryWithoutInterning - (finalMemory - initialMemory)) / memoryWithoutInterning) << "%\n";
        
        std::cout << "\n";
    }
    
    static void demonstratePerformance() {
        std::cout << "3. Performance Comparison:\n";
        
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
        
        std::cout << "\n";
    }
    
    static void demonstrateStatistics() {
        std::cout << "4. String Pool Statistics:\n";
        
        StringPool& pool = StringPool::getInstance();
        
        std::cout << "   Total strings in pool: " << pool.size() << "\n";
        std::cout << "   Total memory usage: " << pool.getMemoryUsage() << " bytes\n";
        std::cout << "   Pool empty: " << (pool.empty() ? "Yes" : "No") << "\n";
        
        // Show all strings in the pool
        auto allStrings = pool.getAllStrings();
        std::cout << "   Strings in pool:\n";
        for (size_t i = 0; i < allStrings.size() && i < 10; ++i) { // Show first 10
            GCString* str = allStrings[i];
            std::cout << "     [" << i << "] \"" << str->getString() << "\" (hash: " 
                      << str->getHash() << ", length: " << str->length() << ")\n";
        }
        
        if (allStrings.size() > 10) {
            std::cout << "     ... and " << (allStrings.size() - 10) << " more strings\n";
        }
        
        std::cout << "\n";
    }
};

int main() {
    try {
        StringPoolDemo::runDemo();
        
        std::cout << "Demo completed successfully!\n";
        std::cout << "\nString interning provides:\n";
        std::cout << "  • Memory efficiency by sharing identical strings\n";
        std::cout << "  • Fast string comparison using pointer equality\n";
        std::cout << "  • Automatic deduplication of string literals\n";
        std::cout << "  • Thread-safe string creation and access\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}