#include "call_stack.hpp"
#include "global_state.hpp"
#include "../common/defines.hpp"
#include <algorithm>
#include <iostream>

namespace Lua {
    
    CallStack::CallStack(LuaState* L, i32 initialSize) 
        : L_(L), base_(nullptr), end_(nullptr), current_(nullptr), 
          size_(0), maxDepthReached_(0), resizeCount_(0) {
        
        if (!L_) {
            throw LuaException("LuaState cannot be null");
        }
        
        // Ensure initial size is within bounds
        initialSize = std::max(MIN_STACK_SIZE, std::min(initialSize, MAX_STACK_SIZE));
        
        // Allocate initial array
        base_ = allocateArray_(initialSize);
        size_ = initialSize;
        end_ = base_ + size_;
        current_ = base_;
        
        // Initialize base call frame
        current_->reset();
        current_->setFresh();
    }
    
    CallStack::~CallStack() {
        if (base_) {
            deallocateArray_(base_);
        }
    }
    
    // Stack operations
    CallInfo* CallStack::push() {
        // Check if we need to grow
        if (isFull()) {
            ensureCapacity(1);
        }
        
        // Move to next frame
        current_++;
        
        // Initialize new frame
        current_->reset();
        current_->setFresh();
        
        // Update statistics
        i32 depth = getDepth();
        maxDepthReached_ = std::max(maxDepthReached_, depth);
        
        return current_;
    }
    
    void CallStack::pop() {
        if (isEmpty()) {
            throw LuaException("Cannot pop from empty call stack");
        }
        
        // Clear current frame
        current_->reset();
        
        // Move back to previous frame
        current_--;
    }
    
    CallInfo* CallStack::getFrame(i32 level) const {
        if (level < 0) {
            return nullptr;
        }
        
        CallInfo* frame = current_;
        for (i32 i = 0; i < level && frame > base_; i++) {
            frame--;
        }
        
        return (frame >= base_) ? frame : nullptr;
    }
    
    // Stack information
    i32 CallStack::getDepth() const {
        return static_cast<i32>(current_ - base_);
    }
    
    f64 CallStack::getUtilization() const {
        if (size_ == 0) {
            return 0.0;
        }
        return static_cast<f64>(getDepth() + 1) / static_cast<f64>(size_);
    }
    
    // Memory management
    void CallStack::resize(i32 newSize) {
        if (newSize < MIN_STACK_SIZE || newSize > MAX_STACK_SIZE) {
            throw LuaException("Invalid call stack size");
        }
        
        i32 currentDepth = getDepth();
        if (newSize <= currentDepth) {
            throw LuaException("Cannot resize call stack smaller than current depth");
        }
        
        // Allocate new array
        CallInfo* newBase = allocateArray_(newSize);
        
        // Copy existing data
        copyCallInfos_(newBase, base_, currentDepth + 1);
        
        // Update pointers
        CallInfo* oldBase = base_;
        updatePointers_(oldBase, newBase);
        
        // Update size
        size_ = newSize;
        end_ = base_ + size_;
        
        // Deallocate old array
        deallocateArray_(oldBase);
        
        resizeCount_++;
    }
    
    void CallStack::shrink() {
        i32 currentDepth = getDepth();
        i32 optimalSize = calculateOptimalSize_(currentDepth);
        
        // Only shrink if it would save significant memory
        if (optimalSize < size_ && getUtilization() < SHRINK_THRESHOLD) {
            resize(optimalSize);
        }
    }
    
    void CallStack::ensureCapacity(i32 additionalFrames) {
        i32 requiredSize = getDepth() + additionalFrames + 1;
        if (requiredSize > size_) {
            i32 newSize = calculateOptimalSize_(requiredSize);
            resize(newSize);
        }
    }
    
