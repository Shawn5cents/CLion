#pragma once

#include <string>
#include <memory>
#include <vector>
#include <curl/curl.h>
#include "clion/common.h"
#include "../utils/token_counter.h"

namespace clion {
namespace llm {

// Supported LLM providers
enum class LLMProvider {
    OPENROUTER,
    REQUESTY_AI,
    OPENAI,
    GEMINI,
    CUSTOM
};

struct LLMResponse {
    std::string content;
    std::vector<std::string> sources;
    int tokens_used = 0;
    bool success = false;
    std::string error_message;
    int http_status_code = 0;
    std::string raw_response;
};

struct LLMConfig {
    LLMProvider provider = LLMProvider::OPENROUTER;
    std::string api_key;
    std::string model = "gpt-3.5-turbo";
    std::string custom_endpoint;
    int timeout_seconds = 30;
    int max_tokens = 4096;
    float temperature = 0.1f;
    bool verbose = false;
};

class LLMClient {
public:
    LLMClient();
    ~LLMClient();
    
    // Initialize with configuration
    bool initialize(const LLMConfig& config);
    bool initialize(const std::string& api_key);  // Legacy support
    
    // Send request to LLM
    LLMResponse sendRequest(const std::string& prompt,
                           const std::string& system_instruction = "",
                           float temperature = -1.0f);  // Use config temp if -1.0
    
    // Session-aware request methods
    LLMResponse sendRequestWithSession(const std::string& prompt,
                                      const std::string& session_id = "",
                                      const std::string& system_instruction = "",
                                      float temperature = -1.0f);
    
    // Session management methods
    std::string createNewSession();
    bool setSession(const std::string& session_id);
    std::string getCurrentSession() const { return current_session_id_; }
    void clearSession();
    std::vector<std::string> listSessions();
    bool deleteSession(const std::string& session_id);
    
    // Configuration methods
    void setProvider(LLMProvider provider);
    void setModel(const std::string& model);
    void setCustomEndpoint(const std::string& endpoint);
    void setTimeout(int seconds);
    void setVerbose(bool verbose);
    
    // Status methods
    bool isInitialized() const { return initialized_; }
    const LLMConfig& getConfig() const { return config_; }
    
    // Static helper methods
    static std::vector<LLMProvider> getSupportedProviders();
    static std::string getProviderName(LLMProvider provider);
    static std::string getDefaultModel(LLMProvider provider);

private:
    CURL* curl_;
    LLMConfig config_;
    bool initialized_;
    std::string current_session_id_;
    
    // Token analysis structures
    struct RequestAnalysis {
        int input_tokens;
        int estimated_output_tokens;
        double estimated_cost;
        std::string model;
        bool within_limits;
        utils::TokenUsage usage_details;
    };
    
    // CURL callback for writing response
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    // Helper methods
    std::string buildJsonPayload(const std::string& prompt,
                                const std::string& system_instruction,
                                float temperature) const;
    LLMResponse parseResponse(const std::string& json_response) const;
    LLMResponse sendJsonPayload(const std::string& json_payload);
    
    // Provider-specific methods
    void setProviderDefaults();
    std::string getEndpoint() const;
    std::string getAuthHeader() const;
    std::string buildPayloadForProvider(const std::string& prompt,
                                       const std::string& system_instruction,
                                       float temperature) const;
    LLMResponse parseResponseForProvider(const std::string& json_response) const;
    
    // Token analysis methods
    RequestAnalysis analyzeRequest(const std::string& prompt,
                                 const std::string& system_instruction = "",
                                 int max_output_tokens = 0);
    void displayTokenUsage(const RequestAnalysis& analysis);
    bool userConfirmsRequest(const RequestAnalysis& analysis);
    
    // Error handling
    void logError(const std::string& message) const;
    void logInfo(const std::string& message) const;
};

} // namespace llm
} // namespace clion