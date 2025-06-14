#pragma once

#include "../common/types.hpp"
#include <unordered_map>
#include <string>
#include <memory>

#pragma warning(disable: 4566)

namespace Lua {
    // Supported language enumeration
    enum class Language {
        English,    // English
        Chinese,    // Chinese
        Japanese,   // Japanese
        Korean,     // Korean
        French,     // French
        German,     // German
        Spanish,    // Spanish
        Russian     // Russian
    };
    
    // Message category enumeration
    enum class MessageCategory {
        ErrorType,      // Error type
        ErrorMessage,   // Error message
        Severity,       // Severity level
        FixSuggestion,  // Fix suggestion
        General         // General message
    };
    
    // Message key structure
    struct MessageKey {
        MessageCategory category;
        Str key;
        
        MessageKey(MessageCategory cat, const Str& k) : category(cat), key(k) {}
        
        bool operator==(const MessageKey& other) const {
            return category == other.category && key == other.key;
        }
    };
    
    // Hash function for message key
    struct MessageKeyHash {
        size_t operator()(const MessageKey& mk) const {
            return std::hash<int>()(static_cast<int>(mk.category)) ^ 
                   (std::hash<Str>()(mk.key) << 1);
        }
    };
    
    // Message catalog class - stores all messages for a specific language
    class MessageCatalog {
    private:
        Language language_;
        std::unordered_map<MessageKey, Str, MessageKeyHash> messages_;
        
    public:
        explicit MessageCatalog(Language lang) : language_(lang) {}
        
        // Add message
        void addMessage(MessageCategory category, const Str& key, const Str& message) {
            messages_[MessageKey(category, key)] = message;
        }
        
        // Get message
        Str getMessage(MessageCategory category, const Str& key) const {
            auto it = messages_.find(MessageKey(category, key));
            return (it != messages_.end()) ? it->second : key; // If not found, return the key itself
        }
        
        // Check if message exists
        bool hasMessage(MessageCategory category, const Str& key) const {
            return messages_.find(MessageKey(category, key)) != messages_.end();
        }
        
        // Get language
        Language getLanguage() const { return language_; }
        
        // Get message count
        size_t getMessageCount() const { return messages_.size(); }
    };
    
    // Localization manager class - singleton pattern
    class LocalizationManager {
    private:
        static std::unique_ptr<LocalizationManager> instance_;
        Language currentLanguage_;
        std::unordered_map<Language, std::unique_ptr<MessageCatalog>> catalogs_;
        
        // Private constructor
        LocalizationManager() : currentLanguage_(Language::English) {
            initializeDefaultCatalogs();
        }
        
        // Initialize default message catalogs
        void initializeDefaultCatalogs();
        
        // Initialize English messages
        void initializeEnglishMessages(MessageCatalog& catalog);
        
        // Initialize Chinese messages
        void initializeChineseMessages(MessageCatalog& catalog);
        
    public:
        // Get singleton instance
        static LocalizationManager& getInstance() {
            if (!instance_) {
                instance_ = std::unique_ptr<LocalizationManager>(new LocalizationManager());
            }
            return *instance_;
        }
        
        // Disable copy constructor and assignment
        LocalizationManager(const LocalizationManager&) = delete;
        LocalizationManager& operator=(const LocalizationManager&) = delete;
        
        // Set current language
        void setLanguage(Language language) {
            if (catalogs_.find(language) != catalogs_.end()) {
                currentLanguage_ = language;
            }
        }
        
        // Get current language
        Language getCurrentLanguage() const { return currentLanguage_; }
        
        // Get localized message
        Str getMessage(MessageCategory category, const Str& key) const {
            auto it = catalogs_.find(currentLanguage_);
            if (it != catalogs_.end()) {
                return it->second->getMessage(category, key);
            }
            // If current language is not available, fallback to English
            auto englishIt = catalogs_.find(Language::English);
            if (englishIt != catalogs_.end()) {
                return englishIt->second->getMessage(category, key);
            }
            return key; // Final fallback to the key itself
        }
        
        // Get formatted message (supports parameter substitution)
        Str getFormattedMessage(MessageCategory category, const Str& key, 
                               const std::vector<Str>& args) const {
            Str message = getMessage(category, key);
            return formatMessage(message, args);
        }
        
        // Add custom message catalog
        void addCatalog(Language language, std::unique_ptr<MessageCatalog> catalog) {
            catalogs_[language] = std::move(catalog);
        }
        
        // Check if language is supported
        bool isLanguageSupported(Language language) const {
            return catalogs_.find(language) != catalogs_.end();
        }
        
        // Get list of supported languages
        std::vector<Language> getSupportedLanguages() const {
            std::vector<Language> languages;
            for (const auto& pair : catalogs_) {
                languages.push_back(pair.first);
            }
            return languages;
        }
        
        // Convert language enum to string
        static Str languageToString(Language language) {
            switch (language) {
                case Language::English: return "English";
                case Language::Chinese: return "Chinese";
                case Language::Japanese: return "Japanese";
                case Language::Korean: return "Korean";
                case Language::French: return "French";
                case Language::German: return "German";
                case Language::Spanish: return "Spanish";
                case Language::Russian: return "Russian";
                default: return "Unknown";
            }
        }
        
        // Convert string to language enum
        static Language stringToLanguage(const Str& languageStr) {
            if (languageStr == "Chinese" || languageStr == "zh" || languageStr == "中文") {
                return Language::Chinese;
            } else if (languageStr == "Japanese" || languageStr == "ja" || languageStr == "日本語") {
                return Language::Japanese;
            } else if (languageStr == "Korean" || languageStr == "ko" || languageStr == "한국어") {
                return Language::Korean;
            } else if (languageStr == "French" || languageStr == "fr" || languageStr == "Français") {
                return Language::French;
            } else if (languageStr == "German" || languageStr == "de" || languageStr == "Deutsch") {
                return Language::German;
            } else if (languageStr == "Spanish" || languageStr == "es" || languageStr == "Español") {
                return Language::Spanish;
            } else if (languageStr == "Russian" || languageStr == "ru" || languageStr == "Русский") {
                return Language::Russian;
            }
            return Language::English; // Default to English
        }
        
    private:
        // Format message (replace placeholders)
        Str formatMessage(const Str& message, const std::vector<Str>& args) const {
            Str result = message;
            for (size_t i = 0; i < args.size(); ++i) {
                Str placeholder = "{" + std::to_string(i) + "}";
                size_t pos = result.find(placeholder);
                if (pos != Str::npos) {
                    result.replace(pos, placeholder.length(), args[i]);
                }
            }
            return result;
        }
    };
    
    // Convenience functions
    inline Str getLocalizedMessage(MessageCategory category, const Str& key) {
        return LocalizationManager::getInstance().getMessage(category, key);
    }
    
    inline Str getLocalizedMessage(MessageCategory category, const Str& key, 
                                  const std::vector<Str>& args) {
        return LocalizationManager::getInstance().getFormattedMessage(category, key, args);
    }
}