#include "cli_parser.h"
#include <iostream>

namespace clion {
namespace cli {

CLIParser::CLIParser() : app_(std::make_unique<CLI::App>(CLION_DESCRIPTION, "clion")) {
    setupCommands();
}

bool CLIParser::parse(int argc, char** argv) {
    try {
        app_->parse(argc, argv);
        return true;
    } catch (const CLI::ParseError& e) {
        if (e.get_exit_code() != 0) {
            std::cerr << "Parse error: " << e.what() << std::endl;
            if (e.get_exit_code() == CLI::ExitCodes::CalledForHelp) {
                options_.help = true;
            } else if (e.get_exit_code() == CLI::ExitCodes::CalledForVersion) {
                options_.version = true;
            }
        }
        return false;
    }
}

void CLIParser::setupCommands() {
    // Require at least one subcommand
    app_->require_subcommand(1);
    
    // Setup global options
    setupGlobalOptions();
    
    // Review command
    auto* review_cmd = app_->add_subcommand("review", "Review and improve code");
    setupReviewCommand(review_cmd);
    
    // Fix-error command
    auto* fix_cmd = app_->add_subcommand("fix-error", "Automatically fix compilation errors");
    setupFixErrorCommand(fix_cmd);
}

void CLIParser::setupGlobalOptions() {
    app_->add_flag("-v,--verbose", options_.verbose, "Enable verbose output")
        ->default_val(false);
    
    app_->add_option("-c,--config", options_.config_file, "Path to configuration file")
        ->default_val(clion::constants::DEFAULT_CONFIG_FILE)
        ->check(CLI::ExistingFile);
    
    app_->add_flag("--explain", options_.explain_mode, "Show detailed reasoning and costs")
        ->default_val(false);
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

void CLIParser::printVersion() const {
    std::cout << CLION_NAME << " version " << CLION_VERSION << std::endl;
    std::cout << "C++ Agentic CLI Tool" << std::endl;
}

} // namespace cli
} // namespace clion