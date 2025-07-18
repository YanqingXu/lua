#include "gc_marker.hpp"
#include "../core/gc_object.hpp"
#include "../core/string_pool.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../vm/table_impl.hpp"
#include "../../vm/function.hpp"
#include "../../vm/state.hpp"
#include <algorithm>
#include <cassert>

namespace Lua {
    
    void GCMarker::markFromRoots(const std::vector<GCObject*>& rootObjects, GCColor currentWhite) {
        // Reset statistics for new marking cycle
        markedObjectCount = 0;
        maxGrayStackSize = 0;
        
        // Mark all root objects
        for (GCObject* root : rootObjects) {
            if (root != nullptr) {
                markObject(root, currentWhite);
            }
        }
        
        // Mark all strings in the string pool as reachable
        // This ensures that interned strings are not collected
        // even if they are temporarily not referenced by other objects
        StringPool& stringPool = StringPool::getInstance();
        for (GCObject* str : stringPool.getAllStrings()) {
            if (str != nullptr) {
                markObject(str, currentWhite);
            }
        }
        
        // Process all gray objects until none remain
        processGrayObjects(currentWhite);
    }
    
    void GCMarker::markObject(GCObject* object, GCColor currentWhite) {
        if (object == nullptr || !isWhite(object, currentWhite)) {
            return;
        }
        
        // Mark object as gray and add to processing stack
        setGray(object);
        addToGrayStack(object);
        markedObjectCount++;
    }
    
    void GCMarker::processGrayObjects(GCColor currentWhite) {
        while (!grayStack.empty()) {
            // Pop a gray object from the stack
            GCObject* object = grayStack.back();
            grayStack.pop_back();
            graySet.erase(object);
            
            // Mark object as black (fully processed)
            setBlack(object);
            
            // Mark all children of this object
            markChildren(object, currentWhite);
        }
    }
    
    bool GCMarker::isMarkingComplete() const {
        return grayStack.empty();
    }
    
    void GCMarker::reset() {
        grayStack.clear();
        graySet.clear();
        markedObjectCount = 0;
        maxGrayStackSize = 0;
    }
    
    void GCMarker::addToGrayStack(GCObject* object) {
        // Avoid adding the same object multiple times
        if (graySet.find(object) == graySet.end()) {
            grayStack.push_back(object);
            graySet.insert(object);
            
            // Update maximum stack size statistics
            if (grayStack.size() > maxGrayStackSize) {
                maxGrayStackSize = grayStack.size();
            }
        }
    }
    
    void GCMarker::markChildren(GCObject* object, GCColor currentWhite) {
        if (object == nullptr) {
            return;
        }
        
        // Mark children based on object type
        switch (object->getType()) {
            case GCObjectType::String:
                // Strings have no children to mark
                break;
                
            case GCObjectType::Table: {
                // Mark all keys and values in the table
                Table* table = static_cast<Table*>(object);
                
                // Mark array part
                for (usize i = 0; i < table->getArraySize(); ++i) {
                    const Value& value = table->getArrayElement(i);
                    if (value.isGCObject()) {
                        markObject(value.asGCObject(), currentWhite);
                    }
                }
                
                // Mark hash part
                table->forEachHashEntry([this, currentWhite](const Value& key, const Value& value) {
                    if (key.isGCObject()) {
                        markObject(key.asGCObject(), currentWhite);
                    }
                    if (value.isGCObject()) {
                        markObject(value.asGCObject(), currentWhite);
                    }
                });
                
                // Mark metatable if present
                auto metatable = table->getMetatable();
                if (metatable) {
                    markObject(metatable.get(), currentWhite);
                }
                break;
            }
            
            case GCObjectType::Function: {
                // Mark function's upvalues and constants
                Function* func = static_cast<Function*>(object);
                
                // Mark upvalues
                for (usize i = 0; i < func->getUpvalueCount(); ++i) {
                    GCRef<Upvalue> upvalue = func->getUpvalue(i);
                    if (upvalue) {
                        markObject(upvalue.get(), currentWhite);
                    }
                }
                
                // Mark constants
                for (usize i = 0; i < func->getConstantCount(); ++i) {
                    const Value& constant = func->getConstant(i);
                    if (constant.isGCObject()) {
                        markObject(constant.asGCObject(), currentWhite);
                    }
                }
                
                // Mark function prototype if present
                if (func->getPrototype() != nullptr) {
                    markObject(func->getPrototype(), currentWhite);
                }
                break;
            }
            
            case GCObjectType::Userdata: {
                // Mark userdata's metatable if present
                // Note: Userdata implementation details depend on your specific design
                // This is a placeholder for userdata marking logic
                break;
            }
            
            case GCObjectType::Thread: {
                // Mark thread's stack and other references
                // Note: Thread implementation details depend on your specific design
                // This is a placeholder for thread marking logic
                break;
            }
            
            case GCObjectType::Proto: {
                // Mark function prototype's constants and nested prototypes
                // Note: Proto implementation details depend on your specific design
                // This is a placeholder for prototype marking logic
                break;
            }
            
            case GCObjectType::State: {
                // Mark Lua state's stack and global variables
                State* state = static_cast<State*>(object);
                // Note: State marking should be handled by the GarbageCollector
                // For now, we'll mark the state's references manually
                // TODO: Refactor to use proper GarbageCollector interface
                break;
            }
            
            default:
                // Unknown object type - should not happen
                assert(false && "Unknown GC object type encountered during marking");
                break;
        }
    }
    
    bool GCMarker::isWhite(GCObject* object, GCColor currentWhite) const {
        if (object == nullptr) {
            return false;
        }
        
        GCColor objectColor = object->getColor();
        
        // An object is white if its color matches the current white color
        return (objectColor == currentWhite);
    }
    
    void GCMarker::setGray(GCObject* object) {
        if (object != nullptr) {
            object->setColor(GCColor::Gray);
        }
    }
    
    void GCMarker::setBlack(GCObject* object) {
        if (object != nullptr) {
            object->setColor(GCColor::Black);
        }
    }
    
} // namespace Lua