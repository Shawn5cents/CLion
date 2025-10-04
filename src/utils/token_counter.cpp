#include "token_counter.h"
#include "clion/common.h"

namespace clion {
namespace utils {

int TokenCounter::countTokens(const std::string& text) {
    // TODO: Implement in Phase 2.4
    return text.length() / 4; // Rough estimate
}

double TokenCounter::estimateCost(int tokens, const std::string& model) {
    // TODO: Implement in Phase 2.4
    return tokens * 0.00001; // Placeholder cost
}

} // namespace utils
} // namespace clion
