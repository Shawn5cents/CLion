#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "clion/common.h"

namespace clion {
namespace indexer {

struct FunctionInfo {
    std::string name;
    std::string return_type;
    std::vector<std::string> parameters;
    int line_number;
};

struct ClassInfo {
    std::string name;
    std::vector<std::string> base_classes;
    int line_number;
};

struct FileInfo {
    path file_path;
    std::vector<std::string> includes;
    std::vector<FunctionInfo> functions;
    std::vector<ClassInfo> classes;
};

using CodeIndex = std::unordered_map<std::string, FileInfo>;

class CodeIndexer {
public:
    static CodeIndex buildIndex(const std::vector<path>& files);
    static FileInfo indexFile(const path& file_path);
};

} // namespace indexer
} // namespace clion
