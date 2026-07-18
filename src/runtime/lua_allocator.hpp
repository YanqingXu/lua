#pragma once

/**
 * @file lua_allocator.hpp
 * @brief Mutable Lua 5.1 allocator callback and user-data ownership.
 */

#include <cstddef>
#include <cstdlib>
#include <array>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Lua {

namespace AllocatorDetail {

inline std::mutex& implementationAllocationMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::vector<void*>& implementationAllocations() {
    static std::vector<void*> allocations;
    return allocations;
}

inline void trackImplementationAllocation(void* pointer) {
    std::scoped_lock lock(implementationAllocationMutex());
    implementationAllocations().push_back(pointer);
}

inline bool releaseImplementationAllocation(void* pointer) noexcept {
    std::scoped_lock lock(implementationAllocationMutex());
    auto& allocations = implementationAllocations();
    for (auto it = allocations.begin(); it != allocations.end(); ++it) {
        if (*it == pointer) {
            allocations.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace AllocatorDetail

using LuaAllocatorFunction = void* (*)(void* userData, void* pointer, std::size_t oldSize, std::size_t newSize);

class LuaAllocator {
public:
    LuaAllocator() = default;

    LuaAllocator(LuaAllocatorFunction function, void* userData) noexcept : function_(function), userData_(userData) {}

    [[nodiscard]] bool isConfigured() const noexcept {
        return function_ != nullptr;
    }

    [[nodiscard]] void* allocate(std::size_t size) const noexcept {
        try {
            return function_ != nullptr ? function_(userData_, nullptr, 0, size) : nullptr;
        } catch (...) {
            return nullptr;
        }
    }

    [[nodiscard]] void* reallocate(void* pointer, std::size_t oldSize, std::size_t newSize) const noexcept {
        try {
            return function_ != nullptr ? function_(userData_, pointer, oldSize, newSize) : nullptr;
        } catch (...) {
            return nullptr;
        }
    }

    void deallocate(void* pointer, std::size_t oldSize) const noexcept {
        if (AllocatorDetail::releaseImplementationAllocation(pointer)) {
            ::operator delete(pointer);
            return;
        }
        if (function_ != nullptr && pointer != nullptr) {
            try {
                (void)function_(userData_, pointer, oldSize, 0);
            } catch (...) {
                // Destruction and rollback are closed exception boundaries.
            }
        }
    }

    void set(LuaAllocatorFunction function, void* userData) noexcept {
        function_ = function;
        userData_ = userData;
    }

    [[nodiscard]] LuaAllocatorFunction getFunction() const noexcept {
        return function_;
    }

    [[nodiscard]] void* getUserData() const noexcept {
        return userData_;
    }

private:
    LuaAllocatorFunction function_ = nullptr;
    void* userData_ = nullptr;
};

template <typename T> class LuaStdAllocator {
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    LuaStdAllocator() noexcept = default;
    explicit LuaStdAllocator(LuaAllocator* allocator) noexcept : allocator_(allocator) {}

    template <typename U>
    LuaStdAllocator(const LuaStdAllocator<U>& other) noexcept : allocator_(other.getLuaAllocator()) {}

    [[nodiscard]] T* allocate(std::size_t count) {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }
        if (count == 0) {
            // lua_Alloc reserves nsize == 0 for deallocation, while standard
            // containers are permitted to request a zero-sized allocation.
            return std::allocator<T>{}.allocate(count);
        }
        if (isImplementationMetadata()) {
            // MSVC Debug rebinds user allocators for _Container_proxy. Keep
            // that implementation-only block outside Lua's failure contract,
            // while tracking it because destruction may arrive through a
            // differently rebound allocator specialization.
            T* memory = static_cast<T*>(::operator new(count * sizeof(T)));
            if (allocator_ != nullptr && allocator_->isConfigured()) {
                try {
                    AllocatorDetail::trackImplementationAllocation(memory);
                } catch (...) {
                    ::operator delete(memory);
                    throw;
                }
            }
            return memory;
        }

        if (allocator_ == nullptr || !allocator_->isConfigured()) {
            return std::allocator<T>{}.allocate(count);
        }

        void* memory = allocator_->allocate(count * sizeof(T));
        if (memory == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(memory);
    }

    void deallocate(T* pointer, std::size_t count) noexcept {
        if (pointer == nullptr) {
            return;
        }
        const bool implementationAllocation = AllocatorDetail::releaseImplementationAllocation(pointer);
        if (implementationAllocation) {
            ::operator delete(pointer);
            return;
        }
        if (count == 0 || isImplementationMetadata()) {
            std::allocator<T>{}.deallocate(pointer, count);
            return;
        }
        if (allocator_ == nullptr || !allocator_->isConfigured()) {
            std::allocator<T>{}.deallocate(pointer, count);
            return;
        }
        allocator_->deallocate(pointer, count * sizeof(T));
    }

    [[nodiscard]] LuaAllocator* getLuaAllocator() const noexcept {
        return allocator_;
    }

    template <typename U> bool operator==(const LuaStdAllocator<U>& other) const noexcept {
        return allocator_ == other.getLuaAllocator();
    }

    template <typename U> bool operator!=(const LuaStdAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

private:
    static constexpr bool isImplementationMetadata() noexcept {
#ifdef _MSC_VER
        return std::is_same_v<std::remove_cv_t<T>, std::_Container_proxy>;
#else
        return false;
#endif
    }

    LuaAllocator* allocator_ = nullptr;
};

/**
 * Standard allocator that snapshots a callback/userdata pair by value.
 *
 * This is intended for compiler temporaries that may be copied or moved out
 * of their producing object. Each rebound allocator remains able to release
 * its storage without depending on a LuaAllocator object's lifetime.
 */
template <typename T> class LuaSnapshotStdAllocator {
public:
    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    LuaSnapshotStdAllocator() noexcept = default;
    explicit LuaSnapshotStdAllocator(const LuaAllocator* allocator) noexcept
        : allocator_(allocator != nullptr ? *allocator : LuaAllocator{}) {}

    template <typename U>
    LuaSnapshotStdAllocator(const LuaSnapshotStdAllocator<U>& other) noexcept : allocator_(other.getLuaAllocator()) {}

    [[nodiscard]] T* allocate(std::size_t count) {
        return LuaStdAllocator<T>(&allocator_).allocate(count);
    }

    void deallocate(T* pointer, std::size_t count) noexcept {
        LuaStdAllocator<T>(&allocator_).deallocate(pointer, count);
    }

    [[nodiscard]] const LuaAllocator& getLuaAllocator() const noexcept {
        return allocator_;
    }

    template <typename U> bool operator==(const LuaSnapshotStdAllocator<U>& other) const noexcept {
        const LuaAllocator& rhs = other.getLuaAllocator();
        return allocator_.getFunction() == rhs.getFunction() && allocator_.getUserData() == rhs.getUserData();
    }

    template <typename U> bool operator!=(const LuaSnapshotStdAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

private:
    LuaAllocator allocator_;
};

template <typename T> using LuaVector = std::vector<T, LuaStdAllocator<T>>;
template <typename T> using LuaOwnedVector = std::vector<T, LuaSnapshotStdAllocator<T>>;

template <typename T> class LuaOwnedObjectDeleter {
public:
    LuaOwnedObjectDeleter() noexcept = default;
    explicit LuaOwnedObjectDeleter(const LuaAllocator* allocator) noexcept : allocator_(allocator) {}

    void operator()(T* pointer) noexcept {
        if (pointer == nullptr) {
            return;
        }
        std::destroy_at(pointer);
        allocator_.deallocate(pointer, 1);
    }

private:
    LuaSnapshotStdAllocator<T> allocator_;
};

template <typename T> using LuaOwnedPtr = std::unique_ptr<T, LuaOwnedObjectDeleter<T>>;

template <typename T, typename... Args>
[[nodiscard]] LuaOwnedPtr<T> makeLuaOwned(const LuaAllocator* allocator, Args&&... args) {
    LuaSnapshotStdAllocator<T> storage(allocator);
    T* memory = storage.allocate(1);
    try {
        std::construct_at(memory, std::forward<Args>(args)...);
    } catch (...) {
        storage.deallocate(memory, 1);
        throw;
    }
    return LuaOwnedPtr<T>(memory, LuaOwnedObjectDeleter<T>(allocator));
}

/**
 * Mutable string with a fixed inline buffer and exact allocator byte counts.
 *
 * Some standard-library basic_string implementations can request N bytes from
 * a user allocator and later deallocate the block with N-1. lua_Alloc requires
 * the original block size, so runtime/compiler strings use this small wrapper
 * instead of inheriting implementation-specific SSO/capacity bookkeeping.
 */
template <typename Allocator> class LuaBasicString {
public:
    using value_type = char;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using iterator = char*;
    using const_iterator = const char*;

    LuaBasicString() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) = default;

    explicit LuaBasicString(const allocator_type& allocator) noexcept : allocator_(allocator) {}

    template <typename InputIt>
    LuaBasicString(InputIt first, InputIt last, const allocator_type& allocator = allocator_type())
        : allocator_(allocator) {
        assignRange(first, last);
    }

    LuaBasicString(const LuaBasicString& other) : allocator_(other.allocator_) {
        assignRange(other.begin(), other.end());
    }

    LuaBasicString(LuaBasicString&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
        : allocator_(std::move(other.allocator_)) {
        takeStorage(other);
    }

    LuaBasicString& operator=(const LuaBasicString& other) {
        if (this != &other) {
            LuaBasicString replacement(other);
            *this = std::move(replacement);
        }
        return *this;
    }

    LuaBasicString& operator=(LuaBasicString&& other) noexcept(std::is_nothrow_move_assignable_v<allocator_type>) {
        if (this != &other) {
            releaseExternal();
            allocator_ = std::move(other.allocator_);
            resetInline();
            takeStorage(other);
        }
        return *this;
    }

    ~LuaBasicString() {
        releaseExternal();
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept {
        return allocator_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] size_type size() const noexcept {
        return size_;
    }

    [[nodiscard]] size_type capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] char* data() noexcept {
        return data_;
    }

    [[nodiscard]] const char* data() const noexcept {
        return data_;
    }

    [[nodiscard]] const char* c_str() const noexcept {
        return data_;
    }

    [[nodiscard]] iterator begin() noexcept {
        return data_;
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return data_;
    }

    [[nodiscard]] iterator end() noexcept {
        return data_ + size_;
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return data_ + size_;
    }

    char& operator[](size_type index) noexcept {
        return data_[index];
    }

    const char& operator[](size_type index) const noexcept {
        return data_[index];
    }

    void clear() noexcept {
        size_ = 0;
        data_[0] = '\0';
    }

    void reserve(size_type requestedCapacity) {
        ensureCapacity(requestedCapacity);
    }

    void resize(size_type requestedSize, char fill = '\0') {
        if (requestedSize <= size_) {
            size_ = requestedSize;
            data_[size_] = '\0';
            return;
        }
        ensureCapacity(requestedSize);
        std::memset(data_ + size_, static_cast<unsigned char>(fill), requestedSize - size_);
        size_ = requestedSize;
        data_[size_] = '\0';
    }

    void push_back(char value) {
        ensureCapacity(size_ + 1);
        data_[size_++] = value;
        data_[size_] = '\0';
    }

    LuaBasicString& operator+=(char value) {
        push_back(value);
        return *this;
    }

    void append(const char* text, size_type count) {
        if (count == 0) {
            return;
        }
        if (text == nullptr || size_ > std::numeric_limits<size_type>::max() - count) {
            throw std::bad_array_new_length();
        }
        ensureCapacity(size_ + count);
        std::memcpy(data_ + size_, text, count);
        size_ += count;
        data_[size_] = '\0';
    }

    void append(size_type count, char value) {
        if (count == 0) {
            return;
        }
        if (size_ > std::numeric_limits<size_type>::max() - count) {
            throw std::bad_array_new_length();
        }
        ensureCapacity(size_ + count);
        std::memset(data_ + size_, static_cast<unsigned char>(value), count);
        size_ += count;
        data_[size_] = '\0';
    }

    template <typename InputIt> void assign(InputIt first, InputIt last) {
        LuaBasicString replacement(first, last, allocator_);
        *this = std::move(replacement);
    }

    friend bool operator==(const LuaBasicString& lhs, const LuaBasicString& rhs) noexcept {
        return lhs.size_ == rhs.size_ && (lhs.size_ == 0 || std::memcmp(lhs.data_, rhs.data_, lhs.size_) == 0);
    }

    friend bool operator!=(const LuaBasicString& lhs, const LuaBasicString& rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    static constexpr size_type kInlineCapacity = 23;

    [[nodiscard]] bool usesExternalStorage() const noexcept {
        return data_ != inlineData_.data();
    }

    void ensureCapacity(size_type requestedCapacity) {
        if (requestedCapacity <= capacity_) {
            return;
        }
        size_type nextCapacity = capacity_;
        while (nextCapacity < requestedCapacity) {
            if (nextCapacity > (std::numeric_limits<size_type>::max() - 1) / 2) {
                nextCapacity = requestedCapacity;
                break;
            }
            nextCapacity *= 2;
        }
        replaceStorage(nextCapacity);
    }

    void replaceStorage(size_type requestedCapacity) {
        if (requestedCapacity >= std::numeric_limits<size_type>::max()) {
            throw std::bad_array_new_length();
        }
        char* replacement = allocator_.allocate(requestedCapacity + 1);
        if (replacement == nullptr) {
            throw std::bad_alloc();
        }
        std::memcpy(replacement, data_, size_ + 1);
        releaseExternal();
        data_ = replacement;
        capacity_ = requestedCapacity;
    }

    template <typename InputIt> void assignRange(InputIt first, InputIt last) {
        const auto distance = std::distance(first, last);
        if (distance < 0) {
            throw std::bad_array_new_length();
        }
        const size_type count = static_cast<size_type>(distance);
        if (count > kInlineCapacity) {
            replaceStorage(count);
        }
        for (; first != last; ++first) {
            data_[size_++] = static_cast<char>(*first);
        }
        data_[size_] = '\0';
    }

    void releaseExternal() noexcept {
        if (usesExternalStorage()) {
            allocator_.deallocate(data_, capacity_ + 1);
        }
    }

    void resetInline() noexcept {
        data_ = inlineData_.data();
        size_ = 0;
        capacity_ = kInlineCapacity;
        inlineData_[0] = '\0';
    }

    void takeStorage(LuaBasicString& other) noexcept {
        if (other.usesExternalStorage()) {
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.resetInline();
            return;
        }
        size_ = other.size_;
        std::memcpy(inlineData_.data(), other.inlineData_.data(), size_ + 1);
        data_ = inlineData_.data();
        capacity_ = kInlineCapacity;
        other.resetInline();
    }

    allocator_type allocator_{};
    std::array<char, kInlineCapacity + 1> inlineData_{};
    char* data_ = inlineData_.data();
    size_type size_ = 0;
    size_type capacity_ = kInlineCapacity;
};

using LuaString = LuaBasicString<LuaStdAllocator<char>>;
using LuaOwnedString = LuaBasicString<LuaSnapshotStdAllocator<char>>;

/**
 * Contiguous storage for trivially copyable runtime records.
 *
 * Unlike std::vector's allocator protocol, growth uses lua_Alloc's real
 * realloc form. A failed growth therefore leaves the old block, capacity,
 * size, and elements unchanged, matching the Lua 5.1 allocator contract.
 */
template <typename T> class LuaReallocVector {
    static_assert(std::is_trivially_copyable_v<T>,
                  "LuaReallocVector requires trivially copyable elements so realloc may relocate them");
    static_assert(std::is_trivially_destructible_v<T>, "LuaReallocVector requires trivially destructible elements");

public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    LuaReallocVector() noexcept = default;
    explicit LuaReallocVector(LuaAllocator* allocator) noexcept : allocator_(allocator) {}

    LuaReallocVector(const LuaReallocVector&) = delete;
    LuaReallocVector& operator=(const LuaReallocVector&) = delete;

    LuaReallocVector(LuaReallocVector&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)), allocator_(std::exchange(other.allocator_, nullptr)) {}

    LuaReallocVector& operator=(LuaReallocVector&& other) noexcept {
        if (this != &other) {
            release();
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
            allocator_ = std::exchange(other.allocator_, nullptr);
        }
        return *this;
    }

    ~LuaReallocVector() {
        release();
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] T* data() noexcept {
        return data_;
    }

    [[nodiscard]] const T* data() const noexcept {
        return data_;
    }

    [[nodiscard]] iterator begin() noexcept {
        return data_;
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return data_;
    }

    [[nodiscard]] iterator end() noexcept {
        return data_ == nullptr ? nullptr : data_ + size_;
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return data_ == nullptr ? nullptr : data_ + size_;
    }

    T& operator[](std::size_t index) noexcept {
        return data_[index];
    }

    const T& operator[](std::size_t index) const noexcept {
        return data_[index];
    }

    void reserve(std::size_t requestedCapacity) {
        if (requestedCapacity <= capacity_) {
            return;
        }
        if (requestedCapacity > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        const std::size_t oldBytes = capacity_ * sizeof(T);
        const std::size_t newBytes = requestedCapacity * sizeof(T);
        void* replacement = nullptr;
        if (allocator_ != nullptr && allocator_->isConfigured()) {
            replacement = allocator_->reallocate(data_, oldBytes, newBytes);
        } else {
            replacement = std::realloc(data_, newBytes);
        }
        if (replacement == nullptr) {
            throw std::bad_alloc();
        }

        data_ = static_cast<T*>(replacement);
        capacity_ = requestedCapacity;
    }

    void resize(std::size_t requestedSize) {
        resize(requestedSize, T{});
    }

    void resize(std::size_t requestedSize, const T& fillValue) {
        if (requestedSize <= size_) {
            size_ = requestedSize;
            return;
        }

        T stableFill = fillValue;
        ensureCapacity(requestedSize);
        for (; size_ < requestedSize; ++size_) {
            std::construct_at(data_ + size_, stableFill);
        }
    }

    void push_back(const T& value) {
        T stableValue = value;
        ensureCapacity(size_ + 1);
        std::construct_at(data_ + size_, stableValue);
        ++size_;
    }

    template <typename... Args> T& emplace_back(Args&&... args) {
        T value(std::forward<Args>(args)...);
        push_back(value);
        return data_[size_ - 1];
    }

    void pop_back() noexcept {
        if (size_ != 0) {
            --size_;
        }
    }

    void clear() noexcept {
        size_ = 0;
    }

private:
    void ensureCapacity(std::size_t requestedSize) {
        if (requestedSize <= capacity_) {
            return;
        }
        std::size_t nextCapacity = capacity_ == 0 ? 1 : capacity_;
        while (nextCapacity < requestedSize) {
            if (nextCapacity > std::numeric_limits<std::size_t>::max() / 2) {
                nextCapacity = requestedSize;
                break;
            }
            nextCapacity *= 2;
        }
        reserve(nextCapacity);
    }

    void release() noexcept {
        if (data_ == nullptr) {
            return;
        }
        const std::size_t bytes = capacity_ * sizeof(T);
        if (allocator_ != nullptr && allocator_->isConfigured()) {
            allocator_->deallocate(data_, bytes);
        } else {
            std::free(data_);
        }
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
    LuaAllocator* allocator_ = nullptr;
};

} // namespace Lua
