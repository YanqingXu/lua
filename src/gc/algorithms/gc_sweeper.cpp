#include "gc_sweeper.hpp"
#include "../core/gc_object.hpp"
#include "../core/string_pool.hpp"
#include "../core/gc_string.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../vm/function.hpp"
#include "../../vm/state.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>

namespace Lua {
    
    GCSweeper::GCSweeper(usize stepSize)
        : currentWhite(GCColor::White0)
        , nextWhite(GCColor::White1)
        , maxStepSize(stepSize)
        , currentPosition(nullptr) {
        stats.reset();
    }
    
    void GCSweeper::startSweep(GCObject* objectList, GCColor white) {
        currentWhite = white;
        nextWhite = flipWhite(white);
        currentPosition = objectList;
        stats.reset();
        finalizationQueue.clear();
    }
    
    GCObject* GCSweeper::sweepAll(GCObject* objectList, GCColor white) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        startSweep(objectList, white);
        
        GCObject* current = objectList;
        GCObject* prev = nullptr;
        GCObject* newHead = objectList;
        
        while (current != nullptr) {
            stats.objectsSwept++;
            
            if (shouldFreeObject(current)) {
                // Object should be freed
                GCObject* next = current->getNext();
                
                // Update list head if we're freeing the first object
                if (current == newHead) {
                    newHead = next;
                }
                
                // Add to finalization queue if needed
                if (current->needsFinalization()) {
                    addToFinalizationQueue(current);
                }
                
                // Remove from string pool if it's a string
                removeFromStringPool(current);
                
                // Free the object and update links
                current = freeObject(current, prev);
            } else {
                // Object should be kept - update its color for next cycle
                updateObjectColor(current);
                stats.objectsKept++;
                prev = current;
                current = current->getNext();
            }
        }
        
        // Process finalizers
        processFinalizers();
        
        // Flip colors for next cycle
        flipWhiteColors();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.sweepTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count();
        
