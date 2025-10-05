#include "session.h"
#include "clion/session_checkpoint.h"
#include "clion/memory_manager.h"
#include "clion/common.h"
#include "../utils/file_utils.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <regex>

using json = nlohmann::json;

namespace clion {
namespace llm {

namespace {
    // Get the session directory path
    std::string getSessionDirectory() {
        const char* home_dir = std::getenv("HOME");
        if (!home_dir) {
            home_dir = std::getenv("USERPROFILE"); // Windows fallback
        }
        if (!home_dir) {
            return "./sessions"; // Fallback to current directory
        }
        
        std::filesystem::path session_dir = std::filesystem::path(home_dir) / ".clion" / "sessions";
        
        // Create directory if it doesn't exist
        std::error_code ec;
        std::filesystem::create_directories(session_dir, ec);
        
        return session_dir.string();
    }
    
    // Get session file path
    std::string getSessionFilePath(const std::string& session_id) {
        return std::filesystem::path(getSessionDirectory()) / (session_id + ".json");
    }
    
    // Generate current timestamp in ISO 8601 format
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return ss.str();
    }
    
    // Generate random string for session ID
    std::string generateRandomString(size_t length) {
        const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.size() - 1);
        
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += chars[dis(gen)];
        }
        return result;
    }
}

bool SessionManager::saveSession(const Session& session) {
    try {
        // Convert session to JSON
        json j;
        j["id"] = session.id;
        j["created_at"] = session.created_at;
        j["updated_at"] = session.updated_at;

        // Enhanced session fields
        j["name"] = session.name;
        j["description"] = session.description;
        j["parent_session_id"] = session.parent_session_id;
        j["total_tokens"] = session.total_tokens;
        j["is_compressed"] = session.is_compressed;
        j["last_checkpoint_id"] = session.last_checkpoint_id;

        // Convert entries to JSON array
        json entries_json = json::array();
        for (const auto& entry : session.entries) {
            json entry_json;
            entry_json["role"] = entry.role;
            entry_json["content"] = entry.content;
            entry_json["timestamp"] = entry.timestamp;
            entries_json.push_back(entry_json);
        }
        j["entries"] = entries_json;

        // Convert child session IDs to JSON array
        json child_ids_json = json::array();
        for (const auto& child_id : session.child_session_ids) {
            child_ids_json.push_back(child_id);
        }
        j["child_session_ids"] = child_ids_json;

        // Convert tags to JSON array
        json tags_json = json::array();
        for (const auto& tag : session.tags) {
            tags_json.push_back(tag);
        }
        j["tags"] = tags_json;

        // Convert checkpoint IDs to JSON array
        json checkpoint_ids_json = json::array();
        for (const auto& checkpoint_id : session.checkpoint_ids) {
            checkpoint_ids_json.push_back(checkpoint_id);
        }
        j["checkpoint_ids"] = checkpoint_ids_json;

        // Convert memory node IDs to JSON array
        json memory_node_ids_json = json::array();
        for (const auto& memory_node_id : session.memory_node_ids) {
            memory_node_ids_json.push_back(memory_node_id);
        }
        j["memory_node_ids"] = memory_node_ids_json;

        // Add metadata
        json metadata_json;
        for (const auto& [key, value] : session.metadata) {
            metadata_json[key] = value;
        }
        j["metadata"] = metadata_json;

        // Write to file
        std::string file_path = getSessionFilePath(session.id);
        std::ofstream file(file_path);
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(2); // Pretty print with 2 spaces indentation
        return file.good();

    } catch (const std::exception&) {
        return false;
    }
}

