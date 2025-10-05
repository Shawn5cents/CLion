#include <iostream>
#include <memory>
#include "cli/cli_parser.h"
#include "cli/interaction.h"
#include "ui/ui_manager.h"
#include "compiler/command_executor.h"
#include "compiler/enhanced_command_executor.h"
#include "compiler/error_parser.h"
#include "nlp/text_analyzer.h"
#include "nlp/command_interpreter.h"
#include "nlp/code_analyzer.h"
#include "nlp/error_interpreter.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "utils/rules_loader.h"
#include "llm/llm_client.h"
#include "llm/prompts.h"
#include "llm/context_builder.h"
#include "nlohmann/json.hpp"

// Global configuration
clion::utils::CLionConfig g_clion_config;

std::string buildSystemInstructions(const clion::utils::CLionConfig& config) {
    std::string instructions = "You are CLion, an AI-powered C++ development assistant. ";

    if (!config.rules.empty()) {
        instructions += "\n\nProject-specific coding conventions (HIGH PRIORITY - follow these rules):";
        for (const auto& rule : config.rules) {
            if (rule.enabled) {
                instructions += "\n- " + rule.name + " (" + rule.priority + "): " + rule.instruction;
            }
        }
    }

    instructions += "\n\nGeneral C++ best practices:";
    instructions += "\n- Write clean, readable, and maintainable code";
    instructions += "\n- Follow modern C++ standards (C++11/14/17/20)";
    instructions += "\n- Use RAII and smart pointers appropriately";
    instructions += "\n- Handle errors gracefully";
    instructions += "\n- Write self-documenting code with meaningful names";

    return instructions;
}

