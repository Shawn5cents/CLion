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
        // CLI11 automatically handles help and version flags
        // Return success for help/version requests, failure for actual errors
        if (e.get_exit_code() == 0 || e.get_exit_code() == 1 || e.get_exit_code() == 2) {
            // Help or version was requested - this is not an error
            if (e.get_exit_code() == 1) {
                options_.help = true;
            } else if (e.get_exit_code() == 2) {
                options_.version = true;
            }
            return true;
        }
        
        // Actual parse error
        std::cerr << "Parse error: " << e.what() << std::endl;
        return false;
    }
}

void CLIParser::setupCommands() {
    // Setup global options first
    setupGlobalOptions();

    // Prompt command - general LLM interaction with @file support
    auto* prompt_cmd = app_->add_subcommand("prompt", "Send a prompt to LLM with @file support");
    setupPromptCommand(prompt_cmd);

    // Review command
    auto* review_cmd = app_->add_subcommand("review", "Review and improve code");
    setupReviewCommand(review_cmd);

    // Fix-error command
    auto* fix_cmd = app_->add_subcommand("fix", "Automatically fix errors from any command");
    setupFixCommand(fix_cmd);

    // Generate command
    auto* generate_cmd = app_->add_subcommand("generate", "Generate code or text from a prompt");
    setupGenerateCommand(generate_cmd);

    // Transform command
    auto* transform_cmd = app_->add_subcommand("transform", "Transform code based on a prompt");
    setupTransformCommand(transform_cmd);

    // Scaffold command
    auto* scaffold_cmd = app_->add_subcommand("scaffold", "Scaffold a new project from a prompt");
    setupScaffoldCommand(scaffold_cmd);

    // NLP command
    auto* nlp_cmd = app_->add_subcommand("nlp", "Natural Language Processing utilities");
    setupNLPCommand(nlp_cmd);

    // Don't require subcommands - allow help/version to work without subcommands
    app_->require_subcommand(0, 1);
}

void CLIParser::setupGenerateCommand(CLI::App* generate_cmd) {
    generate_cmd->add_option("-p,--prompt", options_.generate_prompt, "Prompt for code generation");
    generate_cmd->add_option("-o,--output", options_.output_file, "Output file path");
    generate_cmd->add_flag("-i,--interactive", options_.generate_interactive, "Interactive mode");
    generate_cmd->add_option("-f,--files", options_.generate_files, "Files to use as context");
    
    generate_cmd->callback([&]() {
        options_.command = "generate";
    });
}

void CLIParser::setupScaffoldCommand(CLI::App* scaffold_cmd) {
    scaffold_cmd->add_option("-p,--prompt", options_.scaffold_prompt, "Prompt for project scaffolding")
        ->required();
    
    scaffold_cmd->callback([&]() {
        options_.command = "scaffold";
    });
}

void CLIParser::setupTransformCommand(CLI::App* transform_cmd) {
    transform_cmd->add_option("-p,--prompt", options_.transform_prompt, "Prompt for code transformation")
        ->required();
    transform_cmd->add_option("-f,--file", options_.transform_file, "File to transform");
    
    transform_cmd->callback([&]() {
        options_.command = "transform";
    });
}

// Add new method
void CLIParser::setupNLPCommand(CLI::App* nlp_cmd) {
    // NLP actions
    auto* analyze_cmd = nlp_cmd->add_subcommand("analyze", "Analyze code or text");
    analyze_cmd->add_option("-f,--file", options_.file_path, "File to analyze");
    analyze_cmd->add_option("--text", options_.nlp_text, "Text to analyze");
    analyze_cmd->add_flag("--sentiment", options_.nlp_sentiment, "Sentiment analysis");
    analyze_cmd->add_flag("--complexity", options_.nlp_complexity, "Complexity analysis");
    analyze_cmd->callback([&]() { options_.command = "nlp"; options_.nlp_action = "analyze"; });
    
    auto* interpret_cmd = nlp_cmd->add_subcommand("interpret", "Interpret error messages");
    interpret_cmd->add_option("--error", options_.nlp_error, "Error message to interpret");
    interpret_cmd->callback([&]() { options_.command = "nlp"; options_.nlp_action = "interpret"; });
    
    auto* suggest_cmd = nlp_cmd->add_subcommand("suggest", "Suggest commands from natural language");
    suggest_cmd->add_option("description", options_.nlp_text, "Natural language description");
    suggest_cmd->add_flag("-i,--interactive", options_.nlp_interactive, "Interactive mode");
    suggest_cmd->callback([&]() { options_.command = "nlp"; options_.nlp_action = "suggest"; });
    
    auto* summarize_cmd = nlp_cmd->add_subcommand("summarize", "Generate summaries");
    summarize_cmd->add_option("-f,--file", options_.file_path, "File to summarize");
    summarize_cmd->callback([&]() { options_.command = "nlp"; options_.nlp_action = "summarize"; });

    auto* analyze_code_cmd = nlp_cmd->add_subcommand("analyze-code", "Analyze code using the CodeAnalyzer");
    analyze_code_cmd->add_option("-f,--file", options_.file_path, "File to analyze")->required();
    analyze_code_cmd->callback([&]() { options_.command = "nlp"; options_.nlp_action = "analyze-code"; });

    auto* generate_cmd = nlp_cmd->add_subcommand("generate", "Generate C++ code from a natural language description");
    generate_cmd->add_option("description", options_.nlp_generate, "Natural language description of the code to generate")->required();
    generate_cmd->callback([&]() { options_.command = "nlp"; options_.nlp_action = "generate"; });
}

void CLIParser::setupGlobalOptions() {
    app_->add_flag("-v,--verbose", options_.verbose, "Enable verbose output")
        ->default_val(false);
    
    app_->add_option("-c,--config", options_.config_file, "Path to configuration file")
        ->default_val(clion::constants::DEFAULT_CONFIG_FILE)
        ->check(CLI::ExistingFile);
    
    app_->add_flag("--explain", options_.explain_mode, "Show detailed reasoning and costs")
        ->default_val(false);
    
    app_->add_flag("--version", options_.version, "Show version information")
        ->default_val(false);
}

void CLIParser::setupReviewCommand(CLI::App* review_cmd) {
    review_cmd->add_option("-f,--file", options_.file_path, "File to review")
        ->required()
        ->check(CLI::ExistingFile);
    review_cmd->add_flag("--non-interactive", options_.non_interactive, "Run in non-interactive mode");
    
    review_cmd->callback([&]() {
        options_.command = "review";
    });
}

void CLIParser::setupPromptCommand(CLI::App* prompt_cmd) {
    prompt_cmd->add_option("text", options_.prompt_text, "Prompt text that can include @file <path> syntax")
        ->required();

    prompt_cmd->callback([&]() {
        options_.command = "prompt";
    });
}

void CLIParser::setupFixCommand(CLI::App* fix_cmd) {
    fix_cmd->add_option("command", options_.fix_command, "Build command to execute and fix")
        ->required();

    fix_cmd->callback([&]() {
        options_.command = "fix";
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