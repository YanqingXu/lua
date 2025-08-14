#include "../../gc/core/garbage_collector.hpp"
#include "../../gc/utils/gc_types.hpp"
#include "../../gc/core/gc_object.hpp"
#include "../../common/test_framework.hpp"
#include <iostream>
#include <memory>

namespace Lua {
namespace Tests {

    /**
     * @brief 基础GC功能测试
     * 
     * 测试不依赖完整VM系统的基础GC功能
     */
    class BasicGCTest {
    public:
        static void runAllTests() {
            std::cout << "=== Basic GC Functionality Tests ===" << std::endl;

            std::cout << "Starting test 1..." << std::endl;
            testGCStateEnum();
            std::cout << "Test 1 completed, starting test 2..." << std::endl;
            testGCObjectBasics();
            std::cout << "Test 2 completed, starting test 3..." << std::endl;
            testGCColorOperations();
            std::cout << "Test 3 completed, starting test 4..." << std::endl;
            testGCConfiguration();
            std::cout << "All tests completed!" << std::endl;

            std::cout << "=== Basic GC Tests Completed ===" << std::endl;
        }
        
    private:
        static void testGCStateEnum() {
            std::cout << "Testing GC State Enum..." << std::endl;

            try {
                std::cout << "  Creating GC states..." << std::endl;
                // Test Lua 5.1 compatible 5-state enum
                GCState pause = GCState::Pause;
                GCState propagate = GCState::Propagate;
                GCState sweepString = GCState::SweepString;
                GCState sweep = GCState::Sweep;
                GCState finalize = GCState::Finalize;
                std::cout << "  GC states created successfully" << std::endl;
                
                std::cout << "  Verifying state values..." << std::endl;
                // Verify state values
                TEST_ASSERT(static_cast<int>(pause) == 0, "Pause state should be 0");
                std::cout << "    Pause state OK" << std::endl;
                TEST_ASSERT(static_cast<int>(propagate) == 1, "Propagate state should be 1");
                std::cout << "    Propagate state OK" << std::endl;
                TEST_ASSERT(static_cast<int>(sweepString) == 2, "SweepString state should be 2");
                std::cout << "    SweepString state OK" << std::endl;
                TEST_ASSERT(static_cast<int>(sweep) == 3, "Sweep state should be 3");
                std::cout << "    Sweep state OK" << std::endl;
                TEST_ASSERT(static_cast<int>(finalize) == 4, "Finalize state should be 4");
                std::cout << "    Finalize state OK" << std::endl;

                std::cout << "[PASS] GC State Enum test passed" << std::endl;
                
            } catch (const std::exception& e) {
                std::cout << "[FAIL] GC State Enum test failed: " << e.what() << std::endl;
            }
        }

        static void testGCObjectBasics() {
            std::cout << "Testing GC Object Basics..." << std::endl;

            try {
                std::cout << "  Defining TestGCObject class..." << std::endl;
                // Create a simple GC object
                class TestGCObject : public GCObject {
                public:
                    TestGCObject() : GCObject(GCObjectType::String, sizeof(TestGCObject)) {}
                    
                    void markReferences(GarbageCollector* gc) override {
                        (void)gc; // 简单对象无引用
                    }
                    
                    usize getSize() const override {
                        return sizeof(TestGCObject);
                    }
                    
                    usize getAdditionalSize() const override {
                        return 0;
                    }
                };

                std::cout << "  Creating TestGCObject instance..." << std::endl;
                TestGCObject obj;
                std::cout << "  TestGCObject created successfully" << std::endl;
                
                // Test basic properties
                TEST_ASSERT(obj.getType() == GCObjectType::String, "Object type should be correct");
                TEST_ASSERT(obj.getSize() == sizeof(TestGCObject), "Object size should be correct");

                // Test color operations
                TEST_ASSERT(obj.getColor() == GCColor::White0, "Initial color should be White0");
                
                obj.setColor(GCColor::Gray);
                TEST_ASSERT(obj.getColor() == GCColor::Gray, "Setting color should work");
                TEST_ASSERT(obj.isGray(), "isGray() should return true");

                obj.setColor(GCColor::Black);
                TEST_ASSERT(obj.getColor() == GCColor::Black, "Setting black should work");
                TEST_ASSERT(obj.isBlack(), "isBlack() should return true");

                // Test type setting
                obj.setType(GCObjectType::Table);
                TEST_ASSERT(obj.getType() == GCObjectType::Table, "setType should work");

                std::cout << "[PASS] GC Object Basics test passed" << std::endl;

            } catch (const std::exception& e) {
                std::cout << "[FAIL] GC Object Basics test failed: " << e.what() << std::endl;
            }
        }

        static void testGCColorOperations() {
            std::cout << "Testing GC Color Operations..." << std::endl;
            
            try {
                class TestGCObject : public GCObject {
                public:
                    TestGCObject() : GCObject(GCObjectType::String) {}
                    void markReferences(GarbageCollector* gc) override { (void)gc; }
                    usize getSize() const override { return sizeof(TestGCObject); }
                    usize getAdditionalSize() const override { return 0; }
                };
                
                TestGCObject obj;
                
                // Test Lua 5.1 compatible mark operations
                u8 mark = obj.getGCMark();
                TEST_ASSERT(mark == static_cast<u8>(GCColor::White0), "Initial mark should be White0");

                // Set new mark
                obj.setGCMark(static_cast<u8>(GCColor::Gray));
                TEST_ASSERT(obj.getGCMark() == static_cast<u8>(GCColor::Gray), "setGCMark should work");
                TEST_ASSERT(obj.getColor() == GCColor::Gray, "Color should sync update");

                // Test white checking
                obj.setColor(GCColor::White0);
                TEST_ASSERT(obj.isWhite(), "White0 should be recognized as white");

                obj.setColor(GCColor::White1);
                TEST_ASSERT(obj.isWhite(), "White1 should be recognized as white");

                std::cout << "[PASS] GC Color Operations test passed" << std::endl;

            } catch (const std::exception& e) {
                std::cout << "[FAIL] GC Color Operations test failed: " << e.what() << std::endl;
            }
        }

        static void testGCConfiguration() {
            std::cout << "Testing GC Configuration..." << std::endl;

            try {
                // Test GC configuration structure
                GCConfig config;

                // Verify default values
                TEST_ASSERT(config.gcpause == 200, "Default gcpause should be 200");
                TEST_ASSERT(config.gcstepmul == 200, "Default gcstepmul should be 200");
                TEST_ASSERT(config.gcstepsize == 1024, "Default gcstepsize should be 1024");

                // Modify configuration
                config.gcpause = 150;
                config.gcstepmul = 300;
                config.gcstepsize = 2048;

                TEST_ASSERT(config.gcpause == 150, "gcpause modification should work");
                TEST_ASSERT(config.gcstepmul == 300, "gcstepmul modification should work");
                TEST_ASSERT(config.gcstepsize == 2048, "gcstepsize modification should work");

                std::cout << "[PASS] GC Configuration test passed" << std::endl;

            } catch (const std::exception& e) {
                std::cout << "[FAIL] GC Configuration test failed: " << e.what() << std::endl;
            }
        }
    };

} // namespace Tests
} // namespace Lua

// Test entry point
int main() {
    try {
        Lua::Tests::BasicGCTest::runAllTests();
        std::cout << "\nAll basic tests completed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed: " << e.what() << std::endl;
        return 1;
    }
}