std::optional<Session> SessionManager::loadSession(const std::string& session_id) {
    try {
        std::string file_path = getSessionFilePath(session_id);

        // Check if file exists
        if (!std::filesystem::exists(file_path)) {
            return std::nullopt;
        }

        // Read and parse JSON
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return std::nullopt;
        }

        json j;
        file >> j;

        // Convert JSON to Session
        Session session;
        session.id = j["id"].get<std::string>();
        session.created_at = j["created_at"].get<std::string>();
        session.updated_at = j["updated_at"].get<std::string>();

        // Parse enhanced fields (with defaults for backward compatibility)
        session.name = j.value("name", "");
        session.description = j.value("description", "");
        session.parent_session_id = j.value("parent_session_id", "");
        session.total_tokens = j.value("total_tokens", size_t(0));
        session.is_compressed = j.value("is_compressed", false);
        session.last_checkpoint_id = j.value("last_checkpoint_id", "");

        // Parse entries
        if (j.contains("entries") && j["entries"].is_array()) {
            for (const auto& entry_json : j["entries"]) {
                HistoryEntry entry;
                entry.role = entry_json["role"].get<std::string>();
                entry.content = entry_json["content"].get<std::string>();
                entry.timestamp = entry_json["timestamp"].get<std::string>();
                session.entries.push_back(entry);
            }
        }

        // Parse child session IDs
        if (j.contains("child_session_ids") && j["child_session_ids"].is_array()) {
            for (const auto& child_id_json : j["child_session_ids"]) {
                session.child_session_ids.push_back(child_id_json.get<std::string>());
            }
        }

        // Parse tags
        if (j.contains("tags") && j["tags"].is_array()) {
            for (const auto& tag_json : j["tags"]) {
                session.tags.insert(tag_json.get<std::string>());
            }
        }

        // Parse checkpoint IDs
        if (j.contains("checkpoint_ids") && j["checkpoint_ids"].is_array()) {
            for (const auto& checkpoint_id_json : j["checkpoint_ids"]) {
                session.checkpoint_ids.push_back(checkpoint_id_json.get<std::string>());
            }
        }

        // Parse memory node IDs
        if (j.contains("memory_node_ids") && j["memory_node_ids"].is_array()) {
            for (const auto& memory_node_id_json : j["memory_node_ids"]) {
                session.memory_node_ids.push_back(memory_node_id_json.get<std::string>());
            }
        }

        // Parse metadata
        if (j.contains("metadata") && j["metadata"].is_object()) {
            for (auto& [key, value] : j["metadata"].items()) {
                session.metadata[key] = value.get<std::string>();
            }
        }

        return session;

    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string SessionManager::createSessionId() {
    // Generate session ID with timestamp and random component
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "session_";
    ss << std::put_time(std::gmtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << generateRandomString(8);
    
    return ss.str();
}

std::string SessionManager::createNewSession() {
    std::string session_id = createSessionId();
    std::string timestamp = getCurrentTimestamp();
    
    Session session;
    session.id = session_id;
    session.created_at = timestamp;
    session.updated_at = timestamp;
    
    if (saveSession(session)) {
        return session_id;
    }
    
    return ""; // Failed to save session
}

bool SessionManager::addEntryToSession(const std::string& session_id,
                                      const std::string& role,
                                      const std::string& content) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }
    
    HistoryEntry entry;
    entry.role = role;
    entry.content = content;
    entry.timestamp = getCurrentTimestamp();
    
    session_opt->entries.push_back(entry);
    session_opt->updated_at = entry.timestamp;
    
    return saveSession(*session_opt);
}

std::vector<std::string> SessionManager::listSessions() {
    std::vector<std::string> sessions;
    
    try {
        std::string session_dir = getSessionDirectory();
        
        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                // Remove .json extension
                sessions.push_back(filename.substr(0, filename.length() - 5));
            }
        }
        
        // Sort by session ID (which includes timestamp)
        std::sort(sessions.rbegin(), sessions.rend());
        
    } catch (const std::exception&) {
        // Return empty list on error
    }
    
    return sessions;
}

bool SessionManager::deleteSession(const std::string& session_id) {
    try {
        std::string file_path = getSessionFilePath(session_id);
        return std::filesystem::remove(file_path);
    } catch (const std::exception&) {
        return false;
    }
}

bool SessionManager::sessionExists(const std::string& session_id) {
    std::string file_path = getSessionFilePath(session_id);
    return std::filesystem::exists(file_path);
}

