#include "llm_client.h"
#include "session.h"
#include "clion/common.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>

using json = nlohmann::json;

namespace clion {
namespace llm {

LLMClient::LLMClient() : curl_(nullptr), initialized_(false) {
    // Initialize CURL
    curl_ = curl_easy_init();
    if (curl_) {
        setProviderDefaults();
    }
}

LLMClient::~LLMClient() {
    // Cleanup CURL
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

bool LLMClient::initialize(const LLMConfig& config) {
    if (!curl_) {
        logError("CURL initialization failed");
        return false;
    }
    
    config_ = config;
    setProviderDefaults();
    
    if (config_.api_key.empty()) {
        logError("API key is required");
        return false;
    }
    
    initialized_ = true;
    logInfo("LLMClient initialized with provider: " + getProviderName(config_.provider));
    return true;
}

bool LLMClient::initialize(const std::string& api_key) {
    // Legacy support for backward compatibility
    LLMConfig config;
    config.api_key = api_key;
    config.provider = LLMProvider::OPENROUTER;
    return initialize(config);
}

LLMResponse LLMClient::sendRequest(const std::string& prompt,
                                   const std::string& system_instruction,
                                   float temperature) {
    logInfo("=== LLMClient::sendRequest START ===");
    logInfo("Request size: " + std::to_string(prompt.length()) + " chars");
    logInfo("System instruction size: " + std::to_string(system_instruction.length()) + " chars");

    // Analyze request before sending
    RequestAnalysis analysis = analyzeRequest(prompt, system_instruction);

    logInfo("Request analysis - Input tokens: " + std::to_string(analysis.input_tokens) +
            ", Estimated output: " + std::to_string(analysis.estimated_output_tokens));
    
    // Display token usage and cost to user
    displayTokenUsage(analysis);
    
    // Check if user wants to proceed
    if (!analysis.within_limits && !userConfirmsRequest(analysis)) {
        LLMResponse response;
        response.error_message = "Request cancelled by user due to cost/size concerns";
        response.success = false;
        return response;
    }
    
    // Proceed with original implementation
    LLMResponse response;
    
    if (!initialized_) {
        response.error_message = "LLMClient not initialized";
        response.http_status_code = 0;
        logError("LLMClient not initialized");
        return response;
    }
    
    if (!curl_) {
        response.error_message = "CURL not initialized";
        response.http_status_code = 0;
        logError("CURL not initialized");
        return response;
    }
    
    // Use config temperature if not specified
    float actual_temp = (temperature < 0.0f) ? config_.temperature : temperature;
    
    // Build JSON payload
    std::string json_payload = buildPayloadForProvider(prompt, system_instruction, actual_temp);
    
    if (config_.verbose) {
        logInfo("Sending request to: " + getEndpoint());
        logInfo("Payload: " + json_payload);
    }
    
    // Set up CURL options
    std::string read_buffer;
    std::string header_buffer;
    curl_easy_setopt(curl_, CURLOPT_URL, getEndpoint().c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &read_buffer);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_buffer);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, getAuthHeader().c_str());
    
    // Add OpenRouter-specific headers
    if (config_.provider == LLMProvider::OPENROUTER) {
        headers = curl_slist_append(headers, "HTTP-Referer: https://github.com/Shawn5cents/CLion");
        headers = curl_slist_append(headers, "X-Title: CLion-CPP-Tool");
    }
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Set timeout and other options
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        response.error_message = "CURL error: " + std::string(curl_easy_strerror(res));
        response.http_status_code = 0;
        logError(response.error_message);
        return response;
    }
    
    // Check HTTP response code
    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    response.http_status_code = static_cast<int>(http_code);
    
    if (config_.verbose) {
        logInfo("HTTP Status: " + std::to_string(http_code));
        logInfo("Response: " + read_buffer);
    }
    
    if (http_code != 200) {
        response.error_message = "HTTP error: " + std::to_string(http_code) + " - " + read_buffer;
        logError(response.error_message);
        return response;
    }
    
    // Store raw response
    response.raw_response = read_buffer;
    
    // Parse response
    response = parseResponseForProvider(read_buffer);

    logInfo("=== LLMClient::sendRequest END ===");
    logInfo("Response success: " + std::string(response.success ? "true" : "false"));
    logInfo("Response content size: " + std::to_string(response.content.length()) + " chars");
    logInfo("Response tokens used: " + std::to_string(response.tokens_used));
    if (!response.error_message.empty()) {
        logError("Response error: " + response.error_message);
    }

    return response;
}

