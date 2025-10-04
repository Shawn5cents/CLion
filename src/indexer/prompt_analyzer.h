#pragma once

#include <string>
#include "clion/common.h"

namespace clion {
namespace indexer {

class PromptAnalyzer {
public:
    static bool shouldIncludeFullFile(const std::string& prompt, const std::string& file_path);
    static std::string generateSummary(const std::string& file_path);
};

} // namespace indexer
} // namespace clion
