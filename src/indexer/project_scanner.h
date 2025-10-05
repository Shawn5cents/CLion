#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include "clion/common.h"

namespace clion {
namespace indexer {

struct ScanOptions {
    std::vector<std::string> include_extensions = {".cpp", ".h", ".hpp", ".cc", ".cxx"};
    std::vector<std::string> exclude_patterns = {"build/*", "vendor/*"};
    bool respect_gitignore = true;
    bool scan_subdirectories = true;
};

class ProjectScanner {
public:
    static std::vector<path> scanProject(const path& project_root, const ScanOptions& options = ScanOptions());
    static std::vector<path> scanProjectWithContext(const path& project_root, const ScanOptions& options = ScanOptions());
    static std::unordered_set<std::string> parseGitignore(const path& gitignore_path);

private:
    static std::string convertGitignoreToGlob(const std::string& pattern);
    static void scanDirectoryRecursive(const path& root, const path& current_dir,
                                     std::vector<path>& files,
                                     const std::unordered_set<std::string>& gitignore_patterns,
                                     const ScanOptions& options);
};

} // namespace indexer
} // namespace clion
