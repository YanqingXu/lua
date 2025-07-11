#pragma once

#include "../../common/types.hpp"
#include "../../vm/userdata.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <gtest/gtest.h>
#include <memory>

namespace Lua {
    namespace Tests {
        
        /**
         * @brief Test suite for Userdata implementation
         * 
         * This test suite verifies the complete functionality of the Userdata
         * type according to Lua 5.1 specification, including both light and
         * full userdata variants.
         */
        class UserdataTest : public ::testing::Test {
        protected:
            void SetUp() override {
                // Setup test environment
            }
            
            void TearDown() override {
                // Cleanup test environment
            }
        };
        
        // === Light Userdata Tests ===
        
        /**
         * @brief Test light userdata creation and basic operations
         */
        TEST_F(UserdataTest, LightUserdataCreation) {
            // Test data
            i32 testValue = 42;
            void* testPtr = &testValue;
            
            // Create light userdata
            auto lightUD = Userdata::createLight(testPtr);
            
            // Verify basic properties
            EXPECT_EQ(lightUD->getType(), UserdataType::Light);
            EXPECT_EQ(lightUD->getData(), testPtr);
            EXPECT_EQ(lightUD->getUserDataSize(), 0);  // Light userdata has no size
            EXPECT_FALSE(lightUD->hasMetatable());
            
            // Verify metatable operations fail
            EXPECT_EQ(lightUD->getMetatable(), GCRef<Table>(nullptr));
            EXPECT_THROW(lightUD->setMetatable(GCRef<Table>(nullptr)), std::runtime_error);
        }
        
        /**
         * @brief Test light userdata with null pointer (should fail)
         */
        TEST_F(UserdataTest, LightUserdataNullPointer) {
            EXPECT_THROW(Userdata::createLight(nullptr), std::invalid_argument);
        }
        
        /**
         * @brief Test light userdata typed access
         */
        TEST_F(UserdataTest, LightUserdataTypedAccess) {
            struct TestStruct {
                i32 x = 10;
                f64 y = 3.14;
            };
            
            TestStruct testData;
            auto lightUD = Userdata::createLight(&testData);
            
            // Test typed data access
            auto* typedPtr = lightUD->getTypedData<TestStruct>();
            EXPECT_NE(typedPtr, nullptr);
            EXPECT_EQ(typedPtr->x, 10);
            EXPECT_EQ(typedPtr->y, 3.14);
            
            // Test wrong type access
            auto* wrongPtr = lightUD->getTypedData<i64>();
            EXPECT_EQ(wrongPtr, nullptr);  // Size mismatch
        }
        
        // === Full Userdata Tests ===
        
        /**
         * @brief Test full userdata creation and basic operations
         */
        TEST_F(UserdataTest, FullUserdataCreation) {
            const usize testSize = 128;
            
            // Create full userdata
            auto fullUD = Userdata::createFull(testSize);
            
            // Verify basic properties
            EXPECT_EQ(fullUD->getType(), UserdataType::Full);
            EXPECT_NE(fullUD->getData(), nullptr);
            EXPECT_EQ(fullUD->getUserDataSize(), testSize);
            EXPECT_FALSE(fullUD->hasMetatable());
            
            // Verify data is zero-initialized
            auto* data = static_cast<u8*>(fullUD->getData());
            for (usize i = 0; i < testSize; ++i) {
                EXPECT_EQ(data[i], 0);
            }
        }
        
        /**
         * @brief Test full userdata with zero size (should fail)
         */
        TEST_F(UserdataTest, FullUserdataZeroSize) {
            EXPECT_THROW(Userdata::createFull(0), std::invalid_argument);
        }
        
        /**
         * @brief Test full userdata typed operations
         */
        TEST_F(UserdataTest, FullUserdataTypedOperations) {
            struct TestData {
                i32 value1 = 100;
                f64 value2 = 2.718;
                bool flag = true;
            };
            
            auto fullUD = Userdata::createFull(sizeof(TestData));
            
            // Set typed data
            TestData testObj{200, 1.414, false};
            EXPECT_TRUE(fullUD->setTypedData(testObj));
            
            // Get typed data
            auto* retrievedData = fullUD->getTypedData<TestData>();
            EXPECT_NE(retrievedData, nullptr);
            EXPECT_EQ(retrievedData->value1, 200);
            EXPECT_EQ(retrievedData->value2, 1.414);
            EXPECT_EQ(retrievedData->flag, false);
            
            // Test size mismatch
            struct LargeStruct { char data[1000]; };
            LargeStruct largeObj{};
            EXPECT_FALSE(fullUD->setTypedData(largeObj));
        }
        
        /**
         * @brief Test full userdata metatable operations
         */
        TEST_F(UserdataTest, FullUserdataMetatable) {
            auto fullUD = Userdata::createFull(64);
            
            // Initially no metatable
            EXPECT_FALSE(fullUD->hasMetatable());
            EXPECT_EQ(fullUD->getMetatable(), GCRef<Table>(nullptr));
            
            // Create and set metatable
            auto metatable = GCRef<Table>(new Table());
            fullUD->setMetatable(metatable);
            
            // Verify metatable is set
            EXPECT_TRUE(fullUD->hasMetatable());
            EXPECT_EQ(fullUD->getMetatable(), metatable);
            
            // Clear metatable
            fullUD->setMetatable(GCRef<Table>(nullptr));
            EXPECT_FALSE(fullUD->hasMetatable());
        }
        
        // === Value Integration Tests ===
        
        /**
         * @brief Test userdata integration with Value system
         */
        TEST_F(UserdataTest, ValueIntegration) {
            // Test light userdata in Value
            i32 testInt = 42;
            auto lightUD = Userdata::createLight(&testInt);
            Value lightValue(lightUD);
            
            EXPECT_TRUE(lightValue.isUserdata());
            EXPECT_EQ(lightValue.type(), ValueType::Userdata);
            EXPECT_EQ(lightValue.asUserdata(), lightUD);
            EXPECT_EQ(lightValue.toString(), "userdata");
            
            // Test full userdata in Value
            auto fullUD = Userdata::createFull(256);
            Value fullValue(fullUD);
            
            EXPECT_TRUE(fullValue.isUserdata());
            EXPECT_EQ(fullValue.type(), ValueType::Userdata);
            EXPECT_EQ(fullValue.asUserdata(), fullUD);
            EXPECT_EQ(fullValue.toString(), "userdata");
            
            // Test Value comparison
            Value anotherLightValue(lightUD);
            EXPECT_EQ(lightValue, anotherLightValue);
            EXPECT_NE(lightValue, fullValue);
        }
        
        /**
         * @brief Test userdata GC object properties
         */
        TEST_F(UserdataTest, GCObjectProperties) {
            auto fullUD = Userdata::createFull(128);
            
            // Test GC object interface
            EXPECT_EQ(fullUD->getGCType(), GCObjectType::Userdata);
            EXPECT_GE(fullUD->getSize(), sizeof(Userdata) + 128);
            EXPECT_EQ(fullUD->getAdditionalSize(), 128);
            
            // Test Value GC integration
            Value udValue(fullUD);
            EXPECT_TRUE(udValue.isGCObject());
            EXPECT_EQ(udValue.asGCObject(), fullUD.get());
        }
        
        // === Utility Function Tests ===
        
        /**
         * @brief Test utility functions for userdata creation
         */
        TEST_F(UserdataTest, UtilityFunctions) {
            // Test makeLightUserdata
            f64 testDouble = 3.14159;
            auto lightUD = makeLightUserdata(&testDouble);
            EXPECT_EQ(lightUD->getType(), UserdataType::Light);
            EXPECT_EQ(lightUD->getData(), &testDouble);
            
            // Test makeFullUserdata
            struct TestStruct {
                i32 x = 42;
                Str name = "test";
            };
            TestStruct testObj{100, "hello"};
            auto fullUD = makeFullUserdata(testObj);
            
            EXPECT_EQ(fullUD->getType(), UserdataType::Full);
            EXPECT_EQ(fullUD->getUserDataSize(), sizeof(TestStruct));
            
            auto* retrievedObj = fullUD->getTypedData<TestStruct>();
            EXPECT_NE(retrievedObj, nullptr);
            EXPECT_EQ(retrievedObj->x, 100);
            EXPECT_EQ(retrievedObj->name, "hello");
            
            // Test isUserdataType
            EXPECT_TRUE(isUserdataType<TestStruct>(fullUD));
            EXPECT_FALSE(isUserdataType<i64>(fullUD));  // Size mismatch
        }
        
        // === Performance Tests ===
        
        /**
         * @brief Test userdata creation performance
         */
        TEST_F(UserdataTest, PerformanceTest) {
            const usize iterations = 1000;
            
            // Test light userdata creation performance
            i32 testValue = 42;
            auto start = std::chrono::high_resolution_clock::now();
            
            for (usize i = 0; i < iterations; ++i) {
                auto lightUD = Userdata::createLight(&testValue);
                // Use the userdata to prevent optimization
                EXPECT_EQ(lightUD->getType(), UserdataType::Light);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto lightDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Test full userdata creation performance
            start = std::chrono::high_resolution_clock::now();
            
            for (usize i = 0; i < iterations; ++i) {
                auto fullUD = Userdata::createFull(64);
                // Use the userdata to prevent optimization
                EXPECT_EQ(fullUD->getType(), UserdataType::Full);
            }
            
            end = std::chrono::high_resolution_clock::now();
            auto fullDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Performance requirements from development plan
            // Light userdata creation should be < 1 microsecond per operation
            EXPECT_LT(lightDuration.count() / iterations, 1.0);
            
            // Full userdata creation should be < 10 microseconds per operation
            EXPECT_LT(fullDuration.count() / iterations, 10.0);
            
            std::cout << "Light userdata creation: " 
                      << (lightDuration.count() / static_cast<f64>(iterations)) 
                      << " μs/op" << std::endl;
            std::cout << "Full userdata creation: " 
                      << (fullDuration.count() / static_cast<f64>(iterations)) 
                      << " μs/op" << std::endl;
        }
    }
}
