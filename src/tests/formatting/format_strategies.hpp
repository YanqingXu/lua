#pragma once
#include "../../common/types.hpp"
#include "test_define.hpp"
#include "test_colors.hpp"

namespace Lua {
namespace Tests {
namespace TestFormatting {

// Forward declarations
// class TestColorManager; // No longer needed as we include the full header

// Base interface for formatting strategies
class IFormatStrategy {
public:
    virtual ~IFormatStrategy() = default;
    virtual void printHeader(const Str& title, 
                           const Str& description,
                           const TestColorManager& colorManager) = 0;
    virtual void printFooter(const Str& message,
                            const TestColorManager& colorManager) = 0;
};

// Main level formatting strategy
class MainFormatStrategy : public IFormatStrategy {
public:
    void printHeader(const Str& title, 
                   const Str& description,
                   const TestColorManager& colorManager) override;
    void printFooter(const Str& message,
                   const TestColorManager& colorManager) override;
};

// Suite level formatting strategy
class SuiteFormatStrategy : public IFormatStrategy {
public:
    void printHeader(const Str& title, 
                   const Str& description,
                   const TestColorManager& colorManager) override;
    void printFooter(const Str& message,
                   const TestColorManager& colorManager) override;
};

// Group level formatting strategy
class GroupFormatStrategy : public IFormatStrategy {
public:
    void printHeader(const Str& title, 
                   const Str& description,
                   const TestColorManager& colorManager) override;
    void printFooter(const Str& message,
                   const TestColorManager& colorManager) override;
};

// Individual test formatting strategy
class IndividualFormatStrategy : public IFormatStrategy {
public:
    void printHeader(const Str& title, 
                   const Str& description,
                   const TestColorManager& colorManager) override;
    void printFooter(const Str& message,
                   const TestColorManager& colorManager) override;
};

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua