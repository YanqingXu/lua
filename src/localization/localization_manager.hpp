#pragma once

#include "../common/types.hpp"
#include <unordered_map>
#include <string>
#include <memory>

#pragma warning(disable: 4566)

namespace Lua {
    // 支持的语言枚举
    enum class Language {
        English,    // 英语
        Chinese,    // 中文
        Japanese,   // 日语
        Korean,     // 韩语
        French,     // 法语
        German,     // 德语
        Spanish,    // 西班牙语
        Russian     // 俄语
    };
    
    // 消息类别枚举
    enum class MessageCategory {
        ErrorType,      // 错误类型
        ErrorMessage,   // 错误消息
        Severity,       // 严重程度
        FixSuggestion,  // 修复建议
        General         // 通用消息
    };
    
    // 消息键结构体
    struct MessageKey {
        MessageCategory category;
        Str key;
        
        MessageKey(MessageCategory cat, const Str& k) : category(cat), key(k) {}
        
        bool operator==(const MessageKey& other) const {
            return category == other.category && key == other.key;
        }
    };
    
    // 消息键的哈希函数
    struct MessageKeyHash {
        size_t operator()(const MessageKey& mk) const {
            return std::hash<int>()(static_cast<int>(mk.category)) ^ 
                   (std::hash<Str>()(mk.key) << 1);
        }
    };
    
    // 消息目录类 - 存储特定语言的所有消息
    class MessageCatalog {
    private:
        Language language_;
        std::unordered_map<MessageKey, Str, MessageKeyHash> messages_;
        
    public:
        explicit MessageCatalog(Language lang) : language_(lang) {}
        
        // 添加消息
        void addMessage(MessageCategory category, const Str& key, const Str& message) {
            messages_[MessageKey(category, key)] = message;
        }
        
        // 获取消息
        Str getMessage(MessageCategory category, const Str& key) const {
            auto it = messages_.find(MessageKey(category, key));
            return (it != messages_.end()) ? it->second : key; // 如果找不到，返回键本身
        }
        
        // 检查是否包含消息
        bool hasMessage(MessageCategory category, const Str& key) const {
            return messages_.find(MessageKey(category, key)) != messages_.end();
        }
        
        // 获取语言
        Language getLanguage() const { return language_; }
        
        // 获取消息数量
        size_t getMessageCount() const { return messages_.size(); }
    };
    
    // 本地化管理器类 - 单例模式
    class LocalizationManager {
    private:
        static std::unique_ptr<LocalizationManager> instance_;
        Language currentLanguage_;
        std::unordered_map<Language, std::unique_ptr<MessageCatalog>> catalogs_;
        
        // 私有构造函数
        LocalizationManager() : currentLanguage_(Language::English) {
            initializeDefaultCatalogs();
        }
        
        // 初始化默认消息目录
        void initializeDefaultCatalogs();
        
        // 初始化英文消息
        void initializeEnglishMessages(MessageCatalog& catalog);
        
        // 初始化中文消息
        void initializeChineseMessages(MessageCatalog& catalog);
        
    public:
        // 获取单例实例
        static LocalizationManager& getInstance() {
            if (!instance_) {
                instance_ = std::unique_ptr<LocalizationManager>(new LocalizationManager());
            }
            return *instance_;
        }
        
        // 禁用拷贝构造和赋值
        LocalizationManager(const LocalizationManager&) = delete;
        LocalizationManager& operator=(const LocalizationManager&) = delete;
        
        // 设置当前语言
        void setLanguage(Language language) {
            if (catalogs_.find(language) != catalogs_.end()) {
                currentLanguage_ = language;
            }
        }
        
        // 获取当前语言
        Language getCurrentLanguage() const { return currentLanguage_; }
        
        // 获取本地化消息
        Str getMessage(MessageCategory category, const Str& key) const {
            auto it = catalogs_.find(currentLanguage_);
            if (it != catalogs_.end()) {
                return it->second->getMessage(category, key);
            }
            // 如果当前语言不可用，回退到英语
            auto englishIt = catalogs_.find(Language::English);
            if (englishIt != catalogs_.end()) {
                return englishIt->second->getMessage(category, key);
            }
            return key; // 最后回退到键本身
        }
        
        // 获取格式化消息（支持参数替换）
        Str getFormattedMessage(MessageCategory category, const Str& key, 
                               const std::vector<Str>& args) const {
            Str message = getMessage(category, key);
            return formatMessage(message, args);
        }
        
        // 添加自定义消息目录
        void addCatalog(Language language, std::unique_ptr<MessageCatalog> catalog) {
            catalogs_[language] = std::move(catalog);
        }
        
        // 检查语言是否支持
        bool isLanguageSupported(Language language) const {
            return catalogs_.find(language) != catalogs_.end();
        }
        
        // 获取支持的语言列表
        std::vector<Language> getSupportedLanguages() const {
            std::vector<Language> languages;
            for (const auto& pair : catalogs_) {
                languages.push_back(pair.first);
            }
            return languages;
        }
        
        // 语言枚举转字符串
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
        
        // 字符串转语言枚举
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
            return Language::English; // 默认英语
        }
        
    private:
        // 格式化消息（替换占位符）
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
    
    // 便捷函数
    inline Str getLocalizedMessage(MessageCategory category, const Str& key) {
        return LocalizationManager::getInstance().getMessage(category, key);
    }
    
    inline Str getLocalizedMessage(MessageCategory category, const Str& key, 
                                  const std::vector<Str>& args) {
        return LocalizationManager::getInstance().getFormattedMessage(category, key, args);
    }
}