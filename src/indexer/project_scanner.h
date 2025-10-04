#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "clion/common.h"

namespace clion {
namespace indexer {

struct ScanOptions {
    std::vector<std::string> include_extensions = {".cpp", ".h", ".hpp", ".cc", ".cxx"};
    std::vector<std::string> exclude_patterns = {"build/*", "vendor/*"};
    bool respect_gitignore = true;
};

class ProjectScanner {
public:
    static std::vector<path> scanProject(const path& project_root, const ScanOptions& options = ScanOptions());
    static std::unordered_set<std::string> parseGitignore(const path& gitignore_path);
};

} // namespace indexer
} // namespace clion
