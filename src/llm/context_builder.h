#pragma once

#include <string>
#include <vector>
#include "clion/common.h"

namespace clion {
namespace llm {

class ContextBuilder {
public:
    static std::string buildContext(const std::string& base_prompt,
                                   const std::string& project_root = ".");
    static std::vector<std::string> extractFileInclusions(const std::string& prompt);
    static std::string injectFileContents(const std::string& prompt,
                                        const std::string& project_root = ".");
};

} // namespace llm
} // namespace clion