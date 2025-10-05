#include "rules_loader.h"
#include "clion/common.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>

namespace clion {
namespace utils {

std::optional<CLionConfig> RulesLoader::loadConfig(const std::filesystem::path& config_path) {
    try {
        if (!std::filesystem::exists(config_path)) {
            return std::nullopt;
        }

        YAML::Node config = YAML::LoadFile(config_path.string());
        CLionConfig clion_config;

        // Load API settings
        if (config["api"]) {
            auto api = config["api"];
            if (api["provider"]) clion_config.api_provider = api["provider"].as<std::string>();
            if (api["model"]) clion_config.api_model = api["model"].as<std::string>();
            if (api["max_tokens"]) clion_config.max_tokens = api["max_tokens"].as<int>();
            if (api["temperature"]) clion_config.temperature = api["temperature"].as<float>();
        }

        // Load rules
        if (config["rules"]) {
            auto rules_node = config["rules"];
            for (const auto& rule_node : rules_node) {
                Rule rule;
                if (rule_node["name"]) rule.name = rule_node["name"].as<std::string>();
                if (rule_node["instruction"]) rule.instruction = rule_node["instruction"].as<std::string>();
                if (rule_node["priority"]) rule.priority = rule_node["priority"].as<std::string>();
                if (rule_node["enabled"]) rule.enabled = rule_node["enabled"].as<bool>();
                clion_config.rules.push_back(rule);
            }
        }

        // Load file patterns
        if (config["files"]) {
            auto files = config["files"];
            if (files["include_patterns"]) {
                clion_config.include_patterns = files["include_patterns"].as<std::vector<std::string>>();
            }
            if (files["exclude_patterns"]) {
                clion_config.exclude_patterns = files["exclude_patterns"].as<std::vector<std::string>>();
            }
            if (files["respect_gitignore"]) {
                clion_config.respect_gitignore = files["respect_gitignore"].as<bool>();
            }
        }

        // Load build settings
        if (config["build"]) {
            auto build = config["build"];
            if (build["default_command"]) {
                clion_config.default_build_command = build["default_command"].as<std::string>();
            }
        }

        // Load behavior settings
        if (config["behavior"]) {
            auto behavior = config["behavior"];
            if (behavior["max_fix_attempts"]) {
                clion_config.max_fix_attempts = behavior["max_fix_attempts"].as<int>();
            }
            if (behavior["show_token_usage"]) {
                clion_config.show_token_usage = behavior["show_token_usage"].as<bool>();
            }
            if (behavior["show_cost_estimate"]) {
                clion_config.show_cost_estimate = behavior["show_cost_estimate"].as<bool>();
            }
            if (behavior["auto_apply_safe_fixes"]) {
                clion_config.auto_apply_safe_fixes = behavior["auto_apply_safe_fixes"].as<bool>();
            }
            if (behavior["confirm_before_applying"]) {
                clion_config.confirm_before_applying = behavior["confirm_before_applying"].as<bool>();
            }
            if (behavior["diff_context_lines"]) {
                clion_config.diff_context_lines = behavior["diff_context_lines"].as<int>();
            }
        }

        return clion_config;

    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing YAML config file: " << e.what() << std::endl;
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool RulesLoader::saveConfig(const CLionConfig& config, const std::filesystem::path& config_path) {
    try {
        YAML::Node yaml_config;

        // API settings
        YAML::Node api;
        api["provider"] = config.api_provider;
        api["model"] = config.api_model;
        api["max_tokens"] = config.max_tokens;
        api["temperature"] = config.temperature;
        yaml_config["api"] = api;

        // Rules
        YAML::Node rules_node;
        for (const auto& rule : config.rules) {
            YAML::Node rule_node;
            rule_node["name"] = rule.name;
            rule_node["instruction"] = rule.instruction;
            rule_node["priority"] = rule.priority;
            rule_node["enabled"] = rule.enabled;
            rules_node.push_back(rule_node);
        }
        yaml_config["rules"] = rules_node;

        // File patterns
        YAML::Node files;
        files["include_patterns"] = config.include_patterns;
        files["exclude_patterns"] = config.exclude_patterns;
        files["respect_gitignore"] = config.respect_gitignore;
        yaml_config["files"] = files;

        // Build settings
        YAML::Node build;
        build["default_command"] = config.default_build_command;
        yaml_config["build"] = build;

        // Behavior settings
        YAML::Node behavior;
        behavior["max_fix_attempts"] = config.max_fix_attempts;
        behavior["show_token_usage"] = config.show_token_usage;
        behavior["show_cost_estimate"] = config.show_cost_estimate;
        behavior["auto_apply_safe_fixes"] = config.auto_apply_safe_fixes;
        behavior["confirm_before_applying"] = config.confirm_before_applying;
        behavior["diff_context_lines"] = config.diff_context_lines;
        yaml_config["behavior"] = behavior;

        // Write to file
        std::ofstream fout(config_path);
        if (!fout.is_open()) {
            return false;
        }

        fout << yaml_config;
        return fout.good();

    } catch (const YAML::Exception& e) {
        std::cerr << "Error serializing YAML config: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

CLionConfig RulesLoader::getDefaultConfig() {
    CLionConfig config;

    // Default API settings
    config.api_provider = "gemini";
    config.api_model = "gemini-pro";
    config.max_tokens = 8192;
    config.temperature = 0.1f;

    // Default file patterns
    config.include_patterns = {"*.cpp", "*.hpp", "*.h", "*.cc", "*.cxx"};
    config.exclude_patterns = {"build/*", "cmake-build-*/*", "*.o", "*.so", "*.a"};
    config.respect_gitignore = true;

    // Default rules for C++ development
    config.rules = {
        {"naming_conventions", "Use snake_case for function names and variables", "high", true},
        {"include_guards", "Use #pragma once instead of traditional include guards", "medium", true},
        {"const_correctness", "Use const wherever possible for parameters and variables", "high", true},
        {"error_handling", "Always check return values and handle errors appropriately", "high", true},
        {"memory_management", "Use smart pointers instead of raw pointers when possible", "high", true},
        {"documentation", "Add meaningful comments for complex logic and public APIs", "medium", true}
    };

    // Default build settings
    config.default_build_command = "cmake --build .";

    // Default behavior
    config.max_fix_attempts = 3;
    config.show_token_usage = true;
    config.show_cost_estimate = true;
    config.auto_apply_safe_fixes = false;
    config.diff_context_lines = 3;
    config.confirm_before_applying = true;

    return config;
}

std::optional<std::filesystem::path> RulesLoader::findConfigFile(const std::filesystem::path& project_root) {
    // Look for .clionrules.yaml in project root
    std::filesystem::path config_path = project_root / ".clionrules.yaml";
    if (std::filesystem::exists(config_path)) {
        return config_path;
    }

    // Look for .clionrules.yml as alternative
    config_path = project_root / ".clionrules.yml";
    if (std::filesystem::exists(config_path)) {
        return config_path;
    }

    return std::nullopt;
}

} // namespace utils
} // namespace clion
