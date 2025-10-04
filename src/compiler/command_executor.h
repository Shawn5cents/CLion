#pragma once

#include <string>
#include "clion/common.h"

namespace clion {
namespace compiler {

struct CommandResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
    bool success;
};

class CommandExecutor {
public:
    static CommandResult execute(const std::string& command, const std::string& working_directory = ".");
    static bool commandExists(const std::string& command);
};

} // namespace compiler
} // namespace clion