int main(int argc, char** argv) {
    // Load configuration
    std::filesystem::path current_path = std::filesystem::current_path();
    auto config_path = clion::utils::RulesLoader::findConfigFile(current_path);
    if (config_path) {
        auto loaded_config = clion::utils::RulesLoader::loadConfig(*config_path);
        if (loaded_config) {
            g_clion_config = *loaded_config;
        } else {
            g_clion_config = clion::utils::RulesLoader::getDefaultConfig();
        }
    } else {
        g_clion_config = clion::utils::RulesLoader::getDefaultConfig();
    }

    // Initialize LLM Client
    clion::llm::LLMClient llm_client;
    const char* api_key = std::getenv("OPENROUTER_API_KEY");
    if (api_key != nullptr) {
        clion::llm::LLMConfig config;
        config.api_key = api_key;
        config.provider = g_clion_config.api_provider == "gemini" ?
                         clion::llm::LLMProvider::GEMINI : clion::llm::LLMProvider::OPENROUTER;
        config.model = g_clion_config.api_model;
        config.max_tokens = g_clion_config.max_tokens;
        config.temperature = g_clion_config.temperature;
        llm_client.initialize(config);
    }

    try {
        // Create CLI parser
        clion::cli::CLIParser parser;
        
        // Parse command line arguments
        if (!parser.parse(argc, argv)) {
            parser.printHelp();
            return 1;
        }
        
        const auto& options = parser.getOptions();
        
        // Handle help and version flags first
        if (options.help) {
            parser.printHelp();
            return 0;
        }
        
        if (options.version) {
            parser.printVersion();
            return 0;
        }
        
        // Handle case where no subcommand was provided
        if (options.command.empty()) {
            std::cerr << "Error: No command specified" << std::endl;
            parser.printHelp();
            return 1;
        }
        
        // Handle different commands
        if (options.command == "prompt") {
            if (llm_client.isInitialized()) {
                std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(options.prompt_text);
                auto llm_response = llm_client.sendRequest(enhanced_prompt);
                if (llm_response.success) {
                    std::cout << "LLM Response:" << std::endl;
                    std::cout << llm_response.content << std::endl;
                } else {
                    std::cerr << "Error: " << llm_response.error_message << std::endl;
                }
            } else {
                std::cout << "Prompt command selected with text: " << options.prompt_text << std::endl;
                if (options.explain_mode) {
                    std::cout << "Explain mode enabled" << std::endl;
                }
                // TODO: Implement prompt functionality without LLM
            }
            return 0;
        }
        else if (options.command == "scaffold") {
            if (llm_client.isInitialized()) {
                clion::cli::InteractionHandler::showInfo("Scaffolding project...");

                // 1. Get file structure from LLM
                std::string file_structure_prompt = "You are a project scaffolding expert. Based on the following prompt, generate a JSON object representing the file structure. The keys should be the file paths and the values should be a brief description of each file's purpose. Prompt: " + options.scaffold_prompt;
                std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(file_structure_prompt);
                auto llm_response = llm_client.sendRequest(enhanced_prompt);

                if (!llm_response.success) {
                    clion::cli::InteractionHandler::showError("Failed to get file structure from LLM: " + llm_response.error_message);
                    return 1;
                }

                // 2. Parse the JSON response
                try {
                    nlohmann::json file_structure = nlohmann::json::parse(llm_response.content);

                    // 3. Create directories and files
                    for (auto& [file_path, description] : file_structure.items()) {
                        // Create parent directories
                        std::filesystem::path path(file_path);
                        if (path.has_parent_path()) {
                            std::filesystem::create_directories(path.parent_path());
                        }

                        // 4. Generate file content
                        std::string file_content_prompt = "Generate the code for the file '" + file_path + "'. The file's purpose is: " + description.get<std::string>();
                        std::string enhanced_content_prompt = clion::llm::ContextBuilder::buildContext(file_content_prompt);
                        auto content_response = llm_client.sendRequest(enhanced_content_prompt);

                        if (content_response.success) {
                            // 5. Save file content
                            if (clion::utils::FileUtils::writeFile(file_path, content_response.content)) {
                                clion::cli::InteractionHandler::showInfo("Created file: " + file_path);
                            } else {
                                clion::cli::InteractionHandler::showError("Failed to write to file: " + file_path);
                            }
                        } else {
                            clion::cli::InteractionHandler::showError("Failed to generate content for file: " + file_path);
                        }
                    }

                    clion::cli::InteractionHandler::showSuccess("Project scaffolding completed successfully!");

                } catch (const nlohmann::json::parse_error& e) {
                    clion::cli::InteractionHandler::showError("Failed to parse file structure JSON: " + std::string(e.what()));
                    return 1;
                }

            } else {
                clion::cli::InteractionHandler::showError("LLM client not initialized. Please set OPENROUTER_API_KEY environment variable.");
                return 1;
            }
            return 0;
        }
        else if (options.command == "transform") {
            if (llm_client.isInitialized()) {
                std::string original_content;
                if (!options.transform_file.empty()) {
                    auto content_opt = clion::utils::FileUtils::readFile(options.transform_file);
                    if (content_opt) {
                        original_content = *content_opt;
                    } else {
                        clion::cli::InteractionHandler::showError("Cannot read file: " + options.transform_file);
                        return 1;
                    }
                }

                std::string prompt = options.transform_prompt;
                if (!original_content.empty()) {
                    prompt += "\n\nOriginal code:\n```\n" + original_content + "\n```";
                }

                std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(prompt);
                auto llm_response = llm_client.sendRequest(enhanced_prompt);

                if (llm_response.success) {
                    std::string transformed_code = clion::utils::StringUtils::extractCodeFromBlock(llm_response.content);
                    if (transformed_code.empty()) {
                        transformed_code = llm_response.content;
                    }

                    if (!options.transform_file.empty()) {
                        clion::cli::InteractionHandler::showInfo("Proposed changes:");
                        std::istringstream orig_stream(original_content);
                        std::istringstream trans_stream(transformed_code);
                        std::string orig_line, trans_line;

                        auto& colors = clion::ui::UIManager::getInstance().getColorManager();
                        std::cout << colors.muted("--- Original ---") << std::endl;
                        while (std::getline(orig_stream, orig_line)) {
                            std::cout << colors.error("  " + orig_line) << std::endl;
                        }
                        std::cout << colors.muted("--- Transformed ---") << std::endl;
                        while (std::getline(trans_stream, trans_line)) {
                            std::cout << colors.success("  " + trans_line) << std::endl;
                        }

                        if (clion::cli::InteractionHandler::getConfirmation("Apply these changes?")) {
                            if (clion::utils::FileUtils::writeFile(options.transform_file, transformed_code)) {
                                clion::cli::InteractionHandler::showSuccess("Transformation applied successfully!");
                            } else {
                                clion::cli::InteractionHandler::showError("Failed to write to file: " + options.transform_file);
                            }
                        } else {
                            clion::cli::InteractionHandler::showInfo("Transformation skipped.");
                        }
                    } else {
                        std::cout << transformed_code << std::endl;
                    }
                } else {
                    std::cerr << "Error: " << llm_response.error_message << std::endl;
                }
            } else {
                clion::cli::InteractionHandler::showError("LLM client not initialized. Please set OPENROUTER_API_KEY environment variable.");
                return 1;
            }
            return 0;
        }
        else if (options.command == "generate") {
            if (llm_client.isInitialized()) {
                if (options.generate_interactive) {
                    clion::cli::InteractionHandler::showInfo("Entering interactive generation mode. Type 'exit' or 'quit' to end.");
                    std::string user_input;
                    while (true) {
                        user_input = clion::cli::InteractionHandler::getUserInput("> ");
                        if (user_input == "exit" || user_input == "quit") {
                            break;
                        }
                        std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(user_input);
                        auto llm_response = llm_client.sendRequest(enhanced_prompt);
                        if (llm_response.success) {
                            std::cout << llm_response.content << std::endl;
                        } else {
                            std::cerr << "Error: " << llm_response.error_message << std::endl;
                        }
                    }
                } else {
                    std::string context_files;
                    for (const auto& file_path : options.generate_files) {
                        auto content_opt = clion::utils::FileUtils::readFile(file_path);
                        if (content_opt) {
                            context_files += "\n\n---\nFile: " + file_path + "\n---\n" + *content_opt;
                        } else {
                            clion::cli::InteractionHandler::showWarning("Could not read file: " + file_path);
                        }
                    }

                    std::string prompt_with_context = options.generate_prompt + context_files;
                    std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(prompt_with_context);
                    auto llm_response = llm_client.sendRequest(enhanced_prompt);
                    if (llm_response.success) {
                        if (!options.output_file.empty()) {
                            if (clion::utils::FileUtils::writeFile(options.output_file, llm_response.content)) {
                                clion::cli::InteractionHandler::showSuccess("Code generated successfully and saved to " + options.output_file);
                            } else {
                                clion::cli::InteractionHandler::showError("Failed to write to output file: " + options.output_file);
                            }
                        } else {
                            std::cout << llm_response.content << std::endl;
                        }
                    } else {
                        std::cerr << "Error: " << llm_response.error_message << std::endl;
                    }
                }
            } else {
                clion::cli::InteractionHandler::showError("LLM client not initialized. Please set OPENROUTER_API_KEY environment variable.");
                return 1;
            }
            return 0;
        }
        else if (options.command == "review") {
            // Initialize UI components
            auto& ui = clion::ui::UIManager::getInstance();
            ui.initialize();
            auto& colors = ui.getColorManager();

            ui.showHeader("🔍 CLion Code Review");
            std::cout << colors.info("File: ") << colors.primary(options.file_path) << std::endl;

            if (!llm_client.isInitialized()) {
                clion::cli::InteractionHandler::showError("LLM client not initialized. Please set OPENROUTER_API_KEY environment variable.");
                return 1;
            }

            if (options.file_path.empty()) {
                clion::cli::InteractionHandler::showError("No file specified for review. Use --file <path>");
                return 1;
            }

            // Check if file exists
            if (!clion::utils::FileUtils::fileExists(options.file_path)) {
                clion::cli::InteractionHandler::showError("File does not exist: " + options.file_path);
                return 1;
            }

            // Load original file content
            auto original_content_opt = clion::utils::FileUtils::readFile(options.file_path);
            if (!original_content_opt) {
                clion::cli::InteractionHandler::showError("Cannot read file: " + options.file_path);
                return 1;
            }
            std::string original_content = *original_content_opt;

            const int MAX_ITERATIONS = 3;
            int iteration = 0;
            bool review_complete = false;

            while (iteration < MAX_ITERATIONS && !review_complete) {
                iteration++;
                clion::cli::InteractionHandler::showInfo("Review iteration " + std::to_string(iteration) + "/" + std::to_string(MAX_ITERATIONS));

                // Build review prompt
                std::string system_instruction = buildSystemInstructions(g_clion_config);

                std::string base_prompt = "Please analyze this C++ code and provide specific improvement suggestions.\n";
                base_prompt += "Focus on: code quality, best practices, performance, maintainability, and potential bugs.\n\n";

                if (iteration > 1) {
                    base_prompt += "Previous review iteration " + std::to_string(iteration - 1) + " completed.\n";
                }

                std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(base_prompt + "@file " + options.file_path);

                clion::cli::InteractionHandler::showInfo("Analyzing code with AI...");

                auto llm_response = llm_client.sendRequest(enhanced_prompt);

                if (!llm_response.success) {
                    clion::cli::InteractionHandler::showError("Failed to get AI review: " + llm_response.error_message);
                    return 1;
                }

                // Display AI review
                ui.showHeader("AI Code Review Results");
                std::cout << colors.primary("Review Summary:") << std::endl;
                std::cout << llm_response.content << std::endl;

                // Check if AI suggests code changes
                bool has_suggestions = llm_response.content.find("```") != std::string::npos ||
                                     llm_response.content.find("suggestion") != std::string::npos ||
                                     llm_response.content.find("change") != std::string::npos;

                if (!has_suggestions) {
                    clion::cli::InteractionHandler::showInfo("AI review completed - no code changes suggested.");
                    review_complete = true;
                    break;
                }

                // Extract suggested code changes
                std::string suggested_code = clion::utils::StringUtils::extractCodeFromBlock(llm_response.content);

                if (suggested_code.empty()) {
                    clion::cli::InteractionHandler::showInfo("AI provided suggestions but no code changes were proposed.");
                    review_complete = true;
                    break;
                }

                // Show proposed changes
                ui.showHeader("Proposed Code Changes");

                // Show simple diff preview
                clion::cli::InteractionHandler::showInfo("Proposed changes preview:");
                std::istringstream orig_stream(original_content);
                std::istringstream sugg_stream(suggested_code);
                std::string orig_line, sugg_line;

                std::cout << colors.muted("--- Original (first 5 lines) ---") << std::endl;
                for (int i = 0; i < 5 && std::getline(orig_stream, orig_line); ++i) {
                    std::cout << colors.error("  " + orig_line) << std::endl;
                }

                std::cout << colors.muted("--- Suggested (first 5 lines) ---") << std::endl;
                for (int i = 0; i < 5 && std::getline(sugg_stream, sugg_line); ++i) {
                    std::cout << colors.success("  " + sugg_line) << std::endl;
                }

                // Interactive approval loop
                bool valid_choice = false;
                while (!valid_choice) {
                    std::cout << std::endl;
                    if (options.non_interactive) {
                        clion::cli::InteractionHandler::showInfo("Non-interactive mode: Applying changes automatically.");
                        if (clion::utils::FileUtils::writeFile(options.file_path, suggested_code)) {
                            clion::cli::InteractionHandler::showSuccess("✅ Code review changes applied successfully!");
                            original_content = suggested_code;  // Update for next iteration
                            valid_choice = true;
                        } else {
                            clion::cli::InteractionHandler::showError("❌ Failed to apply changes to file");
                            valid_choice = true;
                            review_complete = true;
                        }
                        continue;
                    }

                    std::cout << colors.bold("Choose action:") << std::endl;
                    std::cout << colors.primary("[A]") << " Apply changes" << std::endl;
                    std::cout << colors.warning("[S]") << " Skip changes" << std::endl;
                    std::cout << colors.info("[E]") << " Edit review prompt and retry" << std::endl;
                    std::cout << colors.muted("[Q]") << " Quit review" << std::endl;

                    std::string choice = clion::cli::InteractionHandler::getUserInput("Your choice (A/S/E/Q)", "S");

                    if (choice.empty()) choice = "S";
                    choice = std::toupper(choice[0]);

                    if (choice == "A") {
                        // Apply changes
                        clion::cli::InteractionHandler::showInfo("Applying code review suggestions...");
                        if (clion::utils::FileUtils::writeFile(options.file_path, suggested_code)) {
                            clion::cli::InteractionHandler::showSuccess("✅ Code review changes applied successfully!");
                            original_content = suggested_code;  // Update for next iteration
                            valid_choice = true;
                        } else {
                            clion::cli::InteractionHandler::showError("❌ Failed to apply changes to file");
                            valid_choice = true;
                            review_complete = true;
                        }
                    } else if (choice == "S") {
                        // Skip changes
                        clion::cli::InteractionHandler::showInfo("Skipping code review suggestions");
                        valid_choice = true;
                        review_complete = true;
                    } else if (choice == "E") {
                        // Edit prompt and retry
                        clion::cli::InteractionHandler::showInfo("Enter additional review instructions:");
                        std::string additional_instructions = clion::cli::InteractionHandler::getUserInput("Additional instructions", "");
                        if (!additional_instructions.empty()) {
                            base_prompt += "\nAdditional instructions: " + additional_instructions + "\n";
                        }
                        valid_choice = true;
                        // Continue loop for another iteration
                    } else if (choice == "Q") {
                        // Quit
                        clion::cli::InteractionHandler::showInfo("Exiting code review");
                        valid_choice = true;
                        review_complete = true;
                    } else {
                        clion::cli::InteractionHandler::showWarning("Invalid choice. Please select A, S, E, or Q.");
                    }
                }
            }

            if (iteration >= MAX_ITERATIONS) {
                clion::cli::InteractionHandler::showWarning("Maximum review iterations reached");
            }

            clion::cli::InteractionHandler::showSuccess("Code review session completed");
            return 0;
        }
        else if (options.command == "fix") {
            // Initialize UI components
            auto& ui = clion::ui::UIManager::getInstance();
            ui.initialize();
            auto& colors = ui.getColorManager();

            ui.showHeader("🔧 CLion Error Fix Workflow");
            std::cout << colors.info("Build Command: ") << colors.primary(options.fix_command) << std::endl;

            if (!llm_client.isInitialized()) {
                clion::cli::InteractionHandler::showError("LLM client not initialized. Please set OPENROUTER_API_KEY environment variable.");
                return 1;
            }

            const int MAX_ITERATIONS = 5;
            int iteration = 0;
            bool build_successful = false;

            while (iteration < MAX_ITERATIONS && !build_successful) {
                iteration++;
                clion::cli::InteractionHandler::showInfo("Starting iteration " + std::to_string(iteration) + "/" + std::to_string(MAX_ITERATIONS));

                // Execute build command
                auto result = clion::compiler::EnhancedCommandExecutor::executeBuild(options.fix_command);

                if (result.success) {
                    clion::cli::InteractionHandler::showSuccess("Command executed successfully!");
                    build_successful = true;
                    break;
                }

                clion::cli::InteractionHandler::showError("Command failed with output:\n" + result.stdout_output);

                // Generate enhanced context for LLM
                std::string context = "Error Details:\n" + result.stdout_output + "\n";

                std::string system_instruction = buildSystemInstructions(g_clion_config);

                std::string prompt = "The following command failed. Please provide a fix.\n\n" + context +
                                   "Please provide a targeted fix. Only modify the necessary code.\n\n" +
                                   "Iteration: " + std::to_string(iteration) + "/" + std::to_string(MAX_ITERATIONS) + "\n\n";

                std::string enhanced_prompt = clion::llm::ContextBuilder::buildContext(prompt);

                clion::cli::InteractionHandler::showInfo("Requesting AI fix...");

                auto llm_response = llm_client.sendRequest(enhanced_prompt, system_instruction);

                if (!llm_response.success) {
                    clion::cli::InteractionHandler::showError("Failed to get AI response: " + llm_response.error_message);
                    continue;
                }

                // Extract and format the fix
                std::string fixed_code = clion::utils::StringUtils::extractCodeFromBlock(llm_response.content);

                if (fixed_code.empty()) {
                    clion::cli::InteractionHandler::showWarning("No code block found in AI response, using raw response");
                    fixed_code = llm_response.content;
                }

                clion::cli::InteractionHandler::showInfo("AI Suggested Fix:");
                std::cout << colors.primary(fixed_code) << std::endl;

                // Get user approval
                bool approved = clion::cli::InteractionHandler::getConfirmation("Apply this fix?");

                if (!approved) {
                    clion::cli::InteractionHandler::showWarning("Fix rejected by user. Stopping workflow.");
                    break;
                }

                // Find the file to apply the fix to
                // For now, we'll assume the first file mentioned in the error output is the correct one.
                std::string file_to_fix;
                std::regex file_regex("([a-zA-Z0-9_./-]+):\\d+:\\d+");
                std::smatch match;
                if (std::regex_search(result.stdout_output, match, file_regex)) {
                    file_to_fix = match[1];
                }

                if (file_to_fix.empty()) {
                    clion::cli::InteractionHandler::showError("Could not determine which file to fix. Please specify the file manually.");
                    break;
                }

                // Apply the fix
                clion::cli::InteractionHandler::showInfo("Applying fix to " + file_to_fix);
                if (clion::utils::FileUtils::writeFile(file_to_fix, fixed_code)) {
                    clion::cli::InteractionHandler::showSuccess("Fix applied successfully");
                } else {
                    clion::cli::InteractionHandler::showError("Failed to apply fix to file");
                    break;
                }

                clion::cli::InteractionHandler::showInfo("Retrying command...");
            }

            if (build_successful) {
                clion::cli::InteractionHandler::showSuccess("✅ Error fix workflow completed successfully!");
                return 0;
            } else {
                clion::cli::InteractionHandler::showError("❌ Error fix workflow failed after " + std::to_string(MAX_ITERATIONS) + " iterations");
                return 1;
            }
        }
        else if (options.command == "nlp") {
            std::cout << "NLP command selected: " << options.nlp_action << std::endl;
            
            if (options.nlp_action == "analyze") {
                if (!options.file_path.empty()) {
                    auto content = clion::utils::FileUtils::readFile(options.file_path);
                    if (content) {
                        auto analysis = clion::nlp::TextAnalyzer::analyzeCode(*content);
                        std::cout << "Analysis Results:" << std::endl;
                        std::cout << "- Sentiment Score: " << analysis.sentiment_score << std::endl;
                        std::cout << "- Complexity Score: " << analysis.complexity_score << std::endl;
                        std::cout << "- Documentation Quality: " << analysis.documentation_quality << std::endl;
                        std::cout << "- Summary: " << analysis.summary << std::endl;
                    } else {
                        std::cerr << "Error: Could not read file: " << options.file_path << std::endl;
                        return 1;
                    }
                } else if (!options.nlp_text.empty()) {
                    auto analysis = clion::nlp::TextAnalyzer::analyzeText(options.nlp_text);
                    std::cout << "Text Analysis Results:" << std::endl;
                    std::cout << "- Sentiment Score: " << analysis.sentiment_score << std::endl;
                    std::cout << "- Summary: " << analysis.summary << std::endl;
                }
            }
            else if (options.nlp_action == "interpret") {
                if (llm_client.isInitialized()) {
                    if (!options.nlp_error.empty()) {
                        std::string prompt = clion::llm::prompts::CXX_ERROR_PROMPT + "\n\n" + options.nlp_error;
                        auto llm_response = llm_client.sendRequest(prompt);
                        if (llm_response.success) {
                            std::cout << "Error Interpretation:" << std::endl;
                            std::cout << llm_response.content << std::endl;
                        } else {
                            std::cerr << "Error: " << llm_response.error_message << std::endl;
                        }
                    }
                } else {
                    if (!options.nlp_error.empty()) {
                        auto interpretation = clion::nlp::ErrorInterpreter::interpret(options.nlp_error);
                        std::cout << "Error Interpretation:" << std::endl;
                        std::cout << "- Explanation: " << interpretation.explanation << std::endl;
                        std::cout << "- Suggested Fix: " << interpretation.suggested_fix << std::endl;
                    }
                }
            }
            else if (options.nlp_action == "suggest") {
                if (llm_client.isInitialized()) {
                    if (!options.nlp_text.empty()) {
                        std::string prompt = clion::llm::prompts::CXX_SUGGEST_PROMPT + "\n\n" + options.nlp_text;
                        auto llm_response = llm_client.sendRequest(prompt);
                        if (llm_response.success) {
                            std::cout << "Suggested Command: " << llm_response.content << std::endl;
                        } else {
                            std::cerr << "Error: " << llm_response.error_message << std::endl;
                        }
                    } else if (options.nlp_interactive) {
                        std::cout << "Interactive NLP mode (type 'exit' to quit):" << std::endl;
                        std::string input;
                        while (std::getline(std::cin, input) && input != "exit") {
                            std::string prompt = clion::llm::prompts::CXX_SUGGEST_PROMPT + "\n\n" + input;
                            auto llm_response = llm_client.sendRequest(prompt);
                            if (llm_response.success) {
                                std::cout << "Suggested: " << llm_response.content << std::endl;
                            } else {
                                std::cerr << "Error: " << llm_response.error_message << std::endl;
                            }
                        }
                    }
                } else {
                    if (!options.nlp_text.empty()) {
                        auto intent = clion::nlp::CommandInterpreter::parseNaturalLanguage(options.nlp_text);
                        std::cout << "Suggested Command: " << intent.action << std::endl;
                        std::cout << "Confidence: " << intent.confidence << std::endl;
                    } else if (options.nlp_interactive) {
                        std::cout << "Interactive NLP mode (type 'exit' to quit):" << std::endl;
                        std::string input;
                        while (std::getline(std::cin, input) && input != "exit") {
                            auto intent = clion::nlp::CommandInterpreter::parseNaturalLanguage(input);
                            std::cout << "Suggested: " << intent.action << " (confidence: " << intent.confidence << ")" << std::endl;
                        }
                    }
                }
            }
            else if (options.nlp_action == "summarize") {
                if (!options.file_path.empty()) {
                    auto content = clion::utils::FileUtils::readFile(options.file_path);
                    if (content) {
                        auto summary = clion::nlp::TextAnalyzer::generateSummary(*content);
                        std::cout << "Summary: " << summary << std::endl;
                    }
                }
            }
            else if (options.nlp_action == "analyze-code") {
                if (!options.file_path.empty()) {
                    auto content = clion::utils::FileUtils::readFile(options.file_path);
                    if (content) {
                        auto analysis = clion::nlp::CodeAnalyzer::analyzeCode(*content);
                        std::cout << "Code Analysis Results:" << std::endl;
                        std::cout << "- Summary: " << analysis.summary << std::endl;
                        for (const auto& suggestion : analysis.suggestions) {
                            std::cout << "- Suggestion: " << suggestion << std::endl;
                        }
                    } else {
                        std::cerr << "Error: Could not read file: " << options.file_path << std::endl;
                        return 1;
                    }
                }
            }
            else if (options.nlp_action == "generate") {
                if (llm_client.isInitialized()) {
                    if (!options.nlp_generate.empty()) {
                        std::string prompt = clion::llm::prompts::CXX_GENERATE_PROMPT + "\n\n" + options.nlp_generate;
                        auto llm_response = llm_client.sendRequest(prompt);
                        if (llm_response.success) {
                            std::cout << "Generated Code:" << std::endl;
                            std::cout << llm_response.content << std::endl;
                        } else {
                            std::cerr << "Error: " << llm_response.error_message << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Error: LLMClient not initialized. Please set the OPENROUTER_API_KEY environment variable." << std::endl;
                }
            }
            
            return 0;
        }
        else {
            std::cerr << "Unknown command: " << options.command << std::endl;
            parser.printHelp();
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}