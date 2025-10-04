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
    void setupReviewCommand(CLI::App* review_cmd);
    void setupFixErrorCommand(CLI::App* fix_cmd);
    void setupGlobalOptions();
};

} // namespace cli
} // namespace clion