LLMResponse LLMClient::sendRequestWithSession(const std::string& prompt,
                                              const std::string& session_id,
                                              const std::string& system_instruction,
                                              float temperature) {
    logInfo("=== LLMClient::sendRequestWithSession START ===");
    logInfo("Input session_id: '" + session_id + "', current_session_id_: '" + current_session_id_ + "'");

    // Determine which session to use
    std::string target_session_id = session_id;
    if (target_session_id.empty()) {
        target_session_id = current_session_id_;
    }
    logInfo("Target session_id: '" + target_session_id + "'");
    
    // If no session exists, create one
    if (target_session_id.empty()) {
        target_session_id = createNewSession();
        if (target_session_id.empty()) {
            LLMResponse response;
            response.error_message = "Failed to create session";
            response.success = false;
            return response;
        }
    }
    
    // Load session history
    auto session_opt = SessionManager::loadSession(target_session_id);
    if (!session_opt) {
        LLMResponse response;
        response.error_message = "Failed to load session: " + target_session_id;
        response.success = false;
        return response;
    }
    
    const Session& session = *session_opt;
    
    // Build conversation context
    std::string enhanced_prompt;
    json messages = json::array();
    
    // Add system instruction if provided
    if (!system_instruction.empty()) {
        messages.push_back({
            {"role", "system"},
            {"content", system_instruction}
        });
    }
    
    // Add conversation history
    for (const auto& entry : session.entries) {
        messages.push_back({
            {"role", entry.role},
            {"content", entry.content}
        });
    }
    
    // Add current user message
    messages.push_back({
        {"role", "user"},
        {"content", prompt}
    });
    
    // Save user message to session
    SessionManager::addEntryToSession(target_session_id, "user", prompt);
    
    // Build JSON payload with conversation history
    json payload;
    if (config_.provider == LLMProvider::OPENROUTER ||
        config_.provider == LLMProvider::REQUESTY_AI ||
        config_.provider == LLMProvider::OPENAI ||
        config_.provider == LLMProvider::CUSTOM) {
        payload["model"] = config_.model;
        payload["messages"] = messages;
        payload["temperature"] = (temperature < 0.0f) ? config_.temperature : temperature;
        payload["max_tokens"] = config_.max_tokens;
        payload["stream"] = false;
    } else if (config_.provider == LLMProvider::GEMINI) {
        // Convert messages to Gemini format
        if (!system_instruction.empty()) {
            payload["systemInstruction"] = {
                {"parts", {{"text", system_instruction}}}
            };
        }
        
        json contents = json::array();
        for (const auto& message : messages) {
            if (message["role"] == "system") continue; // Skip system messages for Gemini
            
            contents.push_back({
                {"parts", {{"text", message["content"]}}}
            });
        }
        payload["contents"] = contents;
        
        payload["generationConfig"] = {
            {"temperature", (temperature < 0.0f) ? config_.temperature : temperature},
            {"topK", 40},
            {"topP", 0.95},
            {"maxOutputTokens", config_.max_tokens}
        };
    }
    
    // Send request with conversation context
    LLMResponse response = sendJsonPayload(payload.dump());
    
    // Save assistant response to session if successful
    if (response.success && !response.content.empty()) {
        logInfo("Saving assistant response to session: " + target_session_id);
        SessionManager::addEntryToSession(target_session_id, "assistant", response.content);
    }

    logInfo("=== LLMClient::sendRequestWithSession END ===");
    logInfo("Final session_id: '" + target_session_id + "', current_session_id_: '" + current_session_id_ + "'");

    return response;
}