    // Validation and debugging
    bool CallStack::validate() const {
        if (!base_ || !current_ || !end_) {
            return false;
        }
        
        if (current_ < base_ || current_ >= end_) {
            return false;
        }
        
        if (size_ <= 0 || size_ > MAX_STACK_SIZE) {
            return false;
        }
        
        // Validate each frame
        for (CallInfo* frame = base_; frame <= current_; frame++) {
            if (!frame->isValid()) {
                return false;
            }
        }
        
        return true;
    }
    
    void CallStack::dumpStack(i32 maxFrames) const {
        std::cout << "=== Call Stack Dump ===" << std::endl;
        std::cout << "Depth: " << getDepth() << ", Size: " << size_ << std::endl;
        std::cout << "Utilization: " << (getUtilization() * 100.0) << "%" << std::endl;
        
        i32 framesToShow = std::min(maxFrames, getDepth() + 1);
        for (i32 i = 0; i < framesToShow; i++) {
            CallInfo* frame = getFrame(i);
            if (frame) {
                std::cout << "Frame[" << i << "]: ";
                std::cout << "base=" << frame->base << " ";
                std::cout << "func=" << frame->func << " ";
                std::cout << "top=" << frame->top << " ";
                std::cout << "nresults=" << frame->nresults << " ";
                std::cout << "tailcalls=" << frame->tailcalls << " ";
                std::cout << "status=0x" << std::hex << static_cast<u32>(frame->callstatus) << std::dec;
                std::cout << std::endl;
            }
        }
        std::cout << "======================" << std::endl;
    }
    
    void CallStack::printStatistics() const {
        std::cout << "=== Call Stack Statistics ===" << std::endl;
        std::cout << "Current depth: " << getDepth() << std::endl;
        std::cout << "Maximum depth reached: " << maxDepthReached_ << std::endl;
        std::cout << "Stack size: " << size_ << std::endl;
        std::cout << "Utilization: " << (getUtilization() * 100.0) << "%" << std::endl;
        std::cout << "Resize count: " << resizeCount_ << std::endl;
        std::cout << "Memory usage: " << getMemoryUsage() << " bytes" << std::endl;
        std::cout << "=============================" << std::endl;
    }
    
    // Performance optimization
    void CallStack::reset() {
        // Reset to base frame only
        current_ = base_;
        current_->reset();
        current_->setFresh();
        
        // Reset statistics
        maxDepthReached_ = 0;
    }
    
    usize CallStack::getMemoryUsage() const {
        return size_ * sizeof(CallInfo);
    }
    
    // Private helper methods
    CallInfo* CallStack::allocateArray_(i32 size) {
        if (!L_ || !L_->getGlobalState()) {
            throw LuaException("Invalid LuaState for CallStack allocation");
        }
        
        CallInfo* array = static_cast<CallInfo*>(
            L_->getGlobalState()->allocate(size * sizeof(CallInfo))
        );
        
        if (!array) {
            throw LuaException("Cannot allocate CallInfo array");
        }
        
        // Initialize array
        for (i32 i = 0; i < size; i++) {
            new (array + i) CallInfo();
        }
        
        return array;
    }
    
    void CallStack::deallocateArray_(CallInfo* array) {
        if (array && L_ && L_->getGlobalState()) {
            // Call destructors
            for (i32 i = 0; i < size_; i++) {
                (array + i)->~CallInfo();
            }
            
            L_->getGlobalState()->deallocate(array);
        }
    }
    
    void CallStack::copyCallInfos_(CallInfo* dest, const CallInfo* src, i32 count) {
        for (i32 i = 0; i < count; i++) {
            dest[i] = src[i];
        }
    }
    
    void CallStack::updatePointers_(CallInfo* oldBase, CallInfo* newBase) {
        i32 offset = static_cast<i32>(current_ - oldBase);
        
        base_ = newBase;
        current_ = base_ + offset;
    }
    
    i32 CallStack::calculateOptimalSize_(i32 requiredDepth) const {
        i32 optimalSize = static_cast<i32>(requiredDepth * GROW_FACTOR) + 1;
        return std::max(MIN_STACK_SIZE, std::min(optimalSize, MAX_STACK_SIZE));
    }
    
} // namespace Lua