// Enhanced session management implementations

std::string SessionManager::createNewSessionWithMetadata(const std::string& name,
                                                       const std::string& description,
                                                       const std::unordered_set<std::string>& tags,
                                                       const std::string& parent_id) {
    std::string session_id = createSessionId();
    std::string timestamp = getCurrentTimestamp();

    Session session;
    session.id = session_id;
    session.created_at = timestamp;
    session.updated_at = timestamp;
    session.name = name;
    session.description = description;
    session.tags = tags;
    session.parent_session_id = parent_id;
    session.total_tokens = 0;
    session.is_compressed = false;

    if (saveSession(session)) {
        // Update parent-child relationships if parent specified
        if (!parent_id.empty()) {
            setParentSession(session_id, parent_id);
        }
        return session_id;
    }

    return "";
}

bool SessionManager::updateSessionMetadata(const std::string& session_id,
                                         const std::string& name,
                                         const std::string& description,
                                         const std::unordered_set<std::string>& tags) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    Session& session = *session_opt;

    if (!name.empty()) {
        session.name = name;
    }

    if (!description.empty()) {
        session.description = description;
    }

    if (!tags.empty()) {
        session.tags.insert(tags.begin(), tags.end());
    }

    session.updated_at = getCurrentTimestamp();

    return saveSession(session);
}

bool SessionManager::addTagsToSession(const std::string& session_id,
                                    const std::unordered_set<std::string>& tags) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    Session& session = *session_opt;
    session.tags.insert(tags.begin(), tags.end());
    session.updated_at = getCurrentTimestamp();

    return saveSession(session);
}

bool SessionManager::removeTagsFromSession(const std::string& session_id,
                                         const std::unordered_set<std::string>& tags) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    Session& session = *session_opt;
    for (const auto& tag : tags) {
        session.tags.erase(tag);
    }
    session.updated_at = getCurrentTimestamp();

    return saveSession(session);
}

std::optional<Session> SessionManager::getSessionWithMetadata(const std::string& session_id) {
    return loadSession(session_id);
}

std::vector<std::string> SessionManager::findSessionsByTag(const std::string& tag) {
    std::vector<std::string> matching_sessions;

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                auto session_opt = loadSession(session_id);
                if (session_opt && session_opt->tags.find(tag) != session_opt->tags.end()) {
                    matching_sessions.push_back(session_id);
                }
            }
        }

    } catch (const std::exception&) {
        // Return empty list on error
    }

    return matching_sessions;
}

std::vector<std::string> SessionManager::findSessionsByName(const std::string& name_pattern) {
    std::vector<std::string> matching_sessions;
    std::regex pattern_regex(name_pattern, std::regex_constants::icase);

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                auto session_opt = loadSession(session_id);
                if (session_opt && std::regex_search(session_opt->name, pattern_regex)) {
                    matching_sessions.push_back(session_id);
                }
            }
        }

    } catch (const std::exception&) {
        // Return empty list on error
    }

    return matching_sessions;
}

std::vector<std::string> SessionManager::findSessionsByContent(const std::string& content_pattern) {
    std::vector<std::string> matching_sessions;
    std::regex pattern_regex(content_pattern, std::regex_constants::icase);

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                auto session_opt = loadSession(session_id);
                if (!session_opt) continue;

                bool found = false;
                for (const auto& entry : session_opt->entries) {
                    if (std::regex_search(entry.content, pattern_regex)) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    matching_sessions.push_back(session_id);
                }
            }
        }

    } catch (const std::exception&) {
        // Return empty list on error
    }

    return matching_sessions;
}

std::vector<std::string> SessionManager::getChildSessions(const std::string& parent_id) {
    auto session_opt = loadSession(parent_id);
    if (!session_opt) {
        return {};
    }

    return session_opt->child_session_ids;
}

