# CLion Implementation Guide

This document provides detailed technical specifications and code examples for implementing the CLion C++ Agentic CLI Tool.

## Phase 1: Foundation and Core CLI Structure

### 1.1 Project Setup

#### CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.16)
project(clion VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON REQUIRED nlohmann_json)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CLI11 REQUIRED CLI11)
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML-CPP REQUIRED yaml-cpp)

# Firestore SDK (will need custom installation)
# find_package(Firebase REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# Create executable
add_executable(clion ${SOURCES})

# Link libraries
target_link_libraries(clion 
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
    ${CLI11_LIBRARIES}
    ${YAML-CPP_LIBRARIES}
    # Firebase::Firestore  # When integrated
)

# Installation
install(TARGETS clion DESTINATION bin)
```

### 1.2 Argument Parsing with CLI11

#### src/cli/cli_parser.h

```cpp
#pragma once

#include <string>
#include <memory>
#include <CLI/CLI.hpp>

namespace clion {
namespace cli {

struct CLIOptions {
    std::string command;
    std::string file_path;
    bool explain_mode = false;
    std::string fix_command;
    // Additional options as needed
};

class CLIParser {
public:
    CLIParser();
    ~CLIParser() = default;
    
    bool parse(int argc, char** argv);
    const CLIOptions& getOptions() const { return options_; }
    void printHelp() const;

private:
    std::unique_ptr<CLI::App> app_;
    CLIOptions options_;
    
    void setupCommands();
    void setupReviewCommand(CLI::App* review_cmd);
    void setupFixErrorCommand(CLI::App* fix_cmd);
};

} // namespace cli
} // namespace clion
```

#### src/cli/cli_parser.cpp

```cpp
#include "cli_parser.h"
#include <iostream>

namespace clion {
namespace cli {

CLIParser::CLIParser() : app_(std::make_unique<CLI::App>("CLion - C++ Agentic CLI Tool")) {
    setupCommands();
}

bool CLIParser::parse(int argc, char** argv) {
    try {
        app_->parse(argc, argv);
        return true;
    } catch (const CLI::ParseError& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return false;
    }
}

void CLIParser::setupCommands() {
    app_->require_subcommand(1);
    
    // Global options
    app_->add_flag("--explain", options_.explain_mode, "Show detailed reasoning and costs");
    
    // Review command
    auto* review_cmd = app_->add_subcommand("review", "Review and improve code");
    setupReviewCommand(review_cmd);
    
    // Fix-error command
    auto* fix_cmd = app_->add_subcommand("fix-error", "Automatically fix compilation errors");
    setupFixErrorCommand(fix_cmd);
}

void CLIParser::setupReviewCommand(CLI::App* review_cmd) {
    review_cmd->add_option("-f,--file", options_.file_path, "File to review")
        ->required()
        ->check(CLI::ExistingFile);
    
    review_cmd->callback([&]() {
        options_.command = "review";
    });
}

void CLIParser::setupFixErrorCommand(CLI::App* fix_cmd) {
    fix_cmd->add_option("command", options_.fix_command, "Build command to execute and fix")
        ->required();
    
    fix_cmd->callback([&]() {
        options_.command = "fix-error";
    });
}

void CLIParser::printHelp() const {
    std::cout << app_->help() << std::endl;
}

} // namespace cli
} // namespace clion
```

### 1.3 HTTP Client with CURL

#### src/llm/llm_client.h

```cpp
#pragma once

#include <string>
#include <memory>
#include <curl/curl.h>

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
    
    // Initialize with API key
    bool initialize(const std::string& api_key);
    
    // Send request to Gemini API
    LLMResponse sendRequest(const std::string& prompt, 
                           const std::string& system_instruction = "",
                           float temperature = 0.1f);
    
    // Check if client is properly initialized
    bool isInitialized() const { return initialized_; }

private:
    CURL* curl_;
    std::string api_key_;
    bool initialized_;
    
    // CURL callback for writing response
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    // Helper methods
    std::string buildJsonPayload(const std::string& prompt, 
                                const std::string& system_instruction,
                                float temperature) const;
    LLMResponse parseResponse(const std::string& json_response) const;
};

} // namespace llm
} // namespace clion
```

### 1.4 Basic I/O & Diff Utilities

#### src/utils/file_utils.h

```cpp
#pragma once

#include <string>
#include <vector>
#include <optional>

namespace clion {
namespace utils {

class FileUtils {
public:
    // Read entire file content
    static std::optional<std::string> readFile(const std::string& path);
    
    // Write content to file
    static bool writeFile(const std::string& path, const std::string& content);
    
    // Check if file exists
    static bool fileExists(const std::string& path);
    
    // Get file size
    static size_t getFileSize(const std::string& path);
    
    // Get file extension
    static std::string getFileExtension(const std::string& path);
    
    // List files in directory with optional filter
    static std::vector<std::string> listFiles(const std::string& directory, 
                                             const std::string& extension = "");
};

} // namespace utils
} // namespace clion
```

#### src/utils/diff_utils.h

```cpp
#pragma once

#include <string>
#include <vector>

namespace clion {
namespace utils {

struct DiffLine {
    char type;  // '+', '-', ' ', or '@'
    std::string content;
    int old_line = -1;
    int new_line = -1;
};

struct DiffHunk {
    std::vector<DiffLine> lines;
    int old_start = -1;
    int old_count = 0;
    int new_start = -1;
    int new_count = 0;
};

class DiffUtils {
public:
    // Generate unified diff between two strings
    static std::string generateUnifiedDiff(const std::string& original,
                                          const std::string& modified,
                                          const std::string& original_file,
                                          const std::string& modified_file);
    
    // Parse unified diff into hunks
    static std::vector<DiffHunk> parseDiff(const std::string& diff);
    
    // Apply diff to original content
    static std::string applyDiff(const std::string& original, 
                                const std::vector<DiffHunk>& hunks);
    
    // Display diff in a readable format
    static void displayDiff(const std::string& diff);

private:
    // Helper to compute longest common subsequence
    static std::vector<std::pair<int, int>> computeLCS(
        const std::vector<std::string>& lines1,
        const std::vector<std::string>& lines2);
};

} // namespace utils
} // namespace clion
```

## Phase 2: LLM Communication and Context Handling

### 2.1 Gemini API Integration

#### Complete LLMClient Implementation

```cpp
// In src/llm/llm_client.cpp
#include "llm_client.h"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace clion {
namespace llm {

LLMClient::LLMClient() : curl_(nullptr), initialized_(false) {
    curl_ = curl_easy_init();
}

LLMClient::~LLMClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

bool LLMClient::initialize(const std::string& api_key) {
    if (!curl_) {
        return false;
    }
    
    api_key_ = api_key;
    initialized_ = true;
    return true;
}

LLMResponse LLMClient::sendRequest(const std::string& prompt, 
                                  const std::string& system_instruction,
                                  float temperature) {
    LLMResponse response;
    
    if (!initialized_) {
        response.error_message = "LLMClient not initialized";
        return response;
    }
    
    // Build JSON payload
    std::string json_payload = buildJsonPayload(prompt, system_instruction, temperature);
    
    // Set up CURL options
    std::string read_buffer;
    curl_easy_setopt(curl_, CURLOPT_URL, "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent");
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &read_buffer);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string api_key_header = "x-goog-api-key: " + api_key_;
    headers = curl_slist_append(headers, api_key_header.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        response.error_message = "CURL error: " + std::string(curl_easy_strerror(res));
        return response;
    }
    
    // Parse response
    response = parseResponse(read_buffer);
    return response;
}

size_t LLMClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

std::string LLMClient::buildJsonPayload(const std::string& prompt, 
                                       const std::string& system_instruction,
                                       float temperature) const {
    json payload;
    
    // Add system instruction if provided
    if (!system_instruction.empty()) {
        payload["systemInstruction"] = {
            {"parts", {{"text", system_instruction}}}
        };
    }
    
    // Add user content
    payload["contents"] = json::array({
        {
            {"parts", {{"text", prompt}}}
        }
    });
    
    // Set generation config
    payload["generationConfig"] = {
        {"temperature", temperature},
        {"topK", 40},
        {"topP", 0.95},
        {"maxOutputTokens", 8192}
    };
    
    return payload.dump();
}

LLMResponse LLMClient::parseResponse(const std::string& json_response) const {
    LLMResponse response;
    
    try {
        json j = json::parse(json_response);
        
        // Check for errors
        if (j.contains("error")) {
            response.error_message = j["error"]["message"];
            return response;
        }
        
        // Extract content
        if (j.contains("candidates") && !j["candidates"].empty()) {
            const auto& candidate = j["candidates"][0];
            if (candidate.contains("content") && 
                candidate["content"].contains("parts") && 
                !candidate["content"]["parts"].empty()) {
                response.content = candidate["content"]["parts"][0]["text"];
                response.success = true;
            }
        }
        
        // Extract usage metadata if available
        if (j.contains("usageMetadata")) {
            response.tokens_used = j["usageMetadata"]["totalTokenCount"];
        }
        
    } catch (const json::exception& e) {
        response.error_message = "JSON parsing error: " + std::string(e.what());
    }
    
    return response;
}

} // namespace llm
} // namespace clion
```

### 2.2 Context Injection

#### src/llm/context_builder.h

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace clion {
namespace llm {

class ContextBuilder {
public:
    // Build context with file inclusions
    static std::string buildContext(const std::string& base_prompt,
                                   const std::string& project_root = ".");
    
    // Extract file paths from prompt using @file syntax
    static std::vector<std::string> extractFileInclusions(const std::string& prompt);
    
    // Replace @file references with actual file content
    static std::string injectFileContents(const std::string& prompt,
                                        const std::string& project_root = ".");

private:
    // Check if a path should be included based on security rules
    static bool isPathAllowed(const std::string& path);
    
    // Get relative path from project root
    static std::string getRelativePath(const std::string& path, 
                                     const std::string& project_root);
};

} // namespace llm
} // namespace clion
```

## Phase 3: Native C++ Indexing

### 3.1 Project Scanner Utility

#### src/indexer/project_scanner.h

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace clion {
namespace indexer {

struct ScanOptions {
    std::vector<std::string> include_extensions = {".cpp", ".h", ".hpp", ".cc", ".cxx"};
    std::vector<std::string> exclude_patterns = {"build/*", "vendor/*"};
    bool respect_gitignore = true;
    int max_depth = 100;  // -1 for unlimited
};

class ProjectScanner {
public:
    // Scan project directory for source files
    static std::vector<std::filesystem::path> scanProject(
        const std::filesystem::path& project_root,
        const ScanOptions& options = ScanOptions());
    
    // Parse .gitignore file
    static std::unordered_set<std::string> parseGitignore(
        const std::filesystem::path& gitignore_path);
    
    // Check if path matches any exclude pattern
    static bool isExcluded(const std::filesystem::path& path,
                          const std::vector<std::string>& patterns,
                          const std::unordered_set<std::string>& gitignore_rules);

private:
    // Convert glob pattern to regex
    static std::string globToRegex(const std::string& pattern);
    
    // Check if path matches a single pattern
    static bool matchesPattern(const std::filesystem::path& path,
                              const std::string& pattern);
};

} // namespace indexer
} // namespace clion
```

### 3.2 Native Code Indexer

#### src/indexer/code_index.h

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <regex>

namespace clion {
namespace indexer {

struct FunctionInfo {
    std::string name;
    std::string return_type;
    std::vector<std::string> parameters;
    int line_number;
    bool is_virtual = false;
    bool is_static = false;
    std::string access_specifier;  // public, protected, private
};

struct ClassInfo {
    std::string name;
    std::vector<std::string> base_classes;
    std::vector<FunctionInfo> methods;
    int line_number;
    bool is_struct = false;
};

struct FileInfo {
    std::filesystem::path path;
    std::vector<std::string> includes;
    std::vector<FunctionInfo> functions;
    std::vector<ClassInfo> classes;
    std::vector<std::string> namespaces;
    size_t last_modified;
};

using CodeIndex = std::unordered_map<std::string, FileInfo>;

class CodeIndexer {
public:
    // Build index from project files
    static CodeIndex buildIndex(const std::vector<std::filesystem::path>& files);
    
    // Extract information from a single file
    static FileInfo indexFile(const std::filesystem::path& file_path);
    
    // Search for functions/classes in the index
    static std::vector<FunctionInfo> findFunctions(const CodeIndex& index, 
                                                  const std::string& name);
    static std::vector<ClassInfo> findClasses(const CodeIndex& index, 
                                             const std::string& name);
    
    // Save/load index to/from file
    static bool saveIndex(const CodeIndex& index, const std::filesystem::path& output_path);
    static std::optional<CodeIndex> loadIndex(const std::filesystem::path& input_path);

private:
    // Regex patterns for parsing C++ code
    static const std::regex FUNCTION_PATTERN;
    static const std::regex CLASS_PATTERN;
    static const std::regex INCLUDE_PATTERN;
    static const std::regex NAMESPACE_PATTERN;
    
    // Helper methods for parsing
    static std::vector<FunctionInfo> extractFunctions(const std::string& content);
    static std::vector<ClassInfo> extractClasses(const std::string& content);
    static std::vector<std::string> extractIncludes(const std::string& content);
    static std::vector<std::string> extractNamespaces(const std::string& content);
};

} // namespace indexer
} // namespace clion
```

## Phase 4: Compiler Feedback Loop

### 4.1 Command Execution Tool

#### src/compiler/command_executor.h

```cpp
#pragma once

#include <string>
#include <optional>

namespace clion {
namespace compiler {

struct CommandResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
    bool success;
    std::string error_message;
};

class CommandExecutor {
public:
    // Execute command and capture output
    static CommandResult execute(const std::string& command,
                                const std::string& working_directory = ".");
    
    // Execute command with timeout
    static CommandResult executeWithTimeout(const std::string& command,
                                          int timeout_seconds,
                                          const std::string& working_directory = ".");
    
    // Check if command exists in PATH
    static bool commandExists(const std::string& command);

private:
    // Platform-specific implementation
    static CommandResult executePlatform(const std::string& command,
                                       const std::string& working_directory);
};

} // namespace compiler
} // namespace clion
```

### 4.2 Error Parsing Filter

#### src/compiler/error_parser.h

```cpp
#pragma once

#include <string>
#include <vector>
#include <regex>

namespace clion {
namespace compiler {

struct CompilerError {
    std::string file_path;
    int line_number;
    int column;
    std::string severity;  // error, warning, note
    std::string message;
    std::string code_snippet;
};

class ErrorParser {
public:
    // Parse compiler output to extract errors
    static std::vector<CompilerError> parseErrors(const std::string& compiler_output);
    
    // Filter errors by severity
    static std::vector<CompilerError> filterBySeverity(
        const std::vector<CompilerError>& errors,
        const std::string& severity);
    
    // Get code snippet around error line
    static std::string getCodeSnippet(const std::string& file_path, 
                                     int line_number,
                                     int context_lines = 3);

private:
    // Regex patterns for different compilers
    static const std::regex GCC_ERROR_PATTERN;
    static const std::regex CLANG_ERROR_PATTERN;
    static const std::regex MSVC_ERROR_PATTERN;
    
    // Try to match with different compiler patterns
    static std::optional<CompilerError> tryParseGCC(const std::string& line);
    static std::optional<CompilerError> tryParseClang(const std::string& line);
    static std::optional<CompilerError> tryParseMSVC(const std::string& line);
};

} // namespace compiler
} // namespace clion
```

## Phase 5: User Experience and Final Polish

### 5.2 Structured Rules Engine

#### .clionrules.yaml Example

```yaml
# CLion Project Configuration
version: "1.0"

# Project Information
project:
  name: "My C++ Project"
  description: "A sample C++ project with CLion configuration"
  
# API Configuration
api:
  provider: "gemini"
  model: "gemini-pro"
  max_tokens: 8192
  temperature: 0.1
  timeout_seconds: 30

# Indexer Configuration
indexer:
  include_patterns:
    - "*.cpp"
    - "*.h"
    - "*.hpp"
    - "*.cc"
    - "*.cxx"
    - "*.c"
  exclude_patterns:
    - "build/*"
    - "vendor/*"
    - "*.pb.cc"
    - "*.pb.h"
  respect_gitignore: true
  cache_enabled: true
  cache_file: ".clion_cache.json"

# Coding Rules and Conventions
rules:
  - name: "naming_conventions"
    instruction: "Use snake_case for function names and variable names, PascalCase for class names, and UPPER_CASE for constants."
    priority: "high"
    enabled: true
    
  - name: "modern_cpp"
    instruction: "Prefer modern C++ features: use smart pointers instead of raw pointers, range-based for loops, constexpr where possible, and auto keyword for type deduction."
    priority: "medium"
    enabled: true
    
  - name: "error_handling"
    instruction: "Use exceptions for error handling in C++. Prefer RAII pattern and avoid raw memory management."
    priority: "medium"
    enabled: true
    
  - name: "documentation"
    instruction: "Add Doxygen-style comments for all public functions and classes. Include parameter descriptions and return value information."
    priority: "low"
    enabled: true

# Compiler Configuration
compiler:
  default_build_command: "cmake --build ."
  error_patterns:
    - "error:"
    - "fatal error:"
    - "undefined reference"
  max_fix_attempts: 3

# UI/UX Configuration
ui:
  show_token_usage: true
  show_cost_estimate: true
  auto_apply_safe_fixes: false
  diff_context_lines: 3
  confirm_before_applying: true
```

#### src/utils/rules_loader.h

```cpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace clion {
namespace utils {

struct Rule {
    std::string name;
    std::string instruction;
    std::string priority;  // high, medium, low
    bool enabled = true;
};

struct CLionConfig {
    // API settings
    std::string api_provider = "gemini";
    std::string api_model = "gemini-pro";
    int max_tokens = 8192;
    float temperature = 0.1f;
    
    // Indexer settings
    std::vector<std::string> include_patterns;
    std::vector<std::string> exclude_patterns;
    bool respect_gitignore = true;
    bool cache_enabled = true;
    std::string cache_file = ".clion_cache.json";
    
    // Rules
    std::vector<Rule> rules;
    
    // Compiler settings
    std::string default_build_command = "cmake --build .";
    std::vector<std::string> error_patterns;
    int max_fix_attempts = 3;
    
    // UI settings
    bool show_token_usage = true;
    bool show_cost_estimate = true;
    bool auto_apply_safe_fixes = false;
    int diff_context_lines = 3;
    bool confirm_before_applying = true;
};

class RulesLoader {
public:
    // Load configuration from file
    static std::optional<CLionConfig> loadConfig(const std::filesystem::path& config_path);
    
    // Save configuration to file
    static bool saveConfig(const CLionConfig& config, const std::filesystem::path& config_path);
    
    // Get default configuration
    static CLionConfig getDefaultConfig();
    
    // Find config file in project directory
    static std::optional<std::filesystem::path> findConfigFile(const std::filesystem::path& project_root);
    
    // Merge configurations (with file taking precedence)
    static CLionConfig mergeConfigs(const CLionConfig& base, const CLionConfig& override);

private:
    // Parse YAML content
    static CLionConfig parseYaml(const std::string& yaml_content);
    
    // Convert configuration to YAML
    static std::string toYaml(const CLionConfig& config);
    
    // Parse rules from YAML node
    static std::vector<Rule> parseRules(const YAML::Node& rules_node);
};

} // namespace utils
} // namespace clion
```

## Implementation Tips

1. **Error Handling**: Use RAII and exceptions consistently throughout the codebase
2. **Logging**: Implement a logging system for debugging and monitoring
3. **Testing**: Write unit tests for each component, especially the parser and indexer
4. **Performance**: Profile the indexer and optimize for large projects
5. **Security**: Validate all file paths and user inputs to prevent security issues
6. **Modularity**: Keep components loosely coupled for easier testing and maintenance
7. **Documentation**: Use Doxygen for API documentation and maintain clear code comments

## Next Steps

1. Begin with Phase 1 implementation to establish the basic CLI structure
2. Set up the build system and ensure all dependencies can be linked properly
3. Implement each component incrementally with thorough testing
4. Focus on the core LLM integration before moving to advanced features
5. Regularly test with real C++ projects to ensure practical utility