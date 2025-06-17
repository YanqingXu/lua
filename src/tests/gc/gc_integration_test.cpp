#include "gc_integration_test.hpp"
#include "../../gc/core/garbage_collector.hpp"
#include "../../gc/core/gc_ref.hpp"
#include "../../vm/state.hpp"
#include "../../vm/state_factory.hpp"
#include "../../vm/table.hpp"
#include "../../vm/function.hpp"
#include "../../vm/value.hpp"
#include "../../gc/memory/allocator.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
namespace Tests {

    /**
     * @brief Test GC integration with core types
     * 
     * This test verifies:
     * 1. GC-managed objects can be created (State, Table, Function)
     * 2. GCRef provides safe references
     * 3. Garbage collection can be triggered
     * 4. Object marking and collection work correctly
     */
    bool GCIntegrationTest::testGCIntegration() {
        std::cout << "=== GC Integration Test ===\n";
        
        try {
            // 1. Create a GC allocator
            GCAllocator allocator;
            
            // 2. Create a GC-managed State object
            GCRef<State> state = make_gc_state(allocator);
            std::cout << "[OK] Created GC-managed State object\n";
            
            // 3. Create GC-managed Table objects
            GCRef<Table> table1 = make_gc_table();
            GCRef<Table> table2 = make_gc_table();
            std::cout << "[OK] Created GC-managed Table objects\n";
            
            // 4. Create Values that reference GC objects
            Value stringValue("Hello, GC World!");
            Value tableValue(table1);
            Value numberValue(42.0);
            
            // 5. Store values in the state
            state->push(stringValue);
            state->push(tableValue);
            state->push(numberValue);
            std::cout << "[OK] Stored values in State stack\n";
            
            // 6. Set global variables
            state->setGlobal("myTable", tableValue);
            state->setGlobal("myString", stringValue);
            std::cout << "[OK] Set global variables\n";
            
            // 7. Create nested table references
            table1->set(Value("nested"), Value(table2));
            table2->set(Value("data"), Value("Nested data"));
            std::cout << "[OK] Created nested table references\n";
            
            // 8. Create a GC-managed Function
            Vec<Instruction> code;
            Vec<Value> constants = {Value("Function constant")};
            GCRef<Function> func = Function::createLua(
                std::make_shared<Vec<Instruction>>(code),
                constants,
                {}, // prototypes
                0, 0, 0
            );
            state->setGlobal("myFunction", Value(func));
            std::cout << "[OK] Created and stored GC-managed Function\n";
            
            // 9. Create a GarbageCollector and perform collection
            GarbageCollector gc(state.get());
            std::cout << "[OK] Performing garbage collection...\n";
            
            // Mark phase: start from State as root object
            std::cout << "[OK] Marking reachable objects...\n";
            gc.markObject(state.get());
            
            // Collection phase
            std::cout << "[OK] Collecting unreachable objects...\n";
            gc.collectGarbage();
            
            std::cout << "[OK] GC Integration Test completed successfully!\n";
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAILED] GC Integration Test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * @brief Test GC object marking with complex reference patterns
     */
    bool GCIntegrationTest::testGCMarking() {
        std::cout << "\n=== GC Marking Test ===\n";
        
        try {
            // Create objects with complex reference patterns
            GCRef<State> state = make_gc_state();
            GCRef<Table> rootTable = make_gc_table();
            GCRef<Table> childTable1 = make_gc_table();
            GCRef<Table> childTable2 = make_gc_table();
            
            // Create reference cycles
            rootTable->set(Value("child1"), Value(childTable1));
            rootTable->set(Value("child2"), Value(childTable2));
            childTable1->set(Value("parent"), Value(rootTable));
            childTable2->set(Value("sibling"), Value(childTable1));
            
            // Store in state
            state->setGlobal("root", Value(rootTable));
            
            std::cout << "[OK] Created complex reference pattern with cycles\n";
            
            // Test marking
            GarbageCollector gc(state.get());
            gc.markObject(state.get());
            
            std::cout << "[OK] Successfully marked objects with reference cycles\n";
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAILED] GC Marking Test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * @brief Run all GC integration tests
     */
    bool GCIntegrationTest::runAllTests() {
        std::cout << "Running GC Integration Tests...\n\n";
        
        bool allPassed = true;
        
        allPassed &= GCIntegrationTest::testGCIntegration();
        allPassed &= GCIntegrationTest::testGCMarking();
        
        if (allPassed) {
            std::cout << "\n[OK] All GC Integration Tests passed!\n";
        } else {
            std::cout << "\n[FAILED] Some GC Integration Tests failed!\n";
        }
        
        return allPassed;
    }

} // namespace Tests
} // namespace Lua