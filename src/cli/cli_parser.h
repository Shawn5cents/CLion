#pragma once

#include <string>
#include <memory>
#include <CLI/CLI.hpp>
#include "clion/common.h"

namespace clion {
namespace cli {

struct CLIOptions {
    std::string command;
    std::string file_path;
    bool explain_mode = false;
    std::string fix_command;
    std::string config_file = clion::constants::DEFAULT_CONFIG_FILE;
    bool verbose = false;
    bool version = false;
    bool help = false;
    bool non_interactive = false;

    // General prompt option for @file syntax support
    std::string prompt_text;

    // NLP Options
    std::string nlp_action;
    std::string nlp_text;
    std::string nlp_error;
    bool nlp_sentiment = false;
    bool nlp_complexity = false;
    bool nlp_interactive = false;
    bool nlp_analyze_code = false;
    std::string nlp_generate;

    // Generate Command Options
    std::string generate_prompt;
    std::string output_file;
    bool generate_interactive = false;
    std::vector<std::string> generate_files;

    // Transform Command Options
    std::string transform_prompt;
    std::string transform_file;

    // Scaffold Command Options
    std::string scaffold_prompt;
};

class CLIParser {
public:
    CLIParser();
    ~CLIParser() = default;
    
    bool parse(int argc, char** argv);
    const CLIOptions& getOptions() const { return options_; }
    void printHelp() const;
    void printVersion() const;

private:
    std::unique_ptr<CLI::App> app_;
    CLIOptions options_;

    void setupCommands();
    void setupPromptCommand(CLI::App* prompt_cmd);
    void setupReviewCommand(CLI::App* review_cmd);
    void setupFixCommand(CLI::App* fix_cmd);
    void setupGenerateCommand(CLI::App* generate_cmd);
    void setupTransformCommand(CLI::App* transform_cmd);
    void setupScaffoldCommand(CLI::App* scaffold_cmd);
    void setupNLPCommand(CLI::App* nlp_cmd);
    void setupGlobalOptions();
};

} // namespace cli
} // namespace clion