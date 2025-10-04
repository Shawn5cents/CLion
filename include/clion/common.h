#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iomanip>

// Common macros
#define CLION_VERSION "1.0.0"
#define CLION_NAME "CLion"
#define CLION_DESCRIPTION "C++ Agentic CLI Tool"

// Common namespaces
namespace clion {

// Common types
using string = std::string;
using string_view = std::string_view;
using path = std::filesystem::path;

// Common exceptions
class CLionException : public std::runtime_error {
public:
    explicit CLionException(const string& message) : std::runtime_error(message) {}
};

class APIException : public CLionException {
public:
    explicit APIException(const string& message) : CLionException(message) {}
};

class FileException : public CLionException {
public:
    explicit FileException(const string& message) : CLionException(message) {}
};

class ParseException : public CLionException {
public:
    explicit ParseException(const string& message) : CLionException(message) {}
};

// Common constants
namespace constants {
    constexpr int DEFAULT_MAX_TOKENS = 8192;
    constexpr float DEFAULT_TEMPERATURE = 0.1f;
    constexpr int DEFAULT_MAX_FIX_ATTEMPTS = 3;
    constexpr int DEFAULT_DIFF_CONTEXT_LINES = 3;
    
    const std::string DEFAULT_CONFIG_FILE = ".clionrules.yaml";
    const std::string DEFAULT_CACHE_FILE = ".clion_cache.json";
    const std::string DEFAULT_SESSION_FILE = ".clion_session.json";
    
    const std::vector<std::string> DEFAULT_INCLUDE_PATTERNS = {
        "*.cpp", "*.h", "*.hpp", "*.cc", "*.cxx", "*.c"
    };
    
    const std::vector<std::string> DEFAULT_EXCLUDE_PATTERNS = {
        "build/*", "vendor/*", "*.pb.cc", "*.pb.h"
    };
}

// Common utility functions
namespace utils {
    inline string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    inline string trim(const string& str) {
        const string whitespace = " \t\n\r\f\v";
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos) return "";
        
        const auto strEnd = str.find_last_not_of(whitespace);
        const auto strRange = strEnd - strBegin + 1;
        
        return str.substr(strBegin, strRange);
    }
    
    inline bool startsWith(const string& str, const string& prefix) {
        return str.rfind(prefix, 0) == 0;
    }
    
    inline bool endsWith(const string& str, const string& suffix) {
        if (suffix.size() > str.size()) return false;
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }
    
    inline std::vector<string> split(const string& str, char delimiter) {
        std::vector<string> tokens;
        std::stringstream ss(str);
        string token;
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
}

} // namespace clion