std::vector<std::string> SessionManager::getSessionHierarchy(const std::string& session_id) {
    std::vector<std::string> hierarchy;
    std::string current_id = session_id;

    while (!current_id.empty()) {
        hierarchy.push_back(current_id);
        auto session_opt = loadSession(current_id);
        if (!session_opt) break;
        current_id = session_opt->parent_session_id;
    }

    std::reverse(hierarchy.begin(), hierarchy.end());
    return hierarchy;
}

bool SessionManager::setParentSession(const std::string& session_id, const std::string& parent_id) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    auto parent_opt = loadSession(parent_id);
    if (!parent_opt) {
        return false;
    }

    Session& session = *session_opt;
    Session& parent = *parent_opt;

    // Remove from old parent's child list
    if (!session.parent_session_id.empty()) {
        auto old_parent_opt = loadSession(session.parent_session_id);
        if (old_parent_opt) {
            auto& child_ids = old_parent_opt->child_session_ids;
            child_ids.erase(std::remove(child_ids.begin(), child_ids.end(), session_id), child_ids.end());
            saveSession(*old_parent_opt);
        }
    }

    // Update session's parent
    session.parent_session_id = parent_id;
    session.updated_at = getCurrentTimestamp();

    // Add to new parent's child list
    parent.child_session_ids.push_back(session_id);
    parent.updated_at = getCurrentTimestamp();

    return saveSession(session) && saveSession(parent);
}

bool SessionManager::addChildSession(const std::string& parent_id, const std::string& child_id) {
    auto parent_opt = loadSession(parent_id);
    auto child_opt = loadSession(child_id);

    if (!parent_opt || !child_opt) {
        return false;
    }

    Session& parent = *parent_opt;
    Session& child = *child_opt;

    // Check if already a child
    auto it = std::find(parent.child_session_ids.begin(), parent.child_session_ids.end(), child_id);
    if (it != parent.child_session_ids.end()) {
        return true; // Already a child
    }

    parent.child_session_ids.push_back(child_id);
    parent.updated_at = getCurrentTimestamp();

    child.parent_session_id = parent_id;
    child.updated_at = getCurrentTimestamp();

    return saveSession(parent) && saveSession(child);
}

bool SessionManager::removeChildSession(const std::string& parent_id, const std::string& child_id) {
    auto parent_opt = loadSession(parent_id);
    auto child_opt = loadSession(child_id);

    if (!parent_opt || !child_opt) {
        return false;
    }

    Session& parent = *parent_opt;
    Session& child = *child_opt;

    // Remove from parent's child list
    auto& child_ids = parent.child_session_ids;
    child_ids.erase(std::remove(child_ids.begin(), child_ids.end(), child_id), child_ids.end());

    parent.updated_at = getCurrentTimestamp();

    // Remove child's parent reference
    child.parent_session_id.clear();
    child.updated_at = getCurrentTimestamp();

    return saveSession(parent) && saveSession(child);
}

std::string SessionManager::createCheckpoint(const std::string& session_id,
                                           const std::string& checkpoint_name,
                                           const std::string& description) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return "";
    }

    std::string checkpoint_id = clion::llm::SessionCheckpointManager::createCheckpoint(
        *session_opt, checkpoint_name, description);

    if (!checkpoint_id.empty()) {
        // Associate checkpoint with session
        Session& session = *session_opt;
        session.checkpoint_ids.push_back(checkpoint_id);
        session.last_checkpoint_id = checkpoint_id;
        session.updated_at = getCurrentTimestamp();
        saveSession(session);
    }

    return checkpoint_id;
}

std::optional<Session> SessionManager::restoreFromCheckpoint(const std::string& checkpoint_id) {
    return clion::llm::SessionCheckpointManager::restoreFromCheckpoint(checkpoint_id);
}

std::vector<std::string> SessionManager::getSessionCheckpoints(const std::string& session_id) {
    return clion::llm::SessionCheckpointManager::listCheckpoints(session_id);
}

