#include "context_builder.h"
#include "clion/common.h"
#include "clion/memory_manager.h"
#include "../utils/file_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>

namespace clion {
namespace llm {

// Regex pattern for matching @file <path> syntax
const std::regex ContextBuilder::INCLUSION_PATTERN(R"(@file\s+([^\s\n]+))");

std::string ContextBuilder::buildContext(const std::string& base_prompt,
                                       const std::string& project_root,
                                       const ContextOptions& options) {
    try {
        if (options.enable_intelligent_selection) {
            return processInclusionsWithIntelligence(base_prompt, project_root, options);
        } else {
            return processInclusions(base_prompt, project_root, options);
        }
    } catch (const std::exception& e) {
        throw CLionException("Failed to build context: " + std::string(e.what()));
    }
}

std::vector<FileInclusion> ContextBuilder::extractFileInclusions(const std::string& prompt) {
    std::vector<FileInclusion> inclusions;
    std::sregex_iterator iter(prompt.begin(), prompt.end(), INCLUSION_PATTERN);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        FileInclusion inclusion;
        inclusion.file_path = match[1].str();
        inclusion.start_position = match.position();
        inclusion.end_position = match.position() + match.length();
        inclusion.full_match = match.str();
        inclusions.push_back(inclusion);
    }
    
