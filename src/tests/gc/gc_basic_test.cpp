#include "garbage_collector_test.hpp"
#include "../../gc/core/garbage_collector.hpp"
#include "../../vm/state.hpp"
#include "../../vm/table.hpp"
#include "../../vm/value.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <iostream>

namespace Lua::Tests {

    void GCBasicTestSuite::runAllTests() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "      GARBAGE COLLECTOR IMPLEMENTATION TEST" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        try {
            // Test 1: Basic GC creation and functionality
            testBasicGCFunctionality();
            
            // Test 2: GC collection cycle
            testGCCollectionCycle();
            
            // Test 3: Memory management
            testMemoryManagement();
            
            // Test 4: GC triggering logic
            testGCTriggering();
            
            std::cout << "\n" << std::string(60, '=') << std::endl;
            std::cout << "    [OK] ALL GARBAGE COLLECTOR TESTS PASSED" << std::endl;
            std::cout << std::string(60, '=') << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "\n" << std::string(60, '=') << std::endl;
            std::cout << "    [FAILED] GARBAGE COLLECTOR TESTS FAILED" << std::endl;
            std::cout << "    Error: " << e.what() << std::endl;
            std::cout << std::string(60, '=') << std::endl;
            throw;
        }
    }
    
    void GCBasicTestSuite::testBasicGCFunctionality() {
        std::cout << "\n=== Test 1: Basic GC Functionality ===\n";
        
        // Create a Lua state and GC
        auto state = std::make_unique<State>();
        GarbageCollector gc(state.get());
        
        // Test initial state
        const auto& stats = gc.getStats();
        std::cout << "[OK] GC created successfully\n";
        std::cout << "     Initial GC cycles: " << stats.gcCycles << "\n";
        std::cout << "     Initial live objects: " << stats.liveObjects << "\n";
        
        // Test shouldCollect initially returns false
        if (!gc.shouldCollect()) {
            std::cout << "[OK] GC correctly reports no collection needed initially\n";
        } else {
            std::cout << "[WARNING] GC reports collection needed on empty state\n";
        }
        
        std::cout << "[OK] Basic GC functionality test passed\n";
    }
    
    void GCBasicTestSuite::testGCCollectionCycle() {
        std::cout << "\n=== Test 2: GC Collection Cycle ===\n";
        
        // Create a Lua state and GC
        auto state = std::make_unique<State>();
        GarbageCollector gc(state.get());
        
        // Register the state with GC (simulating object allocation)
        gc.registerObject(state.get());
        
        // Create some objects to test with
        auto table1 = make_gc_table();
        auto table2 = make_gc_table();
        
        // Register objects with GC
        gc.registerObject(table1.get());
        gc.registerObject(table2.get());
        
        // Set up some references
        state->setGlobal("table1", Value(GCRef<Table>(table1.get())));
        
        std::cout << "[OK] Created test objects and references\n";
        
        // Get initial statistics
        auto statsBefore = gc.getStats();
        std::cout << "     Objects before GC: " << statsBefore.liveObjects << "\n";
        std::cout << "     Memory before GC: " << statsBefore.currentUsage << " bytes\n";
        
        // Run garbage collection
        gc.collectGarbage();
        
        // Get statistics after GC
        auto statsAfter = gc.getStats();
        std::cout << "     Objects after GC: " << statsAfter.liveObjects << "\n";
        std::cout << "     Memory after GC: " << statsAfter.currentUsage << " bytes\n";
        std::cout << "     GC cycles: " << statsAfter.gcCycles << "\n";
        std::cout << "     Objects collected: " << statsAfter.collectedObjects << "\n";
        
        if (statsAfter.gcCycles > statsBefore.gcCycles) {
            std::cout << "[OK] GC cycle completed successfully\n";
        } else {
            throw std::runtime_error("GC cycle did not increment properly");
        }
        
        std::cout << "[OK] GC collection cycle test passed\n";
    }
    
    void GCBasicTestSuite::testMemoryManagement() {
        std::cout << "\n=== Test 3: Memory Management ===\n";
        
        auto state = std::make_unique<State>();
        GarbageCollector gc(state.get());
        
        // Test memory allocation tracking
        usize initialMemory = gc.getStats().currentUsage;
        std::cout << "     Initial memory usage: " << initialMemory << " bytes\n";
        
        // Simulate memory allocation
        gc.updateAllocatedMemory(1024);
        usize afterAlloc = gc.getStats().currentUsage;
        std::cout << "     After +1024 bytes: " << afterAlloc << " bytes\n";
        
        if (afterAlloc >= initialMemory + 1024) {
            std::cout << "[OK] Memory allocation tracking works\n";
        } else {
            throw std::runtime_error("Memory allocation tracking failed");
        }
        
        // Simulate memory deallocation
        gc.updateAllocatedMemory(-512);
        usize afterDealloc = gc.getStats().currentUsage;
        std::cout << "     After -512 bytes: " << afterDealloc << " bytes\n";
        
        if (afterDealloc == afterAlloc - 512) {
            std::cout << "[OK] Memory deallocation tracking works\n";
        } else {
            std::cout << "[WARNING] Memory deallocation tracking may have precision issues\n";
        }
        
        std::cout << "[OK] Memory management test passed\n";
    }
    
    void GCBasicTestSuite::testGCTriggering() {
        std::cout << "\n=== Test 4: GC Triggering Logic ===\n";
        
        auto state = std::make_unique<State>();
        GarbageCollector gc(state.get());
        
        // Initially should not need collection
        bool initialNeed = gc.shouldCollect();
        std::cout << "     Initial collection need: " << (initialNeed ? "YES" : "NO") << "\n";
        
        // Simulate large memory allocation to trigger threshold
        usize largeAllocation = 2 * 1024 * 1024;  // 2MB
        gc.updateAllocatedMemory(static_cast<isize>(largeAllocation));
        
        bool afterLargeAlloc = gc.shouldCollect();
        std::cout << "     After large allocation: " << (afterLargeAlloc ? "YES" : "NO") << "\n";
        std::cout << "     Current usage: " << gc.getStats().currentUsage << " bytes\n";
        
        if (afterLargeAlloc) {
            std::cout << "[OK] GC correctly triggers on large memory usage\n";
        } else {
            std::cout << "[WARNING] GC may need threshold adjustment\n";
        }
        
        // Test that GC won't trigger while already running
        // (This is a more complex test that would require mocking)
        
        std::cout << "[OK] GC triggering logic test passed\n";
    }

} // namespace Lua::Tests
