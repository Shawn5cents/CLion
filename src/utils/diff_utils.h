#pragma once

#include <string>
#include <vector>
#include "clion/common.h"

namespace clion {
namespace utils {

struct DiffLine {
    char type;
    std::string content;
    int old_line = -1;
    int new_line = -1;
};

struct DiffHunk {
    std::vector<DiffLine> lines;
    int old_start = -1;
    int old_count = 0;
    int new_start = -1;
    int new_count = 0;
};

class DiffUtils {
public:
    static std::string generateUnifiedDiff(const std::string& original, const std::string& modified, const std::string& original_file, const std::string& modified_file);
    static std::vector<DiffHunk> parseDiff(const std::string& diff);
    static std::string applyDiff(const std::string& original, const std::vector<DiffHunk>& hunks);
    static void displayDiff(const std::string& diff);

private:
    // Helper to split text into lines
    static std::vector<std::string> splitIntoLines(const std::string& text);
};

} // namespace utils
} // namespace clion
