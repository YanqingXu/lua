#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "gc/garbage_collector.hpp"
#include <iostream>

using namespace Lua;

int main() {
    GarbageCollector& gc = GarbageCollector::getInstance();
    gc.clearAll();
    
    std::cout << "Initial object count: " << gc.getObjectCount() << std::endl;
    
    GCString* gcStr1 = new GCString("GC Test 1");
    std::cout << "After creating gcStr1: " << gc.getObjectCount() << std::endl;
    
    GCString* gcStr2 = new GCString("GC Test 2");
    std::cout << "After creating gcStr2: " << gc.getObjectCount() << std::endl;
    
    Table* gcTable = new Table();
    std::cout << "After creating gcTable: " << gc.getObjectCount() << std::endl;
    
    // Register objects
    gc.registerObject(gcStr1);
    std::cout << "After registering gcStr1: " << gc.getObjectCount() << std::endl;
    
    gc.registerObject(gcStr2);
    std::cout << "After registering gcStr2: " << gc.getObjectCount() << std::endl;
    
    gc.registerObject(gcTable);
    std::cout << "After registering gcTable: " << gc.getObjectCount() << std::endl;
    
    gc.clearAll();
    
    return 0;
}

