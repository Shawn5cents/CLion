#include "project_scanner.h"
#include "clion/common.h"

namespace clion {
namespace indexer {

std::vector<path> ProjectScanner::scanProject(const path& project_root, const ScanOptions& options) {
    // TODO: Implement in Phase 3.1
    return {};
}

std::unordered_set<std::string> ProjectScanner::parseGitignore(const path& gitignore_path) {
    // TODO: Implement in Phase 3.1
    return {};
}

} // namespace indexer
} // namespace clion
