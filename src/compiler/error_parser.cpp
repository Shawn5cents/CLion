#include "error_parser.h"
#include "clion/common.h"
#include <regex>

namespace clion {
namespace compiler {

std::vector<CompilerError> ErrorParser::parseErrors(const std::string& compiler_output) {
    std::vector<CompilerError> errors;

    // Try GCC/Clang format first
    parseGCCClangErrors(compiler_output, errors);

    // Try MSVC format if no GCC/Clang errors found
    if (errors.empty()) {
        parseMSVCErrors(compiler_output, errors);
    }

    // Try linker errors
    parseLinkerErrors(compiler_output, errors);

    return errors;
}

void ErrorParser::parseGCCClangErrors(const std::string& output, std::vector<CompilerError>& errors) {
    // GCC/Clang format: file:line:col: severity: message
    std::regex gcc_regex(R"((.+?):(\d+):(\d+):\s*(error|warning|note):\s*(.+?)(?=\n[^ ]|\n*$))");
    std::smatch match;
    std::string::const_iterator search_start(output.cbegin());

    while (std::regex_search(search_start, output.cend(), match, gcc_regex)) {
        CompilerError error;
        error.file_path = match[1];
        error.line_number = std::stoi(match[2]);
        error.column = std::stoi(match[3]);
        error.severity = match[4];
        error.message = match[5];
        errors.push_back(error);
        search_start = match.suffix().first;
    }
}

void ErrorParser::parseMSVCErrors(const std::string& output, std::vector<CompilerError>& errors) {
    // MSVC format: file(line): severity C####: message
    // or: file(line,col): severity C####: message
    std::regex msvc_regex(R"((.+?)\((\d+)(?:,(\d+))?\):\s*(error|warning|info)\s*(?:C\d+)?\s*:\s*(.+?)(?=\n[^ ]|\n*$))");
    std::smatch match;
    std::string::const_iterator search_start(output.cbegin());

    while (std::regex_search(search_start, output.cend(), match, msvc_regex)) {
        CompilerError error;
        error.file_path = match[1];
        error.line_number = std::stoi(match[2]);
        error.column = match[3].matched ? std::stoi(match[3]) : 0;
        error.severity = match[4];
        error.message = match[5];
        errors.push_back(error);
        search_start = match.suffix().first;
    }
}

void ErrorParser::parseLinkerErrors(const std::string& output, std::vector<CompilerError>& errors) {
    // Linker format: undefined reference to `symbol' in file
    std::regex linker_regex(R"((undefined reference to .+?)(?:\s+in\s+(.+?))?(?=\n[^ ]|\n*$))");
    std::smatch match;
    std::string::const_iterator search_start(output.cbegin());

    while (std::regex_search(search_start, output.cend(), match, linker_regex)) {
        CompilerError error;
        error.file_path = match[2].matched ? match[2].str() : "unknown";
        error.line_number = 0;
        error.column = 0;
        error.severity = "error";
        error.message = match[1];
        errors.push_back(error);
        search_start = match.suffix().first;
    }
}

std::vector<CompilerError> ErrorParser::filterBySeverity(const std::vector<CompilerError>& errors, const std::string& severity) {
    std::vector<CompilerError> filtered;
    std::copy_if(errors.begin(), errors.end(), std::back_inserter(filtered),
                [severity](const CompilerError& error) {
                    return error.severity == severity;
                });
    return filtered;
}

} // namespace compiler
} // namespace clion
