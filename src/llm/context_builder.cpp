#include "context_builder.h"
#include "clion/common.h"

namespace clion {
namespace llm {

std::string ContextBuilder::buildContext(const std::string& base_prompt, const std::string& project_root) {
    // TODO: Implement in Phase 2.2
    return base_prompt;
}

std::vector<std::string> ContextBuilder::extractFileInclusions(const std::string& prompt) {
    // TODO: Implement in Phase 2.2
    return {};
}

std::string ContextBuilder::injectFileContents(const std::string& prompt, const std::string& project_root) {
    // TODO: Implement in Phase 2.2
    return prompt;
}

} // namespace llm
} // namespace clion
