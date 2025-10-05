#include "token_counter.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace clion {
namespace utils {

// Static member definitions
std::map<std::string, ModelPricing> TokenCounter::pricing_database_;
bool TokenCounter::initialized_ = false;

// Token counting implementation
int TokenCounter::countTokens(const std::string& text) {
    if (text.empty()) return 0;
    
    ContentType type = detectContentType(text);
    return countTokens(text, type);
}

int TokenCounter::countTokens(const std::string& text, ContentType content_type) {
    switch (content_type) {
        case ContentType::NATURAL_LANGUAGE:
            return countNaturalLanguageTokens(text);
        case ContentType::CODE:
            return countCodeTokens(text);
        case ContentType::MIXED:
            return countMixedTokens(text);
    }
    return 0;
}

int TokenCounter::countTokensForModel(const std::string& text, const std::string& model) {
    // For now, use the standard counting method
    // In the future, this could be model-specific
    (void)model; // Mark as unused for now
    return countTokens(text);
}

ContentType TokenCounter::detectContentType(const std::string& text) {
    double code_ratio = calculateCodeRatio(text);
    
    if (code_ratio > 0.6) {
        return ContentType::CODE;
    } else if (code_ratio < 0.2) {
        return ContentType::NATURAL_LANGUAGE;
    } else {
        return ContentType::MIXED;
    }
}

double TokenCounter::calculateCodeRatio(const std::string& text) {
    if (text.empty()) return 0.0;
    
    // Count code indicators
    int code_indicators = 0;
    int total_indicators = 0;
    
    // Code patterns
    std::vector<std::regex> code_patterns = {
        std::regex(R"(\b(class|struct|function|int|float|double|char|bool|void|return|if|else|for|while|do|switch|case|break|continue|include|import|namespace|using|public|private|protected)\b)"),
        std::regex(R"([{}();\[\]])"),
        std::regex(R"(/\*.*?\*/|//.*?$)"), // Comments
        std::regex(R"(\b[A-Za-z_][A-Za-z0-9_]*\s*\([^)]*\)\s*\{)"), // Function definitions
        std::regex(R"(\b[A-Za-z_][A-Za-z0-9_]*\s*[=+\-*/])") // Variable assignments
    };
    
    // Natural language patterns
    std::vector<std::regex> lang_patterns = {
        std::regex(R"(\b(the|and|or|but|in|on|at|to|for|of|with|by|from|up|about|into|through|during|before|after|above|below|between|among|under|over|above)\b)"),
        std::regex(R"([.!?]\s+[A-Z])"), // Sentence endings
        std::regex(R"(\b[Ii]s\b|\b[aA]re\b|\b[wW]as\b|\b[wW]ere\b|\b[hH]ave\b|\b[hH]as\b|\b[wW]ill\b|\b[wW]ould\b)") // Common verbs
    };
    
    for (const auto& pattern : code_patterns) {
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), pattern);
        auto words_end = std::sregex_iterator();
        code_indicators += std::distance(words_begin, words_end);
    }
    
    for (const auto& pattern : lang_patterns) {
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), pattern);
        auto words_end = std::sregex_iterator();
        total_indicators += std::distance(words_begin, words_end);
    }
    
    total_indicators += code_indicators;
    return total_indicators > 0 ? static_cast<double>(code_indicators) / total_indicators : 0.0;
}

int TokenCounter::countNaturalLanguageTokens(const std::string& text) {
    // Improved natural language token counting
    std::istringstream iss(text);
    std::string word;
    int token_count = 0;
    
    while (iss >> word) {
        // Base token: 1 token per word on average
        token_count++;
        
        // Add tokens for long words (likely to be split)
        if (word.length() > 8) {
            token_count += word.length() / 4;
        }
        
        // Add tokens for punctuation and special chars
        for (char c : word) {
            if (std::ispunct(c) && c != '\'') {
                token_count += 0.25; // Punctuation often groups with words
            }
        }
    }
    
    // Account for whitespace and formatting
    token_count += std::count(text.begin(), text.end(), '\n') * 0.1;
    
    return static_cast<int>(token_count);
}

int TokenCounter::countCodeTokens(const std::string& text) {
    // More conservative code token counting
    int token_count = 0;
    std::istringstream iss(text);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.empty()) {
            token_count += 0.1; // Empty lines
            continue;
        }
        
        // Count operators and punctuation as separate tokens
        for (char c : line) {
            if (std::string("{}();,[]").find(c) != std::string::npos) {
                token_count += 0.5;
            } else if (std::string("+-*/%=<>!&|^~").find(c) != std::string::npos) {
                token_count += 0.5;
            }
        }
        
        // Count words (identifiers, keywords)
        std::istringstream line_iss(line);
        std::string word;
        while (line_iss >> word) {
            // Skip comments
            if (word.find("//") == 0 || word.find("/*") == 0) {
                break;
            }
            
            // Clean word of punctuation
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            
            if (!word.empty()) {
                // Long identifiers get split into multiple tokens
                if (word.length() > 6) {
                    token_count += word.length() / 3.0;
                } else {
                    token_count += 1.0;
                }
            }
        }
    }
    
    return static_cast<int>(token_count);
}

