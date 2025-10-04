#include <iostream>
#include <memory>
#include "cli/cli_parser.h"

int main(int argc, char** argv) {
    try {
        // Create CLI parser
        clion::cli::CLIParser parser;
        
        // Parse command line arguments
        if (!parser.parse(argc, argv)) {
            parser.printHelp();
            return 1;
        }
        
        const auto& options = parser.getOptions();
        
        // Handle different commands
        if (options.command == "review") {
            std::cout << "Review command selected for file: " << options.file_path << std::endl;
            if (options.explain_mode) {
                std::cout << "Explain mode enabled" << std::endl;
            }
            // TODO: Implement review functionality
            return 0;
        }
        else if (options.command == "fix-error") {
            std::cout << "Fix-error command selected with command: " << options.fix_command << std::endl;
            if (options.explain_mode) {
                std::cout << "Explain mode enabled" << std::endl;
            }
            // TODO: Implement fix-error functionality
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