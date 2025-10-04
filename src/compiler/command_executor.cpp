#include "command_executor.h"
#include "clion/common.h"

namespace clion {
namespace compiler {

CommandResult CommandExecutor::execute(const std::string& command, const std::string& working_directory) {
    // TODO: Implement in Phase 4.1
    CommandResult result;
    result.success = false;
    return result;
}

bool CommandExecutor::commandExists(const std::string& command) {
    // TODO: Implement in Phase 4.1
    return false;
}

} // namespace compiler
} // namespace clion
