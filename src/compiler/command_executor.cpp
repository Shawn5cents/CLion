#include "command_executor.h"
#include "clion/common.h"

namespace clion {
namespace compiler {

CommandResult CommandExecutor::execute(const std::string& command, const std::string& working_directory) {
    CommandResult result;
    
    try {
        // Save current directory
        std::filesystem::path original_dir = std::filesystem::current_path();
        
        // Change to working directory if specified
        if (!working_directory.empty()) {
            std::filesystem::current_path(working_directory);
        }
        
        // Execute command and capture both stdout and stderr
        std::string full_command = command + " 2>&1"; // Redirect stderr to stdout
        FILE* pipe = popen(full_command.c_str(), "r");
        if (!pipe) {
            result.success = false;
            result.stderr_output = "Failed to execute command: " + command;
            return result;
        }
        
        // Read output (combined stdout and stderr)
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result.stdout_output += buffer;
        }
        
        // Get exit code
        int exit_code = pclose(pipe);
        result.success = (exit_code == 0);
        result.exit_code = exit_code;
        
        // Restore original directory
        std::filesystem::current_path(original_dir);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.stderr_output = "Exception during command execution: " + std::string(e.what());
    }
    
    return result;
}

bool CommandExecutor::commandExists(const std::string& command) {
    // Simple check - try to get command help
    std::string test_command = command + " --help > /dev/null 2>&1";
    int result = system(test_command.c_str());
    return (result == 0 || result == 256); // 256 is common for help commands
}

} // namespace compiler
} // namespace clion