LLMResponse LLMClient::sendJsonPayload(const std::string& json_payload) {
    LLMResponse response;
    
    if (!initialized_) {
        response.error_message = "LLMClient not initialized";
        response.http_status_code = 0;
        logError("LLMClient not initialized");
        return response;
    }
    
    if (!curl_) {
        response.error_message = "CURL not initialized";
        response.http_status_code = 0;
        logError("CURL not initialized");
        return response;
    }
    
    if (config_.verbose) {
        logInfo("Sending request to: " + getEndpoint());
        logInfo("Payload: " + json_payload);
    }
    
    // Set up CURL options
    std::string read_buffer;
    std::string header_buffer;
    logInfo("Setting up CURL buffers - read_buffer capacity: " + std::to_string(read_buffer.capacity()) +
            ", header_buffer capacity: " + std::to_string(header_buffer.capacity()));
    curl_easy_setopt(curl_, CURLOPT_URL, getEndpoint().c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &read_buffer);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_buffer);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, getAuthHeader().c_str());
    
    // Add OpenRouter-specific headers
    if (config_.provider == LLMProvider::OPENROUTER) {
        headers = curl_slist_append(headers, "HTTP-Referer: https://github.com/Shawn5cents/CLion");
        headers = curl_slist_append(headers, "X-Title: CLion-CPP-Tool");
    }
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Set timeout and other options
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Perform request
    logInfo("Performing CURL request...");
    CURLcode res = curl_easy_perform(curl_);

    // Clean up headers
    logInfo(std::string("Cleaning up headers, headers pointer: ") + (headers ? "valid" : "nullptr"));
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        response.error_message = "CURL error: " + std::string(curl_easy_strerror(res));
        response.http_status_code = 0;
        logError(response.error_message);
        return response;
    }
    
    // Check HTTP response code
    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    response.http_status_code = static_cast<int>(http_code);
    
    if (config_.verbose) {
        logInfo("HTTP Status: " + std::to_string(http_code));
        logInfo("Response: " + read_buffer);
    }
    
    if (http_code != 200) {
        response.error_message = "HTTP error: " + std::to_string(http_code) + " - " + read_buffer;
        logError(response.error_message);
        return response;
    }
    
    // Store raw response
    response.raw_response = read_buffer;
    logInfo("Raw response size: " + std::to_string(read_buffer.length()) + " bytes");
    logInfo("Response buffer capacity: " + std::to_string(read_buffer.capacity()));

    // Parse response
    logInfo("Parsing response for provider: " + getProviderName(config_.provider));
    response = parseResponseForProvider(read_buffer);
    return response;
}

size_t LLMClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

size_t LLMClient::HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* headers = static_cast<std::string*>(userp);
    headers->append(static_cast<char*>(contents), total_size);
    return total_size;
}

void LLMClient::setProvider(LLMProvider provider) {
    config_.provider = provider;
    setProviderDefaults();
}

void LLMClient::setModel(const std::string& model) {
    config_.model = model;
}

void LLMClient::setCustomEndpoint(const std::string& endpoint) {
    config_.custom_endpoint = endpoint;
    config_.provider = LLMProvider::CUSTOM;
}

void LLMClient::setTimeout(int seconds) {
    config_.timeout_seconds = seconds;
}

void LLMClient::setVerbose(bool verbose) {
    config_.verbose = verbose;
}

std::string LLMClient::createNewSession() {
    std::string session_id = SessionManager::createNewSession();
    if (!session_id.empty()) {
        current_session_id_ = session_id;
        logInfo("Created new session: " + session_id);
    }
    return session_id;
}

bool LLMClient::setSession(const std::string& session_id) {
    logInfo("=== LLMClient::setSession ===");
    logInfo("Attempting to set session to: '" + session_id + "', current: '" + current_session_id_ + "'");
    if (SessionManager::sessionExists(session_id)) {
        current_session_id_ = session_id;
        logInfo("Successfully set current session to: " + session_id);
        return true;
    }
    logError("Session not found: " + session_id);
    return false;
}

void LLMClient::clearSession() {
    if (!current_session_id_.empty()) {
        logInfo("Cleared current session: " + current_session_id_);
        current_session_id_.clear();
    }
}

std::vector<std::string> LLMClient::listSessions() {
    return SessionManager::listSessions();
}

bool LLMClient::deleteSession(const std::string& session_id) {
    bool result = SessionManager::deleteSession(session_id);
    if (result) {
        logInfo("Deleted session: " + session_id);
        // Clear current session if it was the one deleted
        if (current_session_id_ == session_id) {
            current_session_id_.clear();
        }
    } else {
        logError("Failed to delete session: " + session_id);
    }
    return result;
}

std::vector<LLMProvider> LLMClient::getSupportedProviders() {
    return {
        LLMProvider::OPENROUTER,
        LLMProvider::REQUESTY_AI,
        LLMProvider::OPENAI,
        LLMProvider::GEMINI,
        LLMProvider::CUSTOM
    };
}

std::string LLMClient::getProviderName(LLMProvider provider) {
    static const std::map<LLMProvider, std::string> provider_names = {
        {LLMProvider::OPENROUTER, "OpenRouter"},
        {LLMProvider::REQUESTY_AI, "Requesty AI"},
        {LLMProvider::OPENAI, "OpenAI"},
        {LLMProvider::GEMINI, "Gemini"},
        {LLMProvider::CUSTOM, "Custom"}
    };
    return provider_names.at(provider);
}

std::string LLMClient::getDefaultModel(LLMProvider provider) {
    static const std::map<LLMProvider, std::string> default_models = {
        {LLMProvider::OPENROUTER, "meta-llama/llama-3.1-8b-instruct:free"},
        {LLMProvider::REQUESTY_AI, "claude-3-haiku"},
        {LLMProvider::OPENAI, "gpt-3.5-turbo"},
        {LLMProvider::GEMINI, "gemini-pro"},
        {LLMProvider::CUSTOM, "custom-model"}
    };
    return default_models.at(provider);
}

