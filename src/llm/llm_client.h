#pragma once

#include <string>
#include <memory>
#include "clion/common.h"

namespace clion {
namespace llm {

struct LLMResponse {
    std::string content;
    std::vector<std::string> sources;
    int tokens_used = 0;
    bool success = false;
    std::string error_message;
};

class LLMClient {
public:
    LLMClient();
    ~LLMClient();
    
    bool initialize(const std::string& api_key);
    LLMResponse sendRequest(const std::string& prompt, 
                           const std::string& system_instruction = "",
                           float temperature = 0.1f);
    bool isInitialized() const { return initialized_; }

private:
    void* curl_;  // Will be typed as CURL* when CURL is available
    std::string api_key_;
    bool initialized_;
};

} // namespace llm
} // namespace clion