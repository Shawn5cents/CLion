#pragma once

#include <string>
#include <vector>
#include "clion/common.h"

namespace clion {
namespace compiler {

struct CompilerError {
    std::string file_path;
    int line_number;
    int column;
    std::string severity;
    std::string message;
};

class ErrorParser {
public:
    static std::vector<CompilerError> parseErrors(const std::string& compiler_output);
    static std::vector<CompilerError> filterBySeverity(const std::vector<CompilerError>& errors, const std::string& severity);

private:
    static void parseGCCClangErrors(const std::string& output, std::vector<CompilerError>& errors);
    static void parseMSVCErrors(const std::string& output, std::vector<CompilerError>& errors);
    static void parseLinkerErrors(const std::string& output, std::vector<CompilerError>& errors);
};

} // namespace compiler
} // namespace clion
