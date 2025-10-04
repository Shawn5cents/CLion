#pragma once

#include <string>
#include <vector>
#include <optional>
#include "clion/common.h"

namespace clion {
namespace utils {

class FileUtils {
public:
    static std::optional<std::string> readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
    static bool fileExists(const std::string& path);
    static size_t getFileSize(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static std::vector<std::string> listFiles(const std::string& directory, const std::string& extension = "");
};

} // namespace utils
} // namespace clion
