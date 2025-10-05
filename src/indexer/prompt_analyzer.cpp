#include "prompt_analyzer.h"
#include "clion/common.h"
#include "project_scanner.h"
#include "../utils/file_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>

namespace clion {
namespace indexer {

bool PromptAnalyzer::shouldIncludeFullFile(const std::string& prompt, const std::string& file_path) {
    AnalysisOptions options;
    RelevanceScore score = analyzeRelevance(prompt, file_path, options);
    return meetsRelevanceThreshold(score, options);
}

std::string PromptAnalyzer::generateSummary(const std::string& file_path) {
    try {
        FileInfo file_info = CodeIndexer::indexFile(file_path);
        return generateFileSummary(file_info);
    } catch (const std::exception& e) {
        return "// Error generating summary for " + file_path + ": " + e.what();
    }
}

RelevanceScore PromptAnalyzer::analyzeRelevance(const std::string& prompt,
                                               const std::string& file_path,
                                               const AnalysisOptions& options) {
    RelevanceScore score;
    score.score = 0.0;
    score.reason = "No relevance found";
    
    try {
        // Extract keywords from prompt
        std::vector<std::string> prompt_keywords = extractKeywords(prompt, options);
        if (prompt_keywords.empty()) {
            score.reason = "No valid keywords found in prompt";
            return score;
        }
        
        // Get file information
        FileInfo file_info = CodeIndexer::indexFile(file_path);
        std::vector<std::string> file_terms = extractSearchableTerms(file_info, options);
        if (file_terms.empty()) {
            score.reason = "No searchable terms found in file";
            return score;
        }
        
        // Calculate keyword match score
        score.score = calculateKeywordMatch(prompt_keywords, file_terms);
        
        // Determine matched keywords for explanation
        for (const auto& prompt_keyword : prompt_keywords) {
            for (const auto& file_term : file_terms) {
                std::string normalized_prompt = normalizeKeyword(prompt_keyword);
                std::string normalized_file = normalizeKeyword(file_term);
                
                if (normalized_prompt == normalized_file) {
                    score.matched_keywords.push_back(prompt_keyword + " (exact match: " + file_term + ")");
                } else if (normalized_prompt.find(normalized_file) != std::string::npos ||
                          normalized_file.find(normalized_prompt) != std::string::npos) {
                    score.matched_keywords.push_back(prompt_keyword + " (partial match: " + file_term + ")");
                }
            }
        }
        
        // Generate reason
        if (score.score >= 0.8) {
            score.reason = "High relevance: strong keyword matches found";
        } else if (score.score >= 0.5) {
            score.reason = "Medium relevance: some keyword matches found";
        } else if (score.score >= 0.3) {
            score.reason = "Low relevance: weak keyword matches found";
        } else {
            score.reason = "No relevance: no significant keyword matches";
        }
        
    } catch (const std::exception& e) {
        score.score = 0.0;
        score.reason = "Error during analysis: " + std::string(e.what());
    }
    
    return score;
}

std::vector<std::string> PromptAnalyzer::extractKeywords(const std::string& text,
                                                        const AnalysisOptions& options) {
    std::vector<std::string> keywords;
    std::vector<std::string> words = splitIntoWords(text);
    
    for (const auto& word : words) {
        std::string normalized = normalizeKeyword(word);
        
        // Skip if too short or stop word
        if (normalized.length() < options.min_keyword_length ||
            isStopWord(normalized, options.stop_words)) {
            continue;
        }
        
        // Skip if already in list
        if (std::find(keywords.begin(), keywords.end(), normalized) != keywords.end()) {
            continue;
        }
        
        keywords.push_back(normalized);
    }
    
    return keywords;
}

std::vector<std::string> PromptAnalyzer::extractSearchableTerms(const FileInfo& file_info,
                                                               const AnalysisOptions& options) {
    std::vector<std::string> terms;
    
    // Extract function names
    if (options.include_function_names) {
        for (const auto& function : file_info.functions) {
            std::vector<std::string> function_words = splitIntoWords(function.name);
            for (const auto& word : function_words) {
                std::string normalized = normalizeKeyword(word);
                if (normalized.length() >= options.min_keyword_length &&
                    std::find(terms.begin(), terms.end(), normalized) == terms.end()) {
                    terms.push_back(normalized);
                }
            }
        }
    }
    
    // Extract class names
    if (options.include_class_names) {
        for (const auto& class_info : file_info.classes) {
            std::vector<std::string> class_words = splitIntoWords(class_info.name);
            for (const auto& word : class_words) {
                std::string normalized = normalizeKeyword(word);
                if (normalized.length() >= options.min_keyword_length &&
                    std::find(terms.begin(), terms.end(), normalized) == terms.end()) {
                    terms.push_back(normalized);
                }
            }
        }
    }
    
    // Extract include terms
    if (options.include_includes) {
        for (const auto& include : file_info.includes) {
            std::vector<std::string> include_words = splitIntoWords(include);
            for (const auto& word : include_words) {
                std::string normalized = normalizeKeyword(word);
                if (normalized.length() >= options.min_keyword_length &&
                    std::find(terms.begin(), terms.end(), normalized) == terms.end()) {
                    terms.push_back(normalized);
                }
            }
        }
    }
    
    return terms;
}

double PromptAnalyzer::calculateKeywordMatch(const std::vector<std::string>& prompt_keywords,
                                            const std::vector<std::string>& file_terms) {
    if (prompt_keywords.empty() || file_terms.empty()) {
        return 0.0;
    }
    
    double exact_score = calculateExactMatchScore(prompt_keywords, file_terms);
    double partial_score = calculatePartialMatchScore(prompt_keywords, file_terms);
    double contains_score = calculateContainsMatchScore(prompt_keywords, file_terms);
    
    // Weight the different types of matches
    double final_score = (exact_score * 1.0 + partial_score * 0.7 + contains_score * 0.5) / 2.2;
    
    return std::min(final_score, 1.0);
}

bool PromptAnalyzer::meetsRelevanceThreshold(const RelevanceScore& score,
                                            const AnalysisOptions& options) {
    return score.score >= options.relevance_threshold;
}

std::string PromptAnalyzer::normalizeKeyword(const std::string& keyword) {
    std::string normalized;
    for (char c : keyword) {
        if (std::isalnum(c)) {
            normalized += std::tolower(c);
        }
    }
    return normalized;
}

bool PromptAnalyzer::isStopWord(const std::string& word, const std::vector<std::string>& stop_words) {
    return std::find(stop_words.begin(), stop_words.end(), word) != stop_words.end();
}

std::vector<std::string> PromptAnalyzer::splitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        // Remove common punctuation
        word = std::regex_replace(word, std::regex("[^a-zA-Z0-9_]"), "");
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    
    return words;
}

std::string PromptAnalyzer::generateFileSummary(const FileInfo& file_info) {
    std::ostringstream summary;
    
    summary << "// File: " << file_info.file_path.string() << "\n";
    
    // Count functions and list key ones
    if (!file_info.functions.empty()) {
        summary << "// Functions: " << file_info.functions.size();
        size_t max_functions = std::min(file_info.functions.size(), static_cast<size_t>(5));
        for (size_t i = 0; i < max_functions; ++i) {
            if (i == 0) summary << " - ";
            else summary << ", ";
            summary << file_info.functions[i].name;
        }
        if (file_info.functions.size() > 5) {
            summary << " ...";
        }
        summary << "\n";
    }
    
    // Count classes and list key ones
    if (!file_info.classes.empty()) {
        summary << "// Classes: " << file_info.classes.size();
        size_t max_classes = std::min(file_info.classes.size(), static_cast<size_t>(3));
        for (size_t i = 0; i < max_classes; ++i) {
            if (i == 0) summary << " - ";
            else summary << ", ";
            summary << file_info.classes[i].name;
        }
        if (file_info.classes.size() > 3) {
            summary << " ...";
        }
        summary << "\n";
    }
    
    // List key includes
    if (!file_info.includes.empty()) {
        summary << "// Key Includes: ";
        size_t max_includes = std::min(file_info.includes.size(), static_cast<size_t>(5));
        for (size_t i = 0; i < max_includes; ++i) {
            if (i > 0) summary << ", ";
            summary << file_info.includes[i];
        }
        if (file_info.includes.size() > 5) {
            summary << " ...";
        }
        summary << "\n";
    }
    
    summary << "// Estimated content: " << (file_info.functions.size() + file_info.classes.size()) << " major elements\n";
    
    return summary.str();
}

double PromptAnalyzer::calculateExactMatchScore(const std::vector<std::string>& prompt_keywords,
                                               const std::vector<std::string>& file_terms) {
    if (prompt_keywords.empty() || file_terms.empty()) {
        return 0.0;
    }
    
    int matches = 0;
    for (const auto& prompt_keyword : prompt_keywords) {
        for (const auto& file_term : file_terms) {
            if (prompt_keyword == file_term) {
                matches++;
                break; // Count each prompt keyword only once
            }
        }
    }
    
    return static_cast<double>(matches) / prompt_keywords.size();
}

double PromptAnalyzer::calculatePartialMatchScore(const std::vector<std::string>& prompt_keywords,
                                                 const std::vector<std::string>& file_terms) {
    if (prompt_keywords.empty() || file_terms.empty()) {
        return 0.0;
    }
    
    int matches = 0;
    for (const auto& prompt_keyword : prompt_keywords) {
        for (const auto& file_term : file_terms) {
            if (prompt_keyword.find(file_term) != std::string::npos ||
                file_term.find(prompt_keyword) != std::string::npos) {
                matches++;
                break; // Count each prompt keyword only once
            }
        }
    }
    
    return static_cast<double>(matches) / prompt_keywords.size();
}

double PromptAnalyzer::calculateContainsMatchScore(const std::vector<std::string>& prompt_keywords,
                                                  const std::vector<std::string>& file_terms) {
    if (prompt_keywords.empty() || file_terms.empty()) {
        return 0.0;
    }
    
    int matches = 0;
    for (const auto& prompt_keyword : prompt_keywords) {
        for (const auto& file_term : file_terms) {
            if (prompt_keyword.length() >= 3 && file_term.find(prompt_keyword) != std::string::npos) {
                matches++;
                break; // Count each prompt keyword only once
            }
        }
    }
    
    return static_cast<double>(matches) / prompt_keywords.size();
}

} // namespace indexer
} // namespace clion
