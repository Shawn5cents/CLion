#include "interaction.h"
#include "clion/command_processor.h"
#include "../ui/ui_manager.h"
#include "../ui/color_manager.h"
#include "../ui/progress_manager.h"
#include "../ui/prompt_manager.h"
#include "../ui/table_formatter.h"
#include "../ui/terminal_io.h"
#include "../ui/spinner.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace clion {
namespace cli {

bool InteractionHandler::spinner_active_ = false;

void InteractionHandler::showWelcome() {
    auto& ui_manager = ui::UIManager::getInstance();
    ui_manager.showWelcome();
}

void InteractionHandler::showError(const std::string& message, const std::string& context) {
    auto& colors = ui::ColorManager::getInstance();
    auto& terminal = ui::TerminalIO::getInstance();
    
    terminal.printError("❌ Error: " + message);
    if (!context.empty()) {
        terminal.println("│ " + colors.muted(context));
    }
}

void InteractionHandler::showSuccess(const std::string& message) {
    auto& terminal = ui::TerminalIO::getInstance();
    terminal.printSuccess("✅ " + message);
}

void InteractionHandler::showWarning(const std::string& message) {
    auto& terminal = ui::TerminalIO::getInstance();
    terminal.printWarning("⚠️  " + message);
}

void InteractionHandler::showInfo(const std::string& message) {
    auto& terminal = ui::TerminalIO::getInstance();
    terminal.printInfo("ℹ️  " + message);
}

void InteractionHandler::showVerbose(const std::string& message) {
    auto& ui_manager = ui::UIManager::getInstance();
    if (ui_manager.isVerbose()) {
        auto& colors = ui::ColorManager::getInstance();
        auto& terminal = ui::TerminalIO::getInstance();
        terminal.println(colors.muted("[VERBOSE] ") + message);
    }
}

void InteractionHandler::showProgress(const std::string& operation, size_t current, size_t total) {
    auto& progress_manager = ui::ProgressManager::getInstance();
    progress_manager.showFileScanningProgress(operation, current, total);
}

void InteractionHandler::showSpinner(const std::string& message, bool start) {
    auto& colors = ui::ColorManager::getInstance();
    auto& terminal = ui::TerminalIO::getInstance();
    
    if (start) {
        spinner_active_ = true;
        terminal.print(colors.info(message) + " ⠋");
    } else {
        if (spinner_active_) {
            terminal.clearLine();
            terminal.printSuccess("✅ " + message);
            spinner_active_ = false;
        }
    }
}

void InteractionHandler::hideSpinner() {
    if (spinner_active_) {
        auto& terminal = ui::TerminalIO::getInstance();
        terminal.clearLine();
        spinner_active_ = false;
    }
}

void InteractionHandler::showFileTable(const std::vector<std::string>& files) {
    auto& table_formatter = ui::TableFormatter::getInstance();
    table_formatter.printFileList(files);
}

void InteractionHandler::showErrorTable(const std::vector<std::string>& errors) {
    auto& table_formatter = ui::TableFormatter::getInstance();
    table_formatter.printErrorList(errors);
}

bool InteractionHandler::getConfirmation(const std::string& message) {
    auto& prompt_manager = ui::PromptManager::getInstance();
    return prompt_manager.promptConfirmation(message);
}

std::string InteractionHandler::getUserInput(const std::string& prompt, const std::string& default_value) {
    auto& prompt_manager = ui::PromptManager::getInstance();
    return prompt_manager.promptText(prompt, default_value);
}

void InteractionHandler::displayDiff(const std::string& diff) {
    auto& colors = ui::ColorManager::getInstance();
    auto& terminal = ui::TerminalIO::getInstance();
    
    terminal.println(colors.bold("\n=== Code Changes ==="));
    
    std::istringstream diff_stream(diff);
    std::string line;
    while (std::getline(diff_stream, line)) {
        if (line.empty()) continue;
        
        if (line[0] == '+') {
            terminal.println(colors.success(line));
        } else if (line[0] == '-') {
            terminal.println(colors.error(line));
        } else if (line[0] == '@') {
            terminal.println(colors.info(line));
        } else {
            terminal.println(line);
        }
    }
    
    terminal.println(colors.muted("====================="));
}

void InteractionHandler::setVerbose(bool verbose) {
    auto& ui_manager = ui::UIManager::getInstance();
    ui_manager.setVerbose(verbose);
}

void InteractionHandler::setQuiet(bool quiet) {
    auto& ui_manager = ui::UIManager::getInstance();
    ui_manager.setQuiet(quiet);
}

void InteractionHandler::setEnabled(bool enabled) {
    auto& ui_manager = ui::UIManager::getInstance();
    ui_manager.setEnabled(enabled);
}

bool InteractionHandler::processInteractiveCommand(const std::string& input) {
    auto& processor = cli::CommandProcessor::getInstance();
    auto result = processor.processCommand(input);

    if (result.success) {
        if (!result.output.empty()) {
            std::cout << result.output << std::endl;
        }
        return true;
    } else {
        if (!result.error_message.empty()) {
            showError(result.error_message);
        }
        return false;
    }
}

void InteractionHandler::showCommandHistory() {
    auto& processor = cli::CommandProcessor::getInstance();

    if (processor.isHistoryEmpty()) {
        showInfo("No command history available");
        return;
    }

    showInfo("Command History (last " + std::to_string(processor.getHistorySize()) + " commands):");
    auto history = processor.getHistory(std::min(size_t(20), processor.getHistorySize()));

    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << history[i] << std::endl;
    }

    if (processor.getHistorySize() > 20) {
        showInfo("... and " + std::to_string(processor.getHistorySize() - 20) + " more commands");
    }
}

std::string InteractionHandler::promptWithCommandSupport(const std::string& prompt, const std::string& default_value) {
    auto& prompt_manager = ui::PromptManager::getInstance();
    return prompt_manager.promptWithSuggestions(prompt, default_value);
}

} // namespace cli
} // namespace clion