        return newHead;
    }
    
    bool GCSweeper::sweepStep() {
        if (currentPosition == nullptr) {
            return true; // Sweep complete
        }
        
        usize processed = 0;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        while (currentPosition != nullptr && processed < maxStepSize) {
            GCObject* current = currentPosition;
            stats.objectsSwept++;
            processed++;
            
            if (shouldFreeObject(current)) {
                // Object should be freed
                GCObject* next = current->getNext();
                
                // Add to finalization queue if needed
                if (current->needsFinalization()) {
                    addToFinalizationQueue(current);
                }
                
                // Remove from string pool if it's a string
                removeFromStringPool(current);
                
                // Free the object
                freeObject(current, nullptr);
                currentPosition = next;
            } else {
                // Object should be kept - update its color for next cycle
                updateObjectColor(current);
                stats.objectsKept++;
                currentPosition = current->getNext();
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.sweepTimeUs += std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count();
        
        // Process some finalizers if we have time
        if (processed < maxStepSize && !finalizationQueue.empty()) {
            processFinalizers();
        }
        
        return currentPosition == nullptr;
    }
    
    bool GCSweeper::isSweepComplete() const {
        return currentPosition == nullptr;
    }
    
    void GCSweeper::reset() {
        currentPosition = nullptr;
        stats.reset();
        finalizationQueue.clear();
    }
    
    void GCSweeper::flipWhiteColors() {
        std::swap(currentWhite, nextWhite);
    }
    
    void GCSweeper::setFinalizerCallback(FinalizerCallback callback) {
        finalizerCallback = std::move(callback);
    }
    
    void GCSweeper::processFinalizers() {
        for (GCObject* object : finalizationQueue) {
            if (finalizerCallback) {
                finalizerCallback(object);
            }
            
            // Run object-specific finalizer
            object->finalize();
            stats.finalizersRun++;
        }
        
        finalizationQueue.clear();
    }
    
    bool GCSweeper::shouldFreeObject(GCObject* object) const {
        if (object == nullptr) {
            return false;
        }
        
        // Never free fixed objects
        if (object->isFixed()) {
            return false;
        }
        
        // Free white objects (unreachable)
        return isWhite(object, currentWhite);
    }
    
    GCObject* GCSweeper::freeObject(GCObject* object, GCObject* prev) {
        if (object == nullptr) {
            return nullptr;
        }
        
        GCObject* next = object->getNext();
        
        // Update statistics
        stats.objectsFreed++;
        stats.bytesFreed += calculateObjectSize(object);
        
        // Unlink from object list
        if (prev != nullptr) {
            prev->setNext(next);
        }
        if (next != nullptr) {
            next->setPrev(prev);
        }
        
        // Delete the object
        delete object;
        
        return next;
    }
    
    void GCSweeper::addToFinalizationQueue(GCObject* object) {
        if (object != nullptr && object->needsFinalization()) {
            finalizationQueue.push_back(object);
        }
    }
    
    void GCSweeper::removeFromStringPool(GCObject* object) {
        if (object != nullptr && object->getType() == GCObjectType::String) {
            // The string's destructor will automatically remove it from the pool
            // This is handled by GCString's destructor calling StringPool::remove
        }
    }
    
    void GCSweeper::updateObjectColor(GCObject* object) {
        if (object == nullptr) {
            return;
        }
        
        // Convert black objects to next white for next cycle
        if (object->getColor() == GCColor::Black) {
            object->setColor(nextWhite);
        }
        // Gray objects should not exist at sweep time, but handle them
        else if (object->getColor() == GCColor::Gray) {
            object->setColor(nextWhite);
        }
        // Current white objects that weren't freed become next white
        else if (object->getColor() == currentWhite) {
            object->setColor(nextWhite);
        }
    }
    
    usize GCSweeper::calculateObjectSize(GCObject* object) const {
        if (object == nullptr) {
            return 0;
        }
        
        usize baseSize = object->getSize();
        usize additionalSize = object->getAdditionalSize();
        
        return baseSize + additionalSize;
    }
    
    // Helper function implementations
    
    namespace {
        /**
         * @brief Specialized finalizer for different object types
         */
        void runObjectFinalizer(GCObject* object) {
            if (object == nullptr) {
                return;
            }
            
            switch (object->getType()) {
                case GCObjectType::String:
                    // Strings typically don't need special finalization
                    // String pool removal is handled automatically
                    break;
                    
                case GCObjectType::Table: {
                    // Tables might need to clear weak references
                    Table* table = static_cast<Table*>(object);
                    table->clearWeakReferences();
                    break;
                }
                
                case GCObjectType::Function: {
                    // Functions might need to close upvalues
                    Function* func = static_cast<Function*>(object);
                    func->closeUpvalues();
                    break;
                }
                
                case GCObjectType::Userdata:
                    // Userdata finalizers are handled by the callback
                    break;
                    
                case GCObjectType::Thread: {
                    // Threads need to clean up their stack and state
                    // This would be handled by the State destructor
                    break;
                }
                
                case GCObjectType::Proto:
                    // Function prototypes typically don't need finalization
                    break;
            }
        }
    }
    
    /**
     * @brief Create a default sweeper with standard configuration
     */
    std::unique_ptr<GCSweeper> createDefaultSweeper() {
        auto sweeper = std::make_unique<GCSweeper>(1024); // 1KB step size
        
        // Set up default finalizer
        sweeper->setFinalizerCallback([](GCObject* object) {
            runObjectFinalizer(object);
        });
        
        return sweeper;
    }
    
    /**
     * @brief Create a sweeper optimized for incremental collection
     */
    std::unique_ptr<GCSweeper> createIncrementalSweeper(usize stepSize) {
        auto sweeper = std::make_unique<GCSweeper>(stepSize);
        
        // Set up finalizer for incremental mode
        sweeper->setFinalizerCallback([](GCObject* object) {
            runObjectFinalizer(object);
        });
        
        return sweeper;
    }
    
    /**
     * @brief Sweep statistics formatter
     */
    std::string formatSweepStats(const GCSweeper::SweepStats& stats) {
        std::string result;
        result += "Sweep Statistics:\n";
        result += "  Objects swept: " + std::to_string(stats.objectsSwept) + "\n";
        result += "  Objects freed: " + std::to_string(stats.objectsFreed) + "\n";
        result += "  Objects kept: " + std::to_string(stats.objectsKept) + "\n";
        result += "  Bytes freed: " + std::to_string(stats.bytesFreed) + "\n";
        result += "  Finalizers run: " + std::to_string(stats.finalizersRun) + "\n";
        result += "  Sweep time: " + std::to_string(stats.sweepTimeUs) + " μs\n";
        
        if (stats.objectsSwept > 0) {
            double freeRate = (double)stats.objectsFreed / stats.objectsSwept * 100.0;
            result += "  Free rate: " + std::to_string(freeRate) + "%\n";
        }
        
        return result;
    }
}