void LLMClient::setProviderDefaults() {
    if (config_.model.empty()) {
        config_.model = getDefaultModel(config_.provider);
    }
}

std::string LLMClient::getEndpoint() const {
    switch (config_.provider) {
        case LLMProvider::OPENROUTER:
            return "https://openrouter.ai/api/v1/chat/completions";
        case LLMProvider::REQUESTY_AI:
            return "https://api.requesty.ai/v1/chat/completions";
        case LLMProvider::OPENAI:
            return "https://api.openai.com/v1/chat/completions";
        case LLMProvider::GEMINI:
            return "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent";
        case LLMProvider::CUSTOM:
            return config_.custom_endpoint;
        default:
            return "https://openrouter.ai/api/v1/chat/completions";
    }
}

std::string LLMClient::getAuthHeader() const {
    std::string header;
    switch (config_.provider) {
        case LLMProvider::OPENROUTER:
        case LLMProvider::REQUESTY_AI:
        case LLMProvider::OPENAI:
            header = "Authorization: Bearer " + config_.api_key;
            break;
        case LLMProvider::GEMINI:
            header = "x-goog-api-key: " + config_.api_key;
            break;
        case LLMProvider::CUSTOM:
            header = "Authorization: Bearer " + config_.api_key;  // Default to Bearer
            break;
        default:
            header = "Authorization: Bearer " + config_.api_key;
            break;
    }
    return header;
}

std::string LLMClient::buildPayloadForProvider(const std::string& prompt,
                                              const std::string& system_instruction,
                                              float temperature) const {
    json payload;
    
    if (config_.provider == LLMProvider::OPENROUTER ||
        config_.provider == LLMProvider::REQUESTY_AI ||
        config_.provider == LLMProvider::OPENAI) {
        // OpenAI-compatible format
        payload["model"] = config_.model;
        
        // Add messages array
        json messages = json::array();
        
        // Add system instruction if provided
        if (!system_instruction.empty()) {
            messages.push_back({
                {"role", "system"},
                {"content", system_instruction}
            });
        }
        
        // Add user message
        messages.push_back({
            {"role", "user"},
            {"content", prompt}
        });
        
        payload["messages"] = messages;
        payload["temperature"] = temperature;
        payload["max_tokens"] = config_.max_tokens;
        payload["stream"] = false;
    }
    else if (config_.provider == LLMProvider::GEMINI) {
        // Gemini format
        if (!system_instruction.empty()) {
            payload["systemInstruction"] = {
                {"parts", {{"text", system_instruction}}}
            };
        }
        
        payload["contents"] = json::array({
            {
                {"parts", {{"text", prompt}}}
            }
        });
        
        payload["generationConfig"] = {
            {"temperature", temperature},
            {"topK", 40},
            {"topP", 0.95},
            {"maxOutputTokens", config_.max_tokens}
        };
    }
    else if (config_.provider == LLMProvider::CUSTOM) {
        // Default to OpenAI format for custom providers
        payload["model"] = config_.model;
        
        json messages = json::array();
        if (!system_instruction.empty()) {
            messages.push_back({
                {"role", "system"},
                {"content", system_instruction}
            });
        }
        messages.push_back({
            {"role", "user"},
            {"content", prompt}
        });
        payload["messages"] = messages;
        
        payload["temperature"] = temperature;
        payload["max_tokens"] = config_.max_tokens;
        payload["stream"] = false;
    }
    
    return payload.dump();
}

