#include "diff_utils.h"
#include "clion/common.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace clion {
namespace utils {

std::string DiffUtils::generateUnifiedDiff(const std::string& original, const std::string& modified,
                                          const std::string& original_file, const std::string& modified_file) {
    std::stringstream diff;
    
    // Split strings into lines
    std::vector<std::string> original_lines = splitIntoLines(original);
    std::vector<std::string> modified_lines = splitIntoLines(modified);
    
    // Generate diff header
    diff << "--- " << original_file << "\n";
    diff << "+++ " << modified_file << "\n";
    
    // Find all changes
    std::vector<size_t> changed_lines;
    for (size_t i = 0; i < std::max(original_lines.size(), modified_lines.size()); ++i) {
        bool line_changed = false;
        
        if (i < original_lines.size() && i < modified_lines.size()) {
            if (original_lines[i] != modified_lines[i]) {
                line_changed = true;
            }
        } else if (i < original_lines.size()) {
            // Line removed
            line_changed = true;
        } else if (i < modified_lines.size()) {
            // Line added
            line_changed = true;
        }
        
        if (line_changed) {
            changed_lines.push_back(i);
        }
    }
    
    // Group changes into hunks
    if (!changed_lines.empty()) {
        for (size_t i = 0; i < changed_lines.size(); ++i) {
            size_t start = changed_lines[i];
            size_t end = start;
            
            // Find the end of the current hunk (allow gaps of up to 3 lines)
            while ((i + 1) < changed_lines.size() && changed_lines[i + 1] <= end + 3) {
                end = changed_lines[i + 1];
                i++;
            }
            
            // Generate hunk
            int old_start = static_cast<int>(start) + 1;
            int new_start = static_cast<int>(start) + 1;
            int old_count = 0;
            int new_count = 0;
            
            std::stringstream hunk_content;
            
            for (size_t j = start; j <= end; ++j) {
                if (j < original_lines.size() && j < modified_lines.size()) {
                    if (original_lines[j] != modified_lines[j]) {
                        hunk_content << "- " << original_lines[j] << "\n";
                        hunk_content << "+ " << modified_lines[j] << "\n";
                        old_count++;
                        new_count++;
                    } else {
                        hunk_content << "  " << original_lines[j] << "\n";
                        old_count++;
                        new_count++;
                    }
                } else if (j < original_lines.size()) {
                    // Line removed
                    hunk_content << "- " << original_lines[j] << "\n";
                    old_count++;
                } else if (j < modified_lines.size()) {
                    // Line added
                    hunk_content << "+ " << modified_lines[j] << "\n";
                    new_count++;
                }
            }
            
            // Output hunk header
            diff << "@@ -" << old_start;
            if (old_count > 1) diff << "," << old_count;
            diff << " +" << new_start;
            if (new_count > 1) diff << "," << new_count;
            diff << " @@\n";
            
            // Output hunk content
            diff << hunk_content.str();
        }
    }
    
    return diff.str();
}

std::vector<DiffHunk> DiffUtils::parseDiff(const std::string& diff) {
    std::vector<DiffHunk> hunks;
    std::stringstream diff_stream(diff);
    std::string line;
    
    DiffHunk current_hunk;
    bool in_hunk = false;
    
    while (std::getline(diff_stream, line)) {
        if (line.empty()) continue;
        
        if (line[0] == '@') {
            // Start of a new hunk
            if (in_hunk) {
                hunks.push_back(current_hunk);
            }
            
            current_hunk = DiffHunk();
            in_hunk = true;
            
            // Parse hunk header: @@ -old_start,old_count +new_start,new_count @@
            size_t at_pos = line.find("@@");
            if (at_pos != std::string::npos) {
                size_t plus_pos = line.find("+", at_pos);
                if (plus_pos != std::string::npos) {
                    // Extract old start and count
                    std::string old_part = line.substr(at_pos + 3, plus_pos - at_pos - 4);
                    size_t comma_pos = old_part.find(",");
                    if (comma_pos != std::string::npos) {
                        current_hunk.old_start = std::stoi(old_part.substr(0, comma_pos));
                        current_hunk.old_count = std::stoi(old_part.substr(comma_pos + 1));
                    } else {
                        current_hunk.old_start = std::stoi(old_part);
                        current_hunk.old_count = 1;
                    }
                    
                    // Extract new start and count
                    size_t space_pos = line.find(" ", plus_pos);
                    if (space_pos != std::string::npos) {
                        std::string new_part = line.substr(plus_pos + 1, space_pos - plus_pos - 1);
                        comma_pos = new_part.find(",");
                        if (comma_pos != std::string::npos) {
                            current_hunk.new_start = std::stoi(new_part.substr(0, comma_pos));
                            current_hunk.new_count = std::stoi(new_part.substr(comma_pos + 1));
                        } else {
                            current_hunk.new_start = std::stoi(new_part);
                            current_hunk.new_count = 1;
                        }
                    }
                }
            }
        } else if (in_hunk && (line[0] == ' ' || line[0] == '-' || line[0] == '+')) {
            // Diff line
            DiffLine diff_line;
            diff_line.type = line[0];
            diff_line.content = line.substr(1);  // Remove the first character
            
            current_hunk.lines.push_back(diff_line);
        }
    }
    
    // Add the last hunk if we're still in one
    if (in_hunk) {
        hunks.push_back(current_hunk);
    }
    
    return hunks;
}

std::string DiffUtils::applyDiff(const std::string& original, const std::vector<DiffHunk>& hunks) {
    std::vector<std::string> original_lines = splitIntoLines(original);
    std::vector<std::string> result_lines;
    
    size_t old_line_idx = 0;
    
    // Apply hunks in order
    for (const auto& hunk : hunks) {
        // Copy lines up to the start of the hunk
        while (old_line_idx < static_cast<size_t>(hunk.old_start - 1) && old_line_idx < original_lines.size()) {
            result_lines.push_back(original_lines[old_line_idx]);
            old_line_idx++;
        }
        
        // Apply the hunk
        for (const auto& line : hunk.lines) {
            if (line.type == ' ') {
                // Unchanged line - copy from original
                if (old_line_idx < original_lines.size()) {
                    result_lines.push_back(original_lines[old_line_idx]);
                    old_line_idx++;
                }
            } else if (line.type == '-') {
                // Removed line - skip from original
                old_line_idx++;
            } else if (line.type == '+') {
                // Added line - add to result
                result_lines.push_back(line.content);
                // Don't increment old_line_idx for added lines
            }
        }
    }
    
    // Copy any remaining lines from original
    while (old_line_idx < original_lines.size()) {
        result_lines.push_back(original_lines[old_line_idx]);
        old_line_idx++;
    }
    
    // Join lines back into a string
    std::stringstream result;
    for (size_t i = 0; i < result_lines.size(); ++i) {
        result << result_lines[i];
        if (i < result_lines.size() - 1) {
            result << "\n";
        }
    }
    
    return result.str();
}

void DiffUtils::displayDiff(const std::string& diff) {
    std::stringstream diff_stream(diff);
    std::string line;
    
    while (std::getline(diff_stream, line)) {
        if (line.empty()) {
            std::cout << std::endl;
            continue;
        }
        
        if (line[0] == '@') {
            // Hunk header - display in cyan
            std::cout << "\033[36m" << line << "\033[0m" << std::endl;
        } else if (line[0] == '-') {
            // Removed line - display in red
            std::cout << "\033[31m" << line << "\033[0m" << std::endl;
        } else if (line[0] == '+') {
            // Added line - display in green
            std::cout << "\033[32m" << line << "\033[0m" << std::endl;
        } else {
            // Unchanged line
            std::cout << line << std::endl;
        }
    }
}

std::vector<std::string> DiffUtils::splitIntoLines(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

} // namespace utils
} // namespace clion
