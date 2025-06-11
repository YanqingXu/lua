#include <gtest/gtest.h>
#include "../gc/core/string_pool.hpp"
#include "../gc/core/gc_string.hpp"
#include <thread>
#include <vector>
#include <set>

using namespace Lua;

class StringPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear the string pool before each test
        StringPool::getInstance().clear();
    }
    
    void TearDown() override {
        // Clear the string pool after each test
        StringPool::getInstance().clear();
    }
};

// Test basic string interning functionality
TEST_F(StringPoolTest, BasicInterning) {
    StringPool& pool = StringPool::getInstance();
    
    // Create two strings with the same content
    GCString* str1 = GCString::create("hello");
    GCString* str2 = GCString::create("hello");
    
    // They should be the same object (interned)
    EXPECT_EQ(str1, str2);
    EXPECT_EQ(str1->getString(), "hello");
    EXPECT_EQ(str2->getString(), "hello");
    
    // Pool should contain only one string
    EXPECT_EQ(pool.size(), 1);
}

// Test that different strings create different objects
TEST_F(StringPoolTest, DifferentStrings) {
    StringPool& pool = StringPool::getInstance();
    
    GCString* str1 = GCString::create("hello");
    GCString* str2 = GCString::create("world");
    
    // They should be different objects
    EXPECT_NE(str1, str2);
    EXPECT_EQ(str1->getString(), "hello");
    EXPECT_EQ(str2->getString(), "world");
    
    // Pool should contain two strings
    EXPECT_EQ(pool.size(), 2);
}

// Test C-style string interning
TEST_F(StringPoolTest, CStringInterning) {
    StringPool& pool = StringPool::getInstance();
    
    GCString* str1 = GCString::create("test");
    GCString* str2 = GCString::create("test"); // C-style string
    
    EXPECT_EQ(str1, str2);
    EXPECT_EQ(pool.size(), 1);
}

// Test move semantics interning
TEST_F(StringPoolTest, MoveSemantics) {
    StringPool& pool = StringPool::getInstance();
    
    std::string content = "movable";
    GCString* str1 = GCString::create(std::string(content));
    GCString* str2 = GCString::create(std::move(content));
    
    EXPECT_EQ(str1, str2);
    EXPECT_EQ(pool.size(), 1);
}

// Test empty string handling
TEST_F(StringPoolTest, EmptyString) {
    StringPool& pool = StringPool::getInstance();
    
    GCString* str1 = GCString::create("");
    GCString* str2 = GCString::create("");
    GCString* str3 = GCString::create(nullptr); // Should become empty string
    
    EXPECT_EQ(str1, str2);
    EXPECT_EQ(str1, str3);
    EXPECT_TRUE(str1->empty());
    EXPECT_EQ(pool.size(), 1);
}

// Test string comparison
TEST_F(StringPoolTest, StringComparison) {
    GCString* str1 = GCString::create("abc");
    GCString* str2 = GCString::create("abc");
    GCString* str3 = GCString::create("def");
    
    // Same content should be equal
    EXPECT_TRUE(*str1 == *str2);
    EXPECT_TRUE(*str1 == "abc");
    
    // Different content should not be equal
    EXPECT_FALSE(*str1 == *str3);
    EXPECT_FALSE(*str1 == "def");
    
    // Test ordering
    EXPECT_TRUE(*str1 < *str3);
}

// Test hash consistency
TEST_F(StringPoolTest, HashConsistency) {
    GCString* str1 = GCString::create("hash_test");
    GCString* str2 = GCString::create("hash_test");
    
    // Same strings should have same hash
    EXPECT_EQ(str1->getHash(), str2->getHash());
    
    // Different strings should likely have different hashes
    GCString* str3 = GCString::create("different");
    EXPECT_NE(str1->getHash(), str3->getHash());
}

// Test memory usage tracking
TEST_F(StringPoolTest, MemoryUsage) {
    StringPool& pool = StringPool::getInstance();
    
    usize initialUsage = pool.getMemoryUsage();
    
    GCString* str1 = GCString::create("memory_test");
    usize afterFirstString = pool.getMemoryUsage();
    
    // Memory usage should increase
    EXPECT_GT(afterFirstString, initialUsage);
    
    // Creating the same string again should not increase memory much
    GCString* str2 = GCString::create("memory_test");
    usize afterSecondString = pool.getMemoryUsage();
    
    EXPECT_EQ(str1, str2); // Should be the same object
    EXPECT_EQ(afterFirstString, afterSecondString); // Memory usage should be the same
}

// Test thread safety
TEST_F(StringPoolTest, ThreadSafety) {
    const int numThreads = 4;
    const int stringsPerThread = 100;
    std::vector<std::thread> threads;
    std::vector<std::vector<GCString*>> results(numThreads);
    
    // Create multiple threads that create the same strings
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < stringsPerThread; ++i) {
                std::string content = "thread_test_" + std::to_string(i % 10);
                results[t].push_back(GCString::create(content));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify that strings with the same content are the same object
    for (int i = 0; i < stringsPerThread; ++i) {
        GCString* reference = results[0][i];
        for (int t = 1; t < numThreads; ++t) {
            EXPECT_EQ(reference, results[t][i]);
        }
    }
    
    // Pool should contain only 10 unique strings (0-9)
    EXPECT_EQ(StringPool::getInstance().size(), 10);
}

// Test getAllStrings functionality
TEST_F(StringPoolTest, GetAllStrings) {
    StringPool& pool = StringPool::getInstance();
    
    std::set<std::string> expectedStrings = {"alpha", "beta", "gamma", "delta"};
    std::set<GCString*> createdStrings;
    
    // Create strings
    for (const auto& str : expectedStrings) {
        createdStrings.insert(GCString::create(str));
    }
    
    // Get all strings from pool
    auto allStrings = pool.getAllStrings();
    
    EXPECT_EQ(allStrings.size(), expectedStrings.size());
    
    // Verify all created strings are in the result
    for (GCString* str : allStrings) {
        EXPECT_TRUE(createdStrings.count(str) > 0);
        EXPECT_TRUE(expectedStrings.count(str->getString()) > 0);
    }
}

// Test pool clearing
TEST_F(StringPoolTest, PoolClearing) {
    StringPool& pool = StringPool::getInstance();
    
    // Create some strings
    GCString::create("clear_test_1");
    GCString::create("clear_test_2");
    GCString::create("clear_test_3");
    
    EXPECT_EQ(pool.size(), 3);
    EXPECT_FALSE(pool.empty());
    
    // Clear the pool
    pool.clear();
    
    EXPECT_EQ(pool.size(), 0);
    EXPECT_TRUE(pool.empty());
}