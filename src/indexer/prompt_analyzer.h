#pragma once

#include <string>
#include <vector>
#include "clion/common.h"
#include "code_index.h"

namespace clion {
namespace indexer {

struct RelevanceScore {
    double score;                           // 0.0 to 1.0 relevance score
    std::string reason;                     // Human-readable explanation
    std::vector<std::string> matched_keywords;  // Matched keywords
};

struct AnalysisOptions {
    double relevance_threshold = 0.3;       // Minimum score for full inclusion
    bool include_function_names = true;      // Consider function names in matching
    bool include_class_names = true;         // Consider class names in matching
    bool include_includes = false;           // Consider includes in matching
    size_t min_keyword_length = 3;           // Minimum keyword length to consider
    std::vector<std::string> stop_words = {"the", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by", "from", "as", "is", "was", "are", "were"};
};

class PromptAnalyzer {
public:
    // Existing methods
    static bool shouldIncludeFullFile(const std::string& prompt, const std::string& file_path);
    static std::string generateSummary(const std::string& file_path);
    
    // New methods for intelligent analysis
    static RelevanceScore analyzeRelevance(const std::string& prompt,
                                          const std::string& file_path,
                                          const AnalysisOptions& options = {});
    static std::vector<std::string> extractKeywords(const std::string& text,
                                                   const AnalysisOptions& options = {});
    static std::vector<std::string> extractSearchableTerms(const FileInfo& file_info,
                                                          const AnalysisOptions& options = {});
    static double calculateKeywordMatch(const std::vector<std::string>& prompt_keywords,
                                       const std::vector<std::string>& file_terms);
    static bool meetsRelevanceThreshold(const RelevanceScore& score,
                                       const AnalysisOptions& options);
    static std::string normalizeKeyword(const std::string& keyword);
    static bool isStopWord(const std::string& word, const std::vector<std::string>& stop_words);
    static std::vector<std::string> splitIntoWords(const std::string& text);
    static std::string generateFileSummary(const FileInfo& file_info);

private:
    static double calculateExactMatchScore(const std::vector<std::string>& prompt_keywords,
                                          const std::vector<std::string>& file_terms);
    static double calculatePartialMatchScore(const std::vector<std::string>& prompt_keywords,
                                            const std::vector<std::string>& file_terms);
    static double calculateContainsMatchScore(const std::vector<std::string>& prompt_keywords,
                                             const std::vector<std::string>& file_terms);
};

} // namespace indexer
} // namespace clion