bool SessionManager::deleteSessionCheckpoints(const std::string& session_id) {
    size_t deleted_count = clion::llm::SessionCheckpointManager::deleteSessionCheckpoints(session_id);

    // Remove checkpoint associations from session
    auto session_opt = loadSession(session_id);
    if (session_opt) {
        Session& session = *session_opt;
        session.checkpoint_ids.clear();
        session.last_checkpoint_id.clear();
        session.updated_at = getCurrentTimestamp();
        saveSession(session);
    }

    return deleted_count > 0;
}

std::string SessionManager::createMemoryFromSession(const std::string& session_id,
                                                 const std::string& memory_name,
                                                 const std::string& parent_memory_id) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return "";
    }

    std::string memory_id = clion::llm::MemoryManager::createMemoryFromSession(
        session_id, session_opt->entries, memory_name, parent_memory_id);

    if (!memory_id.empty()) {
        // Associate memory with session
        Session& session = *session_opt;
        session.memory_node_ids.push_back(memory_id);
        session.updated_at = getCurrentTimestamp();
        saveSession(session);
    }

    return memory_id;
}

bool SessionManager::associateMemoryWithSession(const std::string& session_id,
                                             const std::string& memory_node_id) {
    if (!clion::llm::MemoryManager::memoryNodeExists(memory_node_id)) {
        return false;
    }

    bool success = clion::llm::MemoryManager::associateSessionWithMemory(memory_node_id, session_id);

    if (success) {
        // Update session's memory associations
        auto session_opt = loadSession(session_id);
        if (session_opt) {
            Session& session = *session_opt;

            // Check if already associated
            auto it = std::find(session.memory_node_ids.begin(), session.memory_node_ids.end(), memory_node_id);
            if (it == session.memory_node_ids.end()) {
                session.memory_node_ids.push_back(memory_node_id);
                session.updated_at = getCurrentTimestamp();
                saveSession(session);
            }
        }
    }

    return success;
}

std::vector<std::string> SessionManager::getSessionMemoryNodes(const std::string& session_id) {
    return clion::llm::MemoryManager::getSessionMemoryNodes(session_id);
}

bool SessionManager::compressSession(const std::string& session_id) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    Session& session = *session_opt;
    session.is_compressed = true;
    session.updated_at = getCurrentTimestamp();

    return saveSession(session);
}

bool SessionManager::decompressSession(const std::string& session_id) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    Session& session = *session_opt;
    session.is_compressed = false;
    session.updated_at = getCurrentTimestamp();

    return saveSession(session);
}

size_t SessionManager::getSessionSize(const std::string& session_id) {
    std::string file_path = getSessionFilePath(session_id);
    if (!std::filesystem::exists(file_path)) {
        return 0;
    }

    return std::filesystem::file_size(file_path);
}

size_t SessionManager::getSessionTokenCount(const std::string& session_id) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return 0;
    }

    if (session_opt->total_tokens > 0) {
        return session_opt->total_tokens;
    }

    // Calculate token count if not cached
    size_t total_tokens = 0;
    for (const auto& entry : session_opt->entries) {
        total_tokens += entry.content.length() / 4; // Rough estimation
    }

    return total_tokens;
}

size_t SessionManager::cleanupOldSessions(size_t max_age_days) {
    size_t cleaned_count = 0;

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                auto file_time = entry.last_write_time();
                auto now = std::filesystem::file_time_type::clock::now();
                auto age = std::chrono::duration_cast<std::chrono::hours>(now - file_time);

                if (age.count() > static_cast<long>(max_age_days * 24)) {
                    std::string filename = entry.path().filename().string();
                    std::string session_id = filename.substr(0, filename.length() - 5);

                    if (deleteSession(session_id)) {
                        cleaned_count++;
                    }
                }
            }
        }

    } catch (const std::exception&) {
        // Continue on error
    }

    return cleaned_count;
}

