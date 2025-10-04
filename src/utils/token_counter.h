#pragma once

#include <string>
#include "clion/common.h"

namespace clion {
namespace utils {

class TokenCounter {
public:
    static int countTokens(const std::string& text);
    static double estimateCost(int tokens, const std::string& model = "gemini-pro");
};

} // namespace utils
} // namespace clion
