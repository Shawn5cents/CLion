#pragma once

#include <string>
#include "clion/common.h"

namespace clion {
namespace cli {

class InteractionHandler {
public:
    // Display methods
    static void showWelcome();
    static void showError(const std::string& message);
    static void showInfo(const std::string& message);
    static void showVerbose(const std::string& message);
    
    // TODO: Add more interaction methods in Phase 5.1
    // static bool getConfirmation(const std::string& message);
    // static std::string getUserInput(const std::string& prompt);
    // static void displayDiff(const std::string& diff);
};

} // namespace cli
} // namespace clion