LLMResponse LLMClient::parseResponseForProvider(const std::string& json_response) const {
    LLMResponse response;
    
    try {
        json j = json::parse(json_response);
        
        // Check for errors (common across providers)
        if (j.contains("error")) {
            if (j["error"].contains("message")) {
                response.error_message = j["error"]["message"];
            } else {
                response.error_message = j["error"].dump();
            }
            logError("API Error: " + response.error_message);
            return response;
        }
        
        switch (config_.provider) {
            case LLMProvider::OPENROUTER:
            case LLMProvider::REQUESTY_AI:
            case LLMProvider::OPENAI:
            case LLMProvider::CUSTOM:
                // OpenAI-compatible format
                if (j.contains("choices") && !j["choices"].empty()) {
                    const auto& choice = j["choices"][0];
                    if (choice.contains("message") && choice["message"].contains("content")) {
                        response.content = choice["message"]["content"];
                        response.success = true;
                    }
                }
                
                // Extract usage metadata
                if (j.contains("usage")) {
                    if (j["usage"].contains("total_tokens")) {
                        response.tokens_used = j["usage"]["total_tokens"];
                    }
                }
                break;
                
            case LLMProvider::GEMINI:
                // Gemini format
                if (j.contains("candidates") && !j["candidates"].empty()) {
                    const auto& candidate = j["candidates"][0];
                    if (candidate.contains("content") &&
                        candidate["content"].contains("parts") &&
                        !candidate["content"]["parts"].empty()) {
                        response.content = candidate["content"]["parts"][0]["text"];
                        response.success = true;
                    }
                }
                
                // Extract usage metadata
                if (j.contains("usageMetadata")) {
                    if (j["usageMetadata"].contains("totalTokenCount")) {
                        response.tokens_used = j["usageMetadata"]["totalTokenCount"];
                    }
                }
                break;
        }
        
        if (response.content.empty()) {
            response.error_message = "No content found in response";
            response.success = false;
            logError("No content found in response: " + json_response);
        } else {
            logInfo("Successfully parsed response, tokens used: " + std::to_string(response.tokens_used));
        }
        
    } catch (const json::exception& e) {
        response.error_message = "JSON parsing error: " + std::string(e.what());
        response.success = false;
        logError(response.error_message);
    }
    
    return response;
}

void LLMClient::logError(const std::string& message) const {
    if (config_.verbose) {
        std::cerr << "[LLMClient ERROR] " << message << std::endl;
    }
}

void LLMClient::logInfo(const std::string& message) const {
    if (config_.verbose) {
        std::cout << "[LLMClient INFO] " << message << std::endl;
    }
}

// Token analysis methods implementation
LLMClient::RequestAnalysis LLMClient::analyzeRequest(const std::string& prompt,
                                                    const std::string& system_instruction,
                                                    int max_output_tokens) {
    RequestAnalysis analysis;
    
    // Calculate input tokens
    std::string full_input = prompt;
    if (!system_instruction.empty()) {
        full_input = system_instruction + "\n\n" + prompt;
    }
    
    analysis.input_tokens = utils::TokenCounter::countTokens(full_input);
    analysis.estimated_output_tokens = max_output_tokens > 0 ?
                                     max_output_tokens :
                                     std::min(analysis.input_tokens / 2, config_.max_tokens);
    analysis.model = config_.model;
    
    // Calculate costs
    analysis.usage_details = utils::TokenCounter::calculateUsage(
        full_input, config_.model, analysis.estimated_output_tokens);
    analysis.estimated_cost = analysis.usage_details.total_cost;
    
    // Check limits
    utils::ModelPricing pricing = utils::TokenCounter::getModelPricing(config_.model);
    int total_tokens = analysis.input_tokens + analysis.estimated_output_tokens;
    analysis.within_limits = total_tokens <= pricing.max_context_tokens;
    
    return analysis;
}

void LLMClient::displayTokenUsage(const RequestAnalysis& analysis) {
    std::cout << "\n=== Token Usage Analysis ===" << std::endl;
    std::cout << "Model: " << analysis.model << std::endl;
    std::cout << "Provider: " << utils::TokenCounter::getModelProvider(analysis.model) << std::endl;
    std::cout << "Input tokens: " << analysis.input_tokens << std::endl;
    std::cout << "Estimated output tokens: " << analysis.estimated_output_tokens << std::endl;
    std::cout << "Total estimated tokens: " << (analysis.input_tokens + analysis.estimated_output_tokens) << std::endl;
    std::cout << "Input cost: $" << std::fixed << std::setprecision(6) << analysis.usage_details.input_cost << std::endl;
    std::cout << "Output cost: $" << std::fixed << std::setprecision(6) << analysis.usage_details.output_cost << std::endl;
    std::cout << "Total estimated cost: $" << std::fixed << std::setprecision(6) << analysis.estimated_cost << std::endl;
    
    if (!analysis.within_limits) {
        std::cout << "⚠️  Warning: Request may exceed token limits!" << std::endl;
    }
    
    std::cout << "=============================" << std::endl;
}

bool LLMClient::userConfirmsRequest(const RequestAnalysis& analysis) {
    (void)analysis; // Mark as unused for now, could be used for more detailed prompts
    if (config_.verbose) {
        return true; // Auto-confirm in verbose mode
    }
    
    std::cout << "Do you want to proceed with this request? [y/N]: ";
    std::string response;
    std::getline(std::cin, response);
    
    return !response.empty() && (response[0] == 'y' || response[0] == 'Y');
}


} // namespace llm
} // namespace clion