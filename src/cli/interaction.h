#pragma once

#include <string>
#include <vector>
#include "clion/common.h"

namespace clion {
namespace cli {

class InteractionHandler {
public:
    // Enhanced display methods
    static void showWelcome();
    static void showError(const std::string& message, const std::string& context = "");
    static void showInfo(const std::string& message);
    static void showVerbose(const std::string& message);
    static void showSuccess(const std::string& message);
    static void showWarning(const std::string& message);
    
    // Progress display
    static void showProgress(const std::string& operation, size_t current, size_t total);
    static void showSpinner(const std::string& message, bool start = true);
    static void hideSpinner();
    
    // Table display
    static void showFileTable(const std::vector<std::string>& files);
    static void showErrorTable(const std::vector<std::string>& errors);
    
    // Interactive methods
    static bool getConfirmation(const std::string& message);
    static std::string getUserInput(const std::string& prompt, const std::string& default_value = "");
    static void displayDiff(const std::string& diff);
    
    // UI configuration
    static void setVerbose(bool verbose);
    static void setQuiet(bool quiet);
    static void setEnabled(bool enabled);

    // Interactive command system
    static bool processInteractiveCommand(const std::string& input);
    static void showCommandHistory();
    static std::string promptWithCommandSupport(const std::string& prompt, const std::string& default_value = "");

private:
    static bool spinner_active_;
};

} // namespace cli
} // namespace clion