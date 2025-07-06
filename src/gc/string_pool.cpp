#include "string_pool.hpp"
#include "core/gc_string.hpp"

namespace Lua {

StringPool& StringPool::getInstance() {
    static StringPool instance;
    return instance;
}

const std::string& StringPool::intern(const std::string& str) {
    auto result = pool_.insert(str);
    return *result.first;
}

const std::string& StringPool::intern(const char* str) {
    return intern(std::string(str));
}

const std::string& StringPool::intern(std::string&& str) {
    auto result = pool_.insert(std::move(str));
    return *result.first;
}

void StringPool::remove(GCString* str) {
    // Stub implementation - in real version would remove string from pool
    (void)str;
}

std::vector<std::string> StringPool::getAllStrings() const {
    std::vector<std::string> result;
    result.reserve(pool_.size());
    for (const auto& str : pool_) {
        result.push_back(str);
    }
    return result;
}

} // namespace Lua