int TokenCounter::countMixedTokens(const std::string& text) {
    // Weighted average of code and natural language counting
    double code_ratio = calculateCodeRatio(text);
    int code_tokens = countCodeTokens(text);
    int lang_tokens = countNaturalLanguageTokens(text);
    
    return static_cast<int>(code_ratio * code_tokens + (1.0 - code_ratio) * lang_tokens);
}

bool TokenCounter::isCodeLike(const std::string& text) {
    return calculateCodeRatio(text) > 0.5;
}

// Pricing database implementation
void TokenCounter::initializePricingDatabase() {
    if (initialized_) return;
    
    // OpenRouter Models
    pricing_database_["meta-llama/llama-3.1-8b-instruct:free"] = {
        "meta-llama/llama-3.1-8b-instruct:free", "OpenRouter", 0.0, 0.0, 128000
    };
    pricing_database_["meta-llama/llama-3.1-70b-instruct"] = {
        "meta-llama/llama-3.1-70b-instruct", "OpenRouter", 0.00088, 0.00088, 128000
    };
    pricing_database_["openai/gpt-4o-mini"] = {
        "openai/gpt-4o-mini", "OpenRouter", 0.00015, 0.00060, 128000
    };
    pricing_database_["anthropic/claude-3-haiku"] = {
        "anthropic/claude-3-haiku", "OpenRouter", 0.00025, 0.00125, 200000
    };
    
    // OpenAI Models
    pricing_database_["gpt-3.5-turbo"] = {
        "gpt-3.5-turbo", "OpenAI", 0.0005, 0.0015, 16385
    };
    pricing_database_["gpt-4"] = {
        "gpt-4", "OpenAI", 0.03, 0.06, 8192
    };
    pricing_database_["gpt-4o-mini"] = {
        "gpt-4o-mini", "OpenAI", 0.00015, 0.00060, 128000
    };
    pricing_database_["gpt-4o"] = {
        "gpt-4o", "OpenAI", 0.005, 0.015, 128000
    };
    
    // Gemini Models
    pricing_database_["gemini-pro"] = {
        "gemini-pro", "Gemini", 0.00025, 0.0005, 32768
    };
    pricing_database_["gemini-pro-vision"] = {
        "gemini-pro-vision", "Gemini", 0.00025, 0.0005, 16384
    };
    
    // Requesty AI Models
    pricing_database_["claude-3-haiku"] = {
        "claude-3-haiku", "Requesty AI", 0.00025, 0.00125, 200000
    };
    pricing_database_["claude-3-sonnet"] = {
        "claude-3-sonnet", "Requesty AI", 0.003, 0.015, 200000
    };
    
    initialized_ = true;
}

// Cost estimation implementation
double TokenCounter::estimateCost(int input_tokens, int output_tokens, const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        // Default pricing for unknown models
        return (input_tokens + output_tokens) * 0.00001;
    }
    
    const ModelPricing& pricing = it->second;
    double input_cost = (input_tokens / 1000.0) * pricing.input_token_price;
    double output_cost = (output_tokens / 1000.0) * pricing.output_token_price;
    
    return input_cost + output_cost;
}

double TokenCounter::estimateInputCost(int tokens, const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        return tokens * 0.00001;
    }
    
    return (tokens / 1000.0) * it->second.input_token_price;
}

double TokenCounter::estimateOutputCost(int tokens, const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        return tokens * 0.00001;
    }
    
    return (tokens / 1000.0) * it->second.output_token_price;
}

// Model information methods
std::string TokenCounter::getModelProvider(const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        return "Unknown";
    }
    
    return it->second.provider;
}

double TokenCounter::getInputTokenPrice(const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        return 0.00001; // Default price
    }
    
    return it->second.input_token_price;
}

double TokenCounter::getOutputTokenPrice(const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        return 0.00001; // Default price
    }
    
    return it->second.output_token_price;
}

ModelPricing TokenCounter::getModelPricing(const std::string& model) {
    initializePricingDatabase();
    
    auto it = pricing_database_.find(model);
    if (it == pricing_database_.end()) {
        // Return default pricing
        return {model, "Unknown", 0.00001, 0.00001, 4096};
    }
    
    return it->second;
}

// Token usage calculation
TokenUsage TokenCounter::calculateUsage(const std::string& input_text,
                                       const std::string& model,
                                       int estimated_output_tokens) {
    TokenUsage usage;
    
    usage.input_tokens = countTokensForModel(input_text, model);
    usage.output_tokens = estimated_output_tokens;
    usage.total_tokens = usage.input_tokens + usage.output_tokens;
    usage.input_cost = estimateInputCost(usage.input_tokens, model);
    usage.output_cost = estimateOutputCost(usage.output_tokens, model);
    usage.total_cost = usage.input_cost + usage.output_cost;
    usage.model = model;
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    usage.timestamp = ss.str();
    
    return usage;
}

bool TokenCounter::isModelSupported(const std::string& model) {
    initializePricingDatabase();
    return pricing_database_.find(model) != pricing_database_.end();
}

std::vector<std::string> TokenCounter::getSupportedModels() {
    initializePricingDatabase();
    
    std::vector<std::string> models;
    for (const auto& pair : pricing_database_) {
        models.push_back(pair.first);
    }
    
    return models;
}

} // namespace utils
} // namespace clion
