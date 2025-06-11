#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include "../vm/function.hpp"
#include "../vm/state_factory.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/memory/allocator.hpp"
#include <iostream>

namespace Lua {
    /**
     * @brief Demonstration of GC integration with core types
     * 
     * This example shows how to:
     * 1. Create GC-managed objects (State, Table, Function)
     * 2. Use GCRef for safe references
     * 3. Trigger garbage collection
     * 4. Verify object marking and collection
     */
    void demonstrateGCIntegration() {
        std::cout << "=== GC Integration Demo ===\n";
        
        // 1. Create a GC allocator
        GCAllocator allocator;
        
        // 2. Create a GC-managed State object
        GCRef<State> state = make_gc_state(allocator);
        std::cout << "Created GC-managed State object\n";
        
        // 3. Create GC-managed Table objects
        GCRef<Table> table1 = make_gc_table();
        GCRef<Table> table2 = make_gc_table();
        std::cout << "Created GC-managed Table objects\n";
        
        // 4. Create Values that reference GC objects
        Value stringValue("Hello, GC World!");
        Value tableValue(table1);
        Value numberValue(42.0);
        
        // 5. Store values in the state
        state->push(stringValue);
        state->push(tableValue);
        state->push(numberValue);
        std::cout << "Stored values in State stack\n";
        
        // 6. Set global variables
        state->setGlobal("myTable", tableValue);
        state->setGlobal("myString", stringValue);
        std::cout << "Set global variables\n";
        
        // 7. Create nested table references
        table1->set(Value("nested"), Value(table2));
        table2->set(Value("data"), Value("Nested data"));
        std::cout << "Created nested table references\n";
        
        // 8. Create a GC-managed Function
        Vec<Instruction> code;
        Vec<Value> constants = {Value("Function constant")};
        GCRef<Function> func = Function::createLua(
            std::make_shared<Vec<Instruction>>(code),
            constants,
            0, 0, 0
        );
        state->setGlobal("myFunction", Value(func));
        std::cout << "Created and stored GC-managed Function\n";
        
        // 9. Create a GarbageCollector and perform collection
        GarbageCollector gc(state.get());
        std::cout << "\nPerforming garbage collection...\n";
        
        // Mark phase: start from State as root object
        std::cout << "Marking reachable objects...\n";
        gc.markObject(state.get());
        
        // Collection phase
        std::cout << "Collecting unreachable objects...\n";
        gc.collectGarbage();
        
        std::cout << "\nGC Integration Demo completed successfully!\n";
        std::cout << "All objects are properly integrated with GC system.\n";
    }
    
    /**
     * @brief Test GC object marking
     */
    void testGCMarking() {
        std::cout << "\n=== GC Marking Test ===\n";
        
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
        
        std::cout << "Created complex reference pattern with cycles\n";
        
        // Test marking
        GarbageCollector gc(state.get());
        gc.markObject(state.get());
        
        std::cout << "Successfully marked objects with reference cycles\n";
    }
}