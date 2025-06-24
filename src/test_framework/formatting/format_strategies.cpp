#include "format_strategies.hpp"
#include "format_colors.hpp"
#include "format_config.hpp"
#include <iostream>

namespace Lua {
namespace Tests {
namespace TestFormatting {

// MainFormatStrategy Implementation
void MainFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::MAIN);
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    char headerChar = config.headerChar.empty() ? '=' : config.headerChar[0];
    int width = config.width;
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    std::cout << "\n" << indentStr << headerColor << Str(width, headerChar) << resetColor << std::endl;
    std::cout << indentStr << headerColor << "  " << emphasisColor << title << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << indentStr << headerColor << "  " << description << resetColor << std::endl;
    }
    std::cout << indentStr << headerColor << Str(width, headerChar) << resetColor << std::endl;
}

void MainFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::MAIN);
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    char footerChar = config.footerChar.empty() ? '=' : config.footerChar[0];
    int width = config.width;
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    std::cout << indentStr << headerColor << Str(width, footerChar) << resetColor << std::endl;
    if (!message.empty()) {
        std::cout << indentStr << headerColor << "  " << message << resetColor << std::endl;
        std::cout << indentStr << headerColor << Str(width, footerChar) << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// ModuleFormatStrategy Implementation
void ModuleFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::MODULE);
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    char headerChar = config.headerChar.empty() ? '-' : config.headerChar[0];
    int width = config.width;
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    std::cout << "\n" << indentStr << headerColor << "+" << Str(width - 2, headerChar) << "+" << resetColor << std::endl;
    std::cout << indentStr << headerColor << "| " << emphasisColor << title << Str(width - title.length() - 3, ' ') << headerColor << "|" << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << indentStr << headerColor << "| " << description << Str(width - description.length() - 3, ' ') << "|" << resetColor << std::endl;
    }
    std::cout << indentStr << headerColor << "+" << Str(width - 2, headerChar) << "+" << resetColor << std::endl;
}

void ModuleFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::MODULE);
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    if (!message.empty()) {
        std::cout << indentStr << headerColor << "+- " << message << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// SuiteFormatStrategy Implementation
void SuiteFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::SUITE);
    Str subheaderColor = colorManager.getColor(ColorType::SUBHEADER);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    char headerChar = config.headerChar.empty() ? '-' : config.headerChar[0];
    int width = config.width;
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    std::cout << "\n" << indentStr << subheaderColor << "+" << Str(width - 2, headerChar) << "+" << resetColor << std::endl;
    std::cout << indentStr << subheaderColor << "| " << emphasisColor << title << Str(width - title.length() - 3, ' ') << subheaderColor << "|" << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << indentStr << subheaderColor << "| " << description << Str(width - description.length() - 3, ' ') << "|" << resetColor << std::endl;
    }
    std::cout << indentStr << subheaderColor << "+" << Str(width - 2, headerChar) << "+" << resetColor << std::endl;
}

void SuiteFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::SUITE);
    Str subheaderColor = colorManager.getColor(ColorType::SUBHEADER);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    if (!message.empty()) {
        std::cout << indentStr << subheaderColor << "+- " << message << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// GroupFormatStrategy Implementation
void GroupFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::GROUP);
    Str infoColor = colorManager.getColor(ColorType::INFO);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    std::cout << "\n" << indentStr << infoColor << "+- " << emphasisColor << title << resetColor;
    if (!description.empty()) {
        std::cout << infoColor << " - " << description << resetColor;
    }
    std::cout << std::endl;
}

void GroupFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::GROUP);
    Str infoColor = colorManager.getColor(ColorType::INFO);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    if (!message.empty()) {
        std::cout << indentStr << infoColor << "+- " << message << resetColor << std::endl;
    }
}

// IndividualFormatStrategy Implementation
void IndividualFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::INDIVIDUAL);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    std::cout << indentStr << emphasisColor << title << resetColor;
    if (!description.empty()) {
        std::cout << " - " << description;
    }
    std::cout << std::endl;
}

void IndividualFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    const auto& config = TestConfig::getInstance().getLevelConfig(TestLevel::INDIVIDUAL);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    int indent = config.indent;
    Str indentStr(indent, ' ');
    
    if (!message.empty()) {
        std::cout << indentStr << emphasisColor << "+- " << message << resetColor << std::endl;
    }
}

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua