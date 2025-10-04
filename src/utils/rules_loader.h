#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "clion/common.h"

namespace clion {
namespace utils {

struct Rule {
    std::string name;
    std::string instruction;
    std::string priority;
    bool enabled = true;
};

struct CLionConfig {
    std::string api_provider = "gemini";
    std::string api_model = "gemini-pro";
    int max_tokens = 8192;
    float temperature = 0.1f;
    std::vector<std::string> include_patterns;
    std::vector<std::string> exclude_patterns;
    bool respect_gitignore = true;
    std::vector<Rule> rules;
    std::string default_build_command = "cmake --build .";
    std::vector<std::string> error_patterns;
    int max_fix_attempts = 3;
    bool show_token_usage = true;
    bool show_cost_estimate = true;
    bool auto_apply_safe_fixes = false;
    int diff_context_lines = 3;
    bool confirm_before_applying = true;
};

class RulesLoader {
public:
    static std::optional<CLionConfig> loadConfig(const std::filesystem::path& config_path);
    static bool saveConfig(const CLionConfig& config, const std::filesystem::path& config_path);
    static CLionConfig getDefaultConfig();
    static std::optional<std::filesystem::path> findConfigFile(const std::filesystem::path& project_root);
};

} // namespace utils
} // namespace clion
