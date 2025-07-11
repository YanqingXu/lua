#include "userdata_test.hpp"
#include <chrono>
#include <iostream>

namespace Lua {
    namespace Tests {
        
        // Test implementations are in the header file using TEST_F macros
        // This file can contain additional test helper functions if needed
        
        /**
         * @brief Helper function to create test userdata with specific pattern
         * @param size Size of userdata to create
         * @param pattern Byte pattern to fill the data with
         * @return GCRef to created userdata
         */
        GCRef<Userdata> createTestUserdata(usize size, u8 pattern = 0xAA) {
            auto ud = Userdata::createFull(size);
            
            // Fill with test pattern
            auto* data = static_cast<u8*>(ud->getData());
            for (usize i = 0; i < size; ++i) {
                data[i] = pattern;
            }
            
            return ud;
        }
        
        /**
         * @brief Helper function to verify userdata contains expected pattern
         * @param ud Userdata to verify
         * @param pattern Expected byte pattern
         * @return true if pattern matches
         */
        bool verifyUserdataPattern(const GCRef<Userdata>& ud, u8 pattern) {
            if (!ud || ud->getType() != UserdataType::Full) {
                return false;
            }
            
            auto* data = static_cast<const u8*>(ud->getData());
            for (usize i = 0; i < ud->getUserDataSize(); ++i) {
                if (data[i] != pattern) {
                    return false;
                }
            }
            
            return true;
        }
        
        /**
         * @brief Additional test for userdata memory alignment
         */
        TEST_F(UserdataTest, MemoryAlignment) {
            // Test various sizes to ensure proper alignment
            Vec<usize> testSizes = {1, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256};
            
            for (usize size : testSizes) {
                auto ud = Userdata::createFull(size);
                
                // Verify data pointer is properly aligned (8-byte boundary)
                uintptr_t dataAddr = reinterpret_cast<uintptr_t>(ud->getData());
                EXPECT_EQ(dataAddr % 8, 0) << "Data not aligned for size " << size;
                
                // Verify we can write to all bytes without issues
                auto* data = static_cast<u8*>(ud->getData());
                for (usize i = 0; i < size; ++i) {
                    data[i] = static_cast<u8>(i & 0xFF);
                }
                
                // Verify the data was written correctly
                for (usize i = 0; i < size; ++i) {
                    EXPECT_EQ(data[i], static_cast<u8>(i & 0xFF)) 
                        << "Data corruption at index " << i << " for size " << size;
                }
            }
        }
        
        /**
         * @brief Test userdata finalization
         */
        TEST_F(UserdataTest, Finalization) {
            auto ud = Userdata::createFull(64);
            
            // Set a metatable
            auto metatable = GCRef<Table>(new Table());
            ud->setMetatable(metatable);
            EXPECT_TRUE(ud->hasMetatable());
            
            // Call finalize
            ud->finalize();
            
            // Metatable should be cleared after finalization
            EXPECT_FALSE(ud->hasMetatable());
            EXPECT_EQ(ud->getMetatable(), GCRef<Table>(nullptr));
        }
        
        /**
         * @brief Test edge cases and error conditions
         */
        TEST_F(UserdataTest, EdgeCases) {
            // Test maximum reasonable size
            const usize maxSize = 1024 * 1024;  // 1MB
            auto largeUD = Userdata::createFull(maxSize);
            EXPECT_EQ(largeUD->getUserDataSize(), maxSize);
            EXPECT_NE(largeUD->getData(), nullptr);
            
            // Test that data is actually accessible
            auto* data = static_cast<u8*>(largeUD->getData());
            data[0] = 0x42;
            data[maxSize - 1] = 0x24;
            EXPECT_EQ(data[0], 0x42);
            EXPECT_EQ(data[maxSize - 1], 0x24);
        }
        
