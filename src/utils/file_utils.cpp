#include "file_utils.h"
#include "clion/common.h"
#include <fstream>

namespace clion {
namespace utils {

std::optional<std::string> FileUtils::readFile(const std::string& path) {
    // TODO: Implement in Phase 1.4
    return std::nullopt;
}

bool FileUtils::writeFile(const std::string& path, const std::string& content) {
    // TODO: Implement in Phase 1.4
    return false;
}

bool FileUtils::fileExists(const std::string& path) {
    // TODO: Implement in Phase 1.4
    return false;
}

size_t FileUtils::getFileSize(const std::string& path) {
    // TODO: Implement in Phase 1.4
    return 0;
}

std::string FileUtils::getFileExtension(const std::string& path) {
    // TODO: Implement in Phase 1.4
    return "";
}

std::vector<std::string> FileUtils::listFiles(const std::string& directory, const std::string& extension) {
    // TODO: Implement in Phase 1.4
    return {};
}

} // namespace utils
} // namespace clion
