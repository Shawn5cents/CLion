#pragma once

#include <string>
#include <vector>
#include <regex>
#include "clion/common.h"
#include "../indexer/prompt_analyzer.h"

namespace clion {
namespace llm {

struct FileInclusion {
    std::string file_path;
    size_t start_position;
    size_t end_position;
    std::string full_match;
};

struct ContextOptions {
    size_t max_context_size = 8192;
    bool include_line_numbers = true;
    std::string file_header_format = "// File: {path}\n";
    bool truncate_large_files = true;
    size_t truncation_warning_size = 1000;
    std::vector<std::string> exclude_patterns = {"*.o", "*.exe", "build/*", "vendor/*"};

    // Phase 3.3: Intelligent Context Selection options
    bool enable_intelligent_selection = true;
    clion::indexer::AnalysisOptions analysis_options;
    bool show_relevance_info = false;
    bool confirm_ambiguous_files = false;

    // Enhanced memory integration options
    bool enable_memory_integration = true;
    size_t max_memory_nodes = 5;
    size_t max_memory_context_size = 2000;
    size_t min_memory_importance = 30;
};

class ContextBuilder {
public:
    static std::string buildContext(const std::string& base_prompt,
                                   const std::string& project_root = ".",
                                   const ContextOptions& options = {});
    static std::vector<FileInclusion> extractFileInclusions(const std::string& prompt);
    static std::string injectFileContents(const std::string& prompt,
                                        const std::string& project_root = ".",
                                        const ContextOptions& options = {});

private:
    static std::string processInclusions(const std::string& prompt,
                                       const std::string& project_root,
                                       const ContextOptions& options);
    static std::string resolvePath(const std::string& path, const std::string& project_root);
    static std::string readFileWithFormatting(const std::string& path,
                                            const ContextOptions& options);
    static std::string truncateFile(const std::string& content,
                                  size_t max_size,
                                  const std::string& file_path);
    static size_t estimateTokenCount(const std::string& text);
    static bool shouldExcludeFile(const std::string& path, const ContextOptions& options);
    static bool isPathAllowed(const std::string& path, const std::string& project_root);
    static bool isAbsolutePath(const std::string& path);
    static std::string normalizePath(const std::string& path);
    
    // Phase 3.3: Intelligent Context Selection methods
    static std::string processInclusionsWithIntelligence(const std::string& prompt,
                                                        const std::string& project_root,
                                                        const ContextOptions& options);
    static std::string analyzeFileRelevance(const std::string& prompt,
                                           const std::string& file_path,
                                           const ContextOptions& options);
    static bool shouldIncludeFullFileIntelligently(const std::string& prompt,
                                                  const std::string& file_path,
                                                  const ContextOptions& options);
    static std::string formatRelevanceInfo(const clion::indexer::RelevanceScore& score,
                                           const std::string& file_path);

    // Enhanced memory integration methods
    static std::string buildContextWithMemory(const std::string& base_prompt,
                                            const std::string& project_root,
                                            const ContextOptions& options,
                                            const std::vector<std::string>& memory_node_ids = {});

    static std::string injectMemoryContext(const std::string& prompt,
                                         const std::vector<std::string>& memory_node_ids,
                                         const ContextOptions& options);

    static std::vector<std::string> findRelevantMemoryNodes(const std::string& prompt,
                                                          const ContextOptions& options,
                                                          size_t max_nodes = 5);

    static std::string generateMemoryContext(const std::vector<std::string>& node_ids,
                                           size_t max_tokens = 2000);

    static std::string formatMemoryNodeForContext(const std::string& node_id);

    static bool shouldIncludeMemoryInContext(const std::string& prompt,
                                           const std::string& node_id,
                                           const ContextOptions& options);

    static std::vector<std::string> extractKeywordsFromPrompt(const std::string& prompt);

    static const std::regex INCLUSION_PATTERN;
};

} // namespace llm
} // namespace clion