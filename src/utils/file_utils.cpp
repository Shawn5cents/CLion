#include "file_utils.h"
#include "clion/common.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace clion {
namespace utils {

std::optional<std::string> FileUtils::readFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return std::nullopt;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool FileUtils::writeFile(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileUtils::fileExists(const std::string& path) {
    try {
        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    } catch (const std::exception&) {
        return false;
    }
}

size_t FileUtils::getFileSize(const std::string& path) {
    try {
        if (!fileExists(path)) {
            return 0;
        }
        return std::filesystem::file_size(path);
    } catch (const std::exception&) {
        return 0;
    }
}

std::string FileUtils::getFileExtension(const std::string& path) {
    try {
        std::filesystem::path fs_path(path);
        return fs_path.extension().string();
    } catch (const std::exception&) {
        return "";
    }
}

std::vector<std::string> FileUtils::listFiles(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;
    
    try {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            return files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                
                // Filter by extension if specified
                if (!extension.empty()) {
                    std::string file_ext = getFileExtension(file_path);
                    // Convert to lowercase for case-insensitive comparison
                    std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
                    std::string ext_lower = extension;
                    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);
                    
                    if (file_ext != ext_lower) {
                        continue;
                    }
                }
                
                files.push_back(file_path);
            }
        }
    } catch (const std::exception&) {
        // Return empty list on error
    }
    
    return files;
}

} // namespace utils
} // namespace clion
