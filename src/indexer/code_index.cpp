#include "code_index.h"
#include "clion/common.h"
#include "../utils/file_utils.h"
#include <regex>

namespace clion {
namespace indexer {

CodeIndex CodeIndexer::buildIndex(const std::vector<path>& files) {
    CodeIndex index;
    for (const auto& file : files) {
        index[file.string()] = indexFile(file);
    }
    return index;
}

FileInfo CodeIndexer::indexFile(const path& file_path) {
    FileInfo file_info;
    file_info.file_path = file_path;

    auto content = clion::utils::FileUtils::readFile(file_path.string());
    if (!content) {
        return file_info;
    }

    // Parse include statements
    std::regex include_regex("#include\\s*[\"<](.+?)[\">]");
    std::smatch match;
    std::string::const_iterator search_start(content->cbegin());

    while (std::regex_search(search_start, content->cend(), match, include_regex)) {
        file_info.includes.push_back(match[1]);
        search_start = match.suffix().first;
    }

    // Parse function definitions
    std::regex function_regex("([\\w::]+)\\s+([\\w::]+)\\s*\\((.*?)\\)\\s*\\{");
    std::smatch function_match;
    std::string::const_iterator function_search_start(content->cbegin());

    while (std::regex_search(function_search_start, content->cend(), function_match, function_regex)) {
        FunctionInfo function_info;
        function_info.return_type = function_match[1];
        function_info.name = function_match[2];
        
        // TODO: Parse parameters

        file_info.functions.push_back(function_info);
        function_search_start = function_match.suffix().first;
    }

    // Parse class definitions
    std::regex class_regex("class\\s+([\\w::]+)");
    std::smatch class_match;
    std::string::const_iterator class_search_start(content->cbegin());

    while (std::regex_search(class_search_start, content->cend(), class_match, class_regex)) {
        ClassInfo class_info;
        class_info.name = class_match[1];

        // TODO: Parse base classes

        file_info.classes.push_back(class_info);
        class_search_start = class_match.suffix().first;
    }

    return file_info;
}

} // namespace indexer
} // namespace clion
