#include "interaction.h"
#include "clion/common.h"
#include <iostream>

namespace clion {
namespace cli {

// Placeholder implementation - will be expanded in Phase 5.1
void InteractionHandler::showWelcome() {
    std::cout << CLION_NAME << " v" << CLION_VERSION << " - " << CLION_DESCRIPTION << std::endl;
}

void InteractionHandler::showError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
}

void InteractionHandler::showInfo(const std::string& message) {
    std::cout << "Info: " << message << std::endl;
}

void InteractionHandler::showVerbose(const std::string& message) {
    std::cout << "[VERBOSE] " << message << std::endl;
}

} // namespace cli
} // namespace clion