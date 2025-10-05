#pragma once

#include <string>
#include <map>
#include <vector>
#include "clion/common.h"

namespace clion {
namespace utils {

enum class ContentType {
    NATURAL_LANGUAGE,
    CODE,
    MIXED
};

struct ModelPricing {
    std::string model_name;
    std::string provider;
    double input_token_price;    // Price per 1K input tokens
    double output_token_price;   // Price per 1K output tokens
    int max_context_tokens;
    std::string currency = "USD";
};

struct TokenUsage {
    int input_tokens = 0;
    int output_tokens = 0;
    int total_tokens = 0;
    double input_cost = 0.0;
    double output_cost = 0.0;
    double total_cost = 0.0;
    std::string model;
    std::string timestamp;
};

class TokenCounter {
public:
    // Core token counting methods
    static int countTokens(const std::string& text);
    static int countTokens(const std::string& text, ContentType content_type);
    static int countTokensForModel(const std::string& text, const std::string& model);
    
    // Content type detection
    static ContentType detectContentType(const std::string& text);
    
    // Cost estimation methods
    static double estimateCost(int input_tokens, int output_tokens, const std::string& model);
    static double estimateInputCost(int tokens, const std::string& model);
    static double estimateOutputCost(int tokens, const std::string& model);
    
    // Model information methods
    static std::string getModelProvider(const std::string& model);
    static double getInputTokenPrice(const std::string& model);
    static double getOutputTokenPrice(const std::string& model);
    static ModelPricing getModelPricing(const std::string& model);
    
    // Token usage calculation
    static TokenUsage calculateUsage(const std::string& input_text,
                                   const std::string& model,
                                   int estimated_output_tokens = 0);
    
    // Pricing database management
    static void initializePricingDatabase();
    static bool isModelSupported(const std::string& model);
    static std::vector<std::string> getSupportedModels();

private:
    // Token counting algorithms
    static int countNaturalLanguageTokens(const std::string& text);
    static int countCodeTokens(const std::string& text);
    static int countMixedTokens(const std::string& text);
    
    // Content analysis helpers
    static bool isCodeLike(const std::string& text);
    static double calculateCodeRatio(const std::string& text);
    
    // Pricing database
    static std::map<std::string, ModelPricing> pricing_database_;
    static bool initialized_;
};

} // namespace utils
} // namespace clion