        /**
         * @brief Test userdata with complex data structures
         */
        TEST_F(UserdataTest, ComplexDataStructures) {
            struct ComplexStruct {
                i32 integers[10];
                f64 doubles[5];
                char strings[100];
                bool flags[8];
                
                ComplexStruct() {
                    for (i32 i = 0; i < 10; ++i) integers[i] = i;
                    for (i32 i = 0; i < 5; ++i) doubles[i] = i * 3.14;
                    std::strcpy(strings, "Hello, Userdata!");
                    for (i32 i = 0; i < 8; ++i) flags[i] = (i % 2 == 0);
                }
                
                bool verify() const {
                    for (i32 i = 0; i < 10; ++i) {
                        if (integers[i] != i) return false;
                    }
                    for (i32 i = 0; i < 5; ++i) {
                        if (std::abs(doubles[i] - i * 3.14) > 1e-10) return false;
                    }
                    if (std::strcmp(strings, "Hello, Userdata!") != 0) return false;
                    for (i32 i = 0; i < 8; ++i) {
                        if (flags[i] != (i % 2 == 0)) return false;
                    }
                    return true;
                }
            };
            
            // Create userdata with complex structure
            ComplexStruct testData;
            auto ud = makeFullUserdata(testData);
            
            // Verify the data was copied correctly
            auto* retrievedData = ud->getTypedData<ComplexStruct>();
            EXPECT_NE(retrievedData, nullptr);
            EXPECT_TRUE(retrievedData->verify());
            
            // Modify the data and verify changes
            retrievedData->integers[5] = 999;
            retrievedData->doubles[2] = 2.718;
            std::strcpy(retrievedData->strings, "Modified!");
            
            EXPECT_EQ(retrievedData->integers[5], 999);
            EXPECT_EQ(retrievedData->doubles[2], 2.718);
            EXPECT_STREQ(retrievedData->strings, "Modified!");
        }
        
        /**
         * @brief Test userdata type safety
         */
        TEST_F(UserdataTest, TypeSafety) {
            struct TypeA { i32 value = 42; };
            struct TypeB { f64 value = 3.14; };
            
            // Create userdata for TypeA
            TypeA objA{100};
            auto udA = makeFullUserdata(objA);
            
            // Verify correct type access
            EXPECT_TRUE(isUserdataType<TypeA>(udA));
            EXPECT_FALSE(isUserdataType<TypeB>(udA));
            
            auto* ptrA = udA->getTypedData<TypeA>();
            EXPECT_NE(ptrA, nullptr);
            EXPECT_EQ(ptrA->value, 100);
            
            // Verify incorrect type access fails safely
            auto* ptrB = udA->getTypedData<TypeB>();
            EXPECT_EQ(ptrB, nullptr);  // Should fail due to size mismatch
        }
        
        /**
         * @brief Stress test for userdata operations
         */
        TEST_F(UserdataTest, StressTest) {
            const usize numOperations = 100;
            Vec<GCRef<Userdata>> userdata;
            userdata.reserve(numOperations);
            
            // Create many userdata objects
            for (usize i = 0; i < numOperations; ++i) {
                if (i % 2 == 0) {
                    // Create light userdata
                    static i32 dummyData = static_cast<i32>(i);
                    userdata.push_back(Userdata::createLight(&dummyData));
                } else {
                    // Create full userdata
                    userdata.push_back(Userdata::createFull(64 + i));
                }
            }
            
            // Verify all userdata objects
            for (usize i = 0; i < numOperations; ++i) {
                EXPECT_NE(userdata[i], nullptr);
                
                if (i % 2 == 0) {
                    EXPECT_EQ(userdata[i]->getType(), UserdataType::Light);
                } else {
                    EXPECT_EQ(userdata[i]->getType(), UserdataType::Full);
                    EXPECT_EQ(userdata[i]->getUserDataSize(), 64 + i);
                }
            }
            
            // Test operations on all userdata
            for (auto& ud : userdata) {
                Value val(ud);
                EXPECT_TRUE(val.isUserdata());
                EXPECT_EQ(val.asUserdata(), ud);
            }
        }
    }
}