    return inclusions;
}

std::string ContextBuilder::injectFileContents(const std::string& prompt,
                                             const std::string& project_root,
                                             const ContextOptions& options) {
    return processInclusions(prompt, project_root, options);
}

std::string ContextBuilder::processInclusions(const std::string& prompt,
                                            const std::string& project_root,
                                            const ContextOptions& options) {
    std::string result = prompt;
    auto inclusions = extractFileInclusions(prompt);
    
    // Process inclusions in reverse order to maintain position indices
    std::sort(inclusions.begin(), inclusions.end(),
              [](const FileInclusion& a, const FileInclusion& b) {
                  return a.start_position > b.start_position;
              });
    
    for (const auto& inclusion : inclusions) {
        try {
            std::string resolved_path = resolvePath(inclusion.file_path, project_root);
            
            // Security check: ensure path is within project root
            if (!isPathAllowed(resolved_path, project_root)) {
                std::string error_msg = "// Error: File '" + inclusion.file_path +
                                       "' is outside project directory or access denied";
                result.replace(inclusion.start_position, inclusion.full_match.length(), error_msg);
                continue;
            }
            
            // Check if file should be excluded
            if (shouldExcludeFile(resolved_path, options)) {
                std::string warning_msg = "// Warning: File '" + inclusion.file_path +
                                         "' matches exclude pattern";
                result.replace(inclusion.start_position, inclusion.full_match.length(), warning_msg);
                continue;
            }
            
            // Read and format file content
            std::string file_content = readFileWithFormatting(resolved_path, options);
            
            // Check if we need to truncate
            if (options.truncate_large_files &&
                estimateTokenCount(file_content) > options.max_context_size) {
                file_content = truncateFile(file_content, options.max_context_size, resolved_path);
            }
            
            // Replace @file reference with actual content
            result.replace(inclusion.start_position, inclusion.full_match.length(), file_content);
            
        } catch (const std::exception& e) {
            std::string error_msg = "// Error reading file '" + inclusion.file_path +
                                   "': " + std::string(e.what());
            result.replace(inclusion.start_position, inclusion.full_match.length(), error_msg);
        }
    }
    
    return result;
}

std::string ContextBuilder::resolvePath(const std::string& path, const std::string& project_root) {
    if (isAbsolutePath(path)) {
        return normalizePath(path);
    }
    
    std::filesystem::path resolved = std::filesystem::path(project_root) / path;
    return normalizePath(resolved.string());
}

std::string ContextBuilder::readFileWithFormatting(const std::string& path,
                                                 const ContextOptions& options) {
    auto content_opt = utils::FileUtils::readFile(path);
    if (!content_opt) {
        throw FileException("Cannot read file: " + path);
    }
    
    std::string content = *content_opt;
    std::string result;
    
    // Add file header
    std::string header = options.file_header_format;
    size_t pos = header.find("{path}");
    if (pos != std::string::npos) {
        header.replace(pos, 6, path);
    }
    result += header;
    
    // Add line numbers if requested
    if (options.include_line_numbers) {
        std::istringstream iss(content);
        std::string line;
        int line_num = 1;
        
        while (std::getline(iss, line)) {
            result += std::to_string(line_num) + " | " + line + "\n";
            line_num++;
        }
    } else {
        result += content;
        if (!content.empty() && content.back() != '\n') {
            result += "\n";
        }
    }
    
    return result;
}

std::string ContextBuilder::truncateFile(const std::string& content,
                                       size_t max_size,
                                       const std::string& file_path) {
    std::istringstream iss(content);
    std::vector<std::string> lines;
    std::string line;
    
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    size_t total_lines = lines.size();
    size_t keep_lines = max_size / 50; // Rough estimate: 50 chars per line
    
    if (keep_lines >= total_lines) {
        return content; // No truncation needed
    }
    
    std::string result;
    result += "// File truncated: showing " + std::to_string(keep_lines) +
              " of " + std::to_string(total_lines) + " lines\n";
    result += "// File: " + file_path + "\n\n";
    
    // Keep beginning and end, with truncation notice in middle
    size_t start_lines = keep_lines / 2;
    size_t end_lines = keep_lines - start_lines;
    
    for (size_t i = 0; i < start_lines && i < total_lines; ++i) {
        result += std::to_string(i + 1) + " | " + lines[i] + "\n";
    }
    
    result += "\n// ... " + std::to_string(total_lines - keep_lines) +
              " lines omitted ...\n\n";
    
    for (size_t i = total_lines - end_lines; i < total_lines; ++i) {
        result += std::to_string(i + 1) + " | " + lines[i] + "\n";
    }
    
    return result;
}

size_t ContextBuilder::estimateTokenCount(const std::string& text) {
    // Simple token estimation: roughly 4 characters per token
    // This is a rough approximation suitable for most cases
    return (text.length() + 3) / 4;
}

bool ContextBuilder::shouldExcludeFile(const std::string& path, const ContextOptions& options) {
    std::filesystem::path fs_path(path);
    std::string filename = fs_path.filename().string();
    std::string extension = fs_path.extension().string();
    
    for (const auto& pattern : options.exclude_patterns) {
        if (pattern.empty()) continue;
        
        // Simple glob pattern matching
        if (pattern.find('*') != std::string::npos) {
            // Convert glob to simple pattern
            std::string regex_pattern = pattern;
            for (size_t i = 0; i < regex_pattern.length(); ++i) {
                if (regex_pattern[i] == '*') {
                    regex_pattern.replace(i, 1, ".*");
                    i += 1; // Skip the added character
                }
            }
            regex_pattern = "^" + regex_pattern + "$";
            
            std::regex pattern_regex(regex_pattern);
            if (std::regex_match(filename, pattern_regex) ||
                std::regex_match(path, pattern_regex)) {
                return true;
            }
        } else if (filename == pattern || path == pattern) {
            return true;
        }
    }
    
    return false;
}

bool ContextBuilder::isPathAllowed(const std::string& path, const std::string& project_root) {
    try {
        std::filesystem::path fs_path(path);
        std::filesystem::path fs_project_root = std::filesystem::absolute(project_root);
        std::filesystem::path fs_absolute_path = std::filesystem::absolute(path);
        
        // Check if the absolute path is within the project root
        auto rel_path = std::filesystem::relative(fs_absolute_path, fs_project_root);
        
        // If the relative path starts with "..", it's outside the project
        std::string rel_str = rel_path.string();
        if (rel_str.substr(0, 2) == "..") {
            return false;
        }
        
        // Check if file exists and is regular file
        return std::filesystem::exists(fs_absolute_path) &&
               std::filesystem::is_regular_file(fs_absolute_path);
               
    } catch (const std::exception&) {
        return false;
    }
}

bool ContextBuilder::isAbsolutePath(const std::string& path) {
    return !path.empty() && (path[0] == '/' ||
           (path.length() > 1 && path[1] == ':'));
}

std::string ContextBuilder::normalizePath(const std::string& path) {
    try {
        std::filesystem::path fs_path(path);
        return std::filesystem::weakly_canonical(fs_path).string();
    } catch (const std::exception&) {
        return path; // Return original if normalization fails
    }
}

// Phase 3.3: Intelligent Context Selection Implementation

std::string ContextBuilder::processInclusionsWithIntelligence(const std::string& prompt,
                                                             const std::string& project_root,
                                                             const ContextOptions& options) {
    std::string result = prompt;
    auto inclusions = extractFileInclusions(prompt);
    
    // Process inclusions in reverse order to maintain position indices
    std::sort(inclusions.begin(), inclusions.end(),
              [](const FileInclusion& a, const FileInclusion& b) {
                  return a.start_position > b.start_position;
              });
    
    for (const auto& inclusion : inclusions) {
        try {
            std::string resolved_path = resolvePath(inclusion.file_path, project_root);
            
            // Security check: ensure path is within project root
            if (!isPathAllowed(resolved_path, project_root)) {
                std::string error_msg = "// Error: File '" + inclusion.file_path +
                                       "' is outside project directory or access denied";
                result.replace(inclusion.start_position, inclusion.full_match.length(), error_msg);
                continue;
            }
            
            // Check if file should be excluded
            if (shouldExcludeFile(resolved_path, options)) {
                std::string warning_msg = "// Warning: File '" + inclusion.file_path +
                                         "' matches exclude pattern";
                result.replace(inclusion.start_position, inclusion.full_match.length(), warning_msg);
                continue;
            }
            
            // Use intelligent analysis to determine inclusion type
            std::string replacement_content = analyzeFileRelevance(prompt, resolved_path, options);
            
            // Replace @file reference with analyzed content
            result.replace(inclusion.start_position, inclusion.full_match.length(), replacement_content);
            
        } catch (const std::exception& e) {
            std::string error_msg = "// Error reading file '" + inclusion.file_path +
                                   "': " + std::string(e.what());
            result.replace(inclusion.start_position, inclusion.full_match.length(), error_msg);
        }
    }
    
    return result;
}

std::string ContextBuilder::analyzeFileRelevance(const std::string& prompt,
                                                const std::string& file_path,
                                                const ContextOptions& options) {
    // Analyze file relevance using PromptAnalyzer
    clion::indexer::RelevanceScore score = clion::indexer::PromptAnalyzer::analyzeRelevance(
        prompt, file_path, options.analysis_options);
    
    std::string content;
    
    if (shouldIncludeFullFileIntelligently(prompt, file_path, options)) {
        // Include full file content
        content = readFileWithFormatting(file_path, options);
        
        // Check if we need to truncate
        if (options.truncate_large_files &&
            estimateTokenCount(content) > options.max_context_size) {
            content = truncateFile(content, options.max_context_size, file_path);
        }
        
        // Add relevance info if requested
        if (options.show_relevance_info) {
            content = formatRelevanceInfo(score, file_path) + "\n" + content;
        }
    } else {
        // Include summary instead of full file
        content = clion::indexer::PromptAnalyzer::generateSummary(file_path);
        
        // Add relevance info if requested
        if (options.show_relevance_info) {
            content = formatRelevanceInfo(score, file_path) + "\n" + content;
        }
        
        // Add note about summary usage
        content += "\n// Note: File summary shown instead of full content due to low relevance score.\n";
        content += "// Use @file " + file_path + " --force to include full file if needed.\n";
    }
    
    return content;
}

bool ContextBuilder::shouldIncludeFullFileIntelligently(const std::string& prompt,
                                                       const std::string& file_path,
                                                       const ContextOptions& options) {
    return clion::indexer::PromptAnalyzer::shouldIncludeFullFile(prompt, file_path);
}

std::string ContextBuilder::formatRelevanceInfo(const clion::indexer::RelevanceScore& score,
                                               const std::string& file_path) {
    std::ostringstream info;
    info << "// Relevance Analysis for: " << file_path << "\n";
    info << "// Score: " << std::fixed << std::setprecision(2) << score.score << " - " << score.reason << "\n";

    if (!score.matched_keywords.empty()) {
        info << "// Matched keywords: ";
        for (size_t i = 0; i < score.matched_keywords.size(); ++i) {
            if (i > 0) info << ", ";
            info << score.matched_keywords[i];
        }
        info << "\n";
    }

    return info.str();
}

// Enhanced memory integration implementations

std::string ContextBuilder::buildContextWithMemory(const std::string& base_prompt,
                                                 const std::string& project_root,
                                                 const ContextOptions& options,
                                                 const std::vector<std::string>& memory_node_ids) {
    // Build base context from files
    std::string context = buildContext(base_prompt, project_root, options);

    // Add memory context if nodes are specified
    if (!memory_node_ids.empty()) {
        std::string memory_context = injectMemoryContext(context, memory_node_ids, options);
        return memory_context;
    }

    // Auto-discover relevant memory nodes if enabled
    if (options.enable_memory_integration) {
        auto relevant_nodes = findRelevantMemoryNodes(base_prompt, options, options.max_memory_nodes);
        if (!relevant_nodes.empty()) {
            std::string memory_context = injectMemoryContext(context, relevant_nodes, options);
            return memory_context;
        }
    }

    return context;
}

std::string ContextBuilder::injectMemoryContext(const std::string& prompt,
                                              const std::vector<std::string>& memory_node_ids,
                                              const ContextOptions& options) {
    std::string result = prompt;

    if (memory_node_ids.empty()) {
        return result;
    }

    // Generate context from memory nodes
    std::string memory_context = generateMemoryContext(memory_node_ids, options.max_context_size / 2);

    if (!memory_context.empty()) {
        // Insert memory context at the beginning with clear separation
        std::string memory_section = "\n// ===== MEMORY CONTEXT =====\n";
        memory_section += memory_context;
        memory_section += "// ===== END MEMORY CONTEXT =====\n\n";

        result = memory_section + result;
    }

    return result;
}

std::vector<std::string> ContextBuilder::findRelevantMemoryNodes(const std::string& prompt,
                                                              const ContextOptions& options,
                                                              size_t max_nodes) {
    std::vector<std::string> relevant_nodes;

    try {
        // Extract keywords from prompt for memory search
        std::vector<std::string> keywords = ContextBuilder::extractKeywordsFromPrompt(prompt);

        // Search for memory nodes matching keywords
        for (const auto& keyword : keywords) {
            auto search_results = clion::llm::MemoryManager::searchMemoryNodes(keyword, {}, max_nodes * 2);

            for (const auto& node_id : search_results) {
                if (shouldIncludeMemoryInContext(prompt, node_id, options)) {
                    // Check if node is already in results
                    if (std::find(relevant_nodes.begin(), relevant_nodes.end(), node_id) == relevant_nodes.end()) {
                        relevant_nodes.push_back(node_id);

                        if (relevant_nodes.size() >= max_nodes) {
                            break;
                        }
                    }
                }
            }

            if (relevant_nodes.size() >= max_nodes) {
                break;
            }
        }

        // Also include recently accessed high-importance nodes
        if (relevant_nodes.size() < max_nodes) {
            auto recent_nodes = clion::llm::MemoryManager::getRecentlyAccessed(max_nodes - relevant_nodes.size());
            for (const auto& node_id : recent_nodes) {
                if (shouldIncludeMemoryInContext(prompt, node_id, options)) {
                    if (std::find(relevant_nodes.begin(), relevant_nodes.end(), node_id) == relevant_nodes.end()) {
                        relevant_nodes.push_back(node_id);
                    }
                }
            }
        }

    } catch (const std::exception& e) {
        // Log error but don't fail - memory integration is optional
        // In a real implementation, you might want to log this
    }

    return relevant_nodes;
}

std::string ContextBuilder::generateMemoryContext(const std::vector<std::string>& node_ids,
                                                size_t max_tokens) {
    return clion::llm::MemoryManager::generateContextFromMemory(node_ids, max_tokens);
}

std::string ContextBuilder::formatMemoryNodeForContext(const std::string& node_id) {
    auto node_opt = clion::llm::MemoryManager::getMemoryNode(node_id);
    if (!node_opt) {
        return "";
    }

    std::stringstream ss;
    const auto& node = *node_opt;

    ss << "## Memory Node: " << node.name << "\n";
    if (!node.description.empty()) {
        ss << "**Description:** " << node.description << "\n";
    }
    ss << "**Content:** " << node.content << "\n";

    if (!node.tags.empty()) {
        ss << "**Tags:** ";
        bool first = true;
        for (const auto& tag : node.tags) {
            if (!first) ss << ", ";
            ss << tag;
            first = false;
        }
        ss << "\n";
    }

    ss << "**Importance:** " << node.importance_score << "/100\n";
    ss << "**Access Count:** " << node.access_count << "\n";
    ss << "**Last Accessed:** " << node.last_accessed << "\n";
    ss << "\n";

    return ss.str();
}

bool ContextBuilder::shouldIncludeMemoryInContext(const std::string& prompt,
                                                const std::string& node_id,
                                                const ContextOptions& options) {
    auto node_opt = clion::llm::MemoryManager::getMemoryNode(node_id);
    if (!node_opt) {
        return false;
    }

    const auto& node = *node_opt;

    // Check importance threshold
    if (node.importance_score < 30) { // Minimum importance threshold
        return false;
    }

    // Check if node content is relevant to prompt
    std::vector<std::string> prompt_keywords = ContextBuilder::extractKeywordsFromPrompt(prompt);
    std::vector<std::string> node_keywords;

    // Extract keywords from node content
    for (const auto& tag : node.tags) {
        node_keywords.push_back(tag);
    }

    // Simple relevance check - look for keyword overlap
    for (const auto& prompt_keyword : prompt_keywords) {
        for (const auto& node_keyword : node_keywords) {
            if (prompt_keyword == node_keyword ||
                node.content.find(prompt_keyword) != std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

// Helper method to extract keywords from prompt
std::vector<std::string> ContextBuilder::extractKeywordsFromPrompt(const std::string& prompt) {
    std::vector<std::string> keywords;
    std::regex word_regex(R"(\b\w{4,}\b)"); // Words with 4+ characters
    std::sregex_iterator iter(prompt.begin(), prompt.end(), word_regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string word = (*iter).str();
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        keywords.push_back(word);
    }

    // Remove duplicates
    std::sort(keywords.begin(), keywords.end());
    keywords.erase(std::unique(keywords.begin(), keywords.end()), keywords.end());

    return keywords;
}

} // namespace llm
} // namespace clion
