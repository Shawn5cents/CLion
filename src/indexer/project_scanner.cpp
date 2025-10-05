#include "project_scanner.h"
#include "clion/common.h"
#include "../utils/string_utils.h"
#include <fstream>
#include <iostream>

namespace clion {
namespace indexer {

std::vector<path> ProjectScanner::scanProject(const path& project_root, const ScanOptions& options) {
    std::vector<path> files;
    std::unordered_set<std::string> gitignore_patterns;
    if (options.respect_gitignore) {
        gitignore_patterns = parseGitignore(project_root / ".gitignore");
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(project_root)) {
        if (entry.is_regular_file()) {
            std::string file_path = entry.path().string();
            bool excluded = false;

            // Check exclude patterns
            for (const auto& pattern : options.exclude_patterns) {
                if (clion::utils::StringUtils::matchesGlob(file_path, pattern)) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) continue;

            // Check gitignore patterns
            if (options.respect_gitignore) {
                for (const auto& pattern : gitignore_patterns) {
                    if (clion::utils::StringUtils::matchesGlob(file_path, pattern)) {
                        excluded = true;
                        break;
                    }
                }
            }
            if (excluded) continue;

            // Check include extensions
            bool included = false;
            for (const auto& ext : options.include_extensions) {
                if (file_path.length() >= ext.length() && file_path.substr(file_path.length() - ext.length()) == ext) {
                    included = true;
                    break;
                }
            }

            if (included) {
                files.push_back(entry.path());
            }
        }
    }

    return files;
}

std::unordered_set<std::string> ProjectScanner::parseGitignore(const path& gitignore_path) {
    std::unordered_set<std::string> patterns;
    if (!std::filesystem::exists(gitignore_path)) {
        return patterns;
    }

    std::ifstream file(gitignore_path);
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());

        if (!line.empty() && line[0] != '#') {
            // Convert gitignore pattern to regex-like pattern for glob matching
            std::string pattern = convertGitignoreToGlob(line);
            if (!pattern.empty()) {
                patterns.insert(pattern);
            }
        }
    }

    return patterns;
}

std::string ProjectScanner::convertGitignoreToGlob(const std::string& pattern) {
    std::string glob_pattern = pattern;

    // Handle directory patterns
    if (!glob_pattern.empty() && glob_pattern.back() == '/') {
        glob_pattern += "**/*";
    }

    // Handle patterns that should match directories
    if (glob_pattern.find('/') == std::string::npos && glob_pattern.find('*') == std::string::npos) {
        // This might be a directory name, also match files in it
        // Keep as is for now, let the glob matcher handle it
    }

    return glob_pattern;
}

std::vector<path> ProjectScanner::scanProjectWithContext(const path& project_root, const ScanOptions& options) {
    std::vector<path> files;
    std::unordered_set<std::string> gitignore_patterns;

    if (options.respect_gitignore) {
        gitignore_patterns = parseGitignore(project_root / ".gitignore");
    }

    // Also check for .gitignore in parent directories
    path current_path = project_root;
    while (current_path.has_parent_path()) {
        current_path = current_path.parent_path();
        if (std::filesystem::exists(current_path / ".gitignore")) {
            auto parent_patterns = parseGitignore(current_path / ".gitignore");
            gitignore_patterns.insert(parent_patterns.begin(), parent_patterns.end());
        }
        if (current_path.parent_path() == current_path) break;
    }

    scanDirectoryRecursive(project_root, project_root, files, gitignore_patterns, options);

    return files;
}

void ProjectScanner::scanDirectoryRecursive(const path& root, const path& current_dir,
                                           std::vector<path>& files,
                                           const std::unordered_set<std::string>& gitignore_patterns,
                                           const ScanOptions& options) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(current_dir)) {
            std::string relative_path = std::filesystem::relative(entry.path(), root).string();
            bool excluded = false;

            // Check exclude patterns
            for (const auto& pattern : options.exclude_patterns) {
                if (clion::utils::StringUtils::matchesGlob(relative_path, pattern)) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) continue;

            // Check gitignore patterns
            if (options.respect_gitignore) {
                for (const auto& pattern : gitignore_patterns) {
                    if (clion::utils::StringUtils::matchesGlob(relative_path, pattern)) {
                        excluded = true;
                        break;
                    }
                }
            }
            if (excluded) continue;

            if (entry.is_regular_file()) {
                // Check include extensions
                bool included = options.include_extensions.empty();
                for (const auto& ext : options.include_extensions) {
                    if (relative_path.length() >= ext.length() &&
                        relative_path.substr(relative_path.length() - ext.length()) == ext) {
                        included = true;
                        break;
                    }
                }

                if (included) {
                    files.push_back(entry.path());
                }
            } else if (entry.is_directory() && options.scan_subdirectories) {
                // Check if this directory should be scanned
                std::string dir_pattern = relative_path + "/";
                bool dir_excluded = false;

                for (const auto& pattern : gitignore_patterns) {
                    if (clion::utils::StringUtils::matchesGlob(dir_pattern, pattern)) {
                        dir_excluded = true;
                        break;
                    }
                }

                if (!dir_excluded) {
                    scanDirectoryRecursive(root, entry.path(), files, gitignore_patterns, options);
                }
            }
        }
    } catch (const std::exception& e) {
        // Log error but continue scanning other directories
        std::cerr << "Warning: Error scanning directory " << current_dir.string() << ": " << e.what() << std::endl;
    }
}

} // namespace indexer
} // namespace clion