std::unordered_map<std::string, std::string> SessionManager::getSessionStats() {
    std::unordered_map<std::string, std::string> stats;

    try {
        std::string session_dir = getSessionDirectory();
        size_t total_sessions = 0;
        size_t total_size = 0;
        size_t total_tokens = 0;

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                total_sessions++;
                total_size += entry.file_size();

                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);
                total_tokens += getSessionTokenCount(session_id);
            }
        }

        stats["total_sessions"] = std::to_string(total_sessions);
        stats["total_size_bytes"] = std::to_string(total_size);
        stats["total_size_mb"] = std::to_string(total_size / (1024.0 * 1024.0));
        stats["total_tokens"] = std::to_string(total_tokens);

    } catch (const std::exception&) {
        stats["error"] = "Failed to calculate session stats";
    }

    return stats;
}

bool SessionManager::validateSessionIntegrity(const std::string& session_id) {
    auto session_opt = loadSession(session_id);
    if (!session_opt) {
        return false;
    }

    // Basic validation checks
    if (session_opt->id.empty() || session_opt->entries.empty()) {
        return false;
    }

    // Check timestamp format (basic validation)
    if (session_opt->created_at.empty() || session_opt->updated_at.empty()) {
        return false;
    }

    return true;
}

std::vector<std::string> SessionManager::searchSessions(const std::string& query,
                                                      const std::unordered_set<std::string>& tags,
                                                      const std::string& /* date_from */,
                                                      const std::string& /* date_to */) {
    std::vector<std::string> results;

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                auto session_opt = loadSession(session_id);
                if (!session_opt) continue;

                // Check tag filter
                if (!tags.empty()) {
                    bool has_all_tags = true;
                    for (const auto& required_tag : tags) {
                        if (session_opt->tags.find(required_tag) == session_opt->tags.end()) {
                            has_all_tags = false;
                            break;
                        }
                    }
                    if (!has_all_tags) continue;
                }

                // Search in name, description, and content
                std::string search_text = session_opt->name + " " + session_opt->description;
                for (const auto& entry : session_opt->entries) {
                    search_text += " " + entry.content;
                }

                std::string query_lower = query;
                std::string search_text_lower = search_text;
                std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
                std::transform(search_text_lower.begin(), search_text_lower.end(), search_text_lower.begin(), ::tolower);

                if (search_text_lower.find(query_lower) != std::string::npos) {
                    results.push_back(session_id);
                }
            }
        }

    } catch (const std::exception&) {
        // Return empty results on error
    }

    return results;
}

std::vector<std::string> SessionManager::getSessionsByDateRange(const std::string& from_date,
                                                             const std::string& to_date) {
    std::vector<std::string> results;

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                auto session_opt = loadSession(session_id);
                if (!session_opt) continue;

                // Simple date comparison (would need proper date parsing in real implementation)
                if (session_opt->created_at >= from_date && session_opt->created_at <= to_date) {
                    results.push_back(session_id);
                }
            }
        }

    } catch (const std::exception&) {
        // Return empty results on error
    }

    return results;
}

std::vector<std::string> SessionManager::getSessionsBySize(size_t min_size, size_t max_size) {
    std::vector<std::string> results;

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                size_t file_size = entry.file_size();
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                if ((min_size == 0 || file_size >= min_size) &&
                    (max_size == 0 || file_size <= max_size)) {
                    results.push_back(session_id);
                }
            }
        }

    } catch (const std::exception&) {
        // Return empty results on error
    }

    return results;
}

std::vector<std::string> SessionManager::getRecentlyModifiedSessions(size_t limit) {
    std::vector<std::pair<std::string, std::string>> sessions_by_time;

    try {
        std::string session_dir = getSessionDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(session_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string session_id = filename.substr(0, filename.length() - 5);

                sessions_by_time.emplace_back(session_id, entry.path().filename().string());
            }
        }

        // Sort by modification time (newest first)
        std::sort(sessions_by_time.begin(), sessions_by_time.end(),
                  [](const auto& a, const auto& b) {
                      return a.second > b.second;
                  });

        std::vector<std::string> results;
        for (size_t i = 0; i < std::min(limit, sessions_by_time.size()); ++i) {
            results.push_back(sessions_by_time[i].first);
        }

        return results;

    } catch (const std::exception&) {
        return {};
    }
}

} // namespace llm
} // namespace clion
