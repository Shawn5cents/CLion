#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "clion/common.h"

namespace clion {
namespace llm {

struct HistoryEntry {
    std::string role;
    std::string content;
    std::string timestamp;
};

struct Session {
    std::string id;
    std::vector<HistoryEntry> entries;
    std::string created_at;
    std::string updated_at;

    // Enhanced session management fields
    std::string name;                          ///< Human-readable session name
    std::string description;                   ///< Optional session description
    std::unordered_set<std::string> tags;      ///< Tags for organization and search
    std::string parent_session_id;             ///< Parent session for hierarchy
    std::vector<std::string> child_session_ids; ///< Child sessions
    std::unordered_map<std::string, std::string> metadata; ///< Additional metadata
    std::vector<std::string> checkpoint_ids;   ///< Associated checkpoint IDs
    std::vector<std::string> memory_node_ids;  ///< Associated memory node IDs
    size_t total_tokens;                       ///< Total token count for the session
    bool is_compressed;                        ///< Whether session data is compressed
    std::string last_checkpoint_id;            ///< Most recent checkpoint ID
};

class SessionManager {
public:
    // Core session operations
    static bool saveSession(const Session& session);
    static std::optional<Session> loadSession(const std::string& session_id);
    static std::string createSessionId();

    // Session management utilities
    static std::string createNewSession();
    static bool addEntryToSession(const std::string& session_id,
                                  const std::string& role,
                                  const std::string& content);
    static std::vector<std::string> listSessions();
    static bool deleteSession(const std::string& session_id);
    static bool sessionExists(const std::string& session_id);

    // Enhanced session management features
    static std::string createNewSessionWithMetadata(const std::string& name,
                                                   const std::string& description = "",
                                                   const std::unordered_set<std::string>& tags = {},
                                                   const std::string& parent_id = "");

    static bool updateSessionMetadata(const std::string& session_id,
                                    const std::string& name = "",
                                    const std::string& description = "",
                                    const std::unordered_set<std::string>& tags = {});

    static bool addTagsToSession(const std::string& session_id,
                               const std::unordered_set<std::string>& tags);

    static bool removeTagsFromSession(const std::string& session_id,
                                    const std::unordered_set<std::string>& tags);

    static std::optional<Session> getSessionWithMetadata(const std::string& session_id);

    static std::vector<std::string> findSessionsByTag(const std::string& tag);

    static std::vector<std::string> findSessionsByName(const std::string& name_pattern);

    static std::vector<std::string> findSessionsByContent(const std::string& content_pattern);

    static std::vector<std::string> getChildSessions(const std::string& parent_id);

    static std::vector<std::string> getSessionHierarchy(const std::string& session_id);

    static bool setParentSession(const std::string& session_id, const std::string& parent_id);

    static bool addChildSession(const std::string& parent_id, const std::string& child_id);

    static bool removeChildSession(const std::string& parent_id, const std::string& child_id);

    // Checkpoint integration
    static std::string createCheckpoint(const std::string& session_id,
                                       const std::string& checkpoint_name,
                                       const std::string& description = "");

    static std::optional<Session> restoreFromCheckpoint(const std::string& checkpoint_id);

    static std::vector<std::string> getSessionCheckpoints(const std::string& session_id);

    static bool deleteSessionCheckpoints(const std::string& session_id);

    // Memory integration
    static std::string createMemoryFromSession(const std::string& session_id,
                                             const std::string& memory_name,
                                             const std::string& parent_memory_id = "");

    static bool associateMemoryWithSession(const std::string& session_id,
                                         const std::string& memory_node_id);

    static std::vector<std::string> getSessionMemoryNodes(const std::string& session_id);

    // Session compression and optimization
    static bool compressSession(const std::string& session_id);

    static bool decompressSession(const std::string& session_id);

    static size_t getSessionSize(const std::string& session_id);

    static size_t getSessionTokenCount(const std::string& session_id);

    // Session cleanup and maintenance
    static size_t cleanupOldSessions(size_t max_age_days = 30);

    static std::unordered_map<std::string, std::string> getSessionStats();

    static bool validateSessionIntegrity(const std::string& session_id);

    // Session search and filtering
    static std::vector<std::string> searchSessions(const std::string& query,
                                                 const std::unordered_set<std::string>& tags = {},
                                                 const std::string& date_from = "",
                                                 const std::string& date_to = "");

    static std::vector<std::string> getSessionsByDateRange(const std::string& from_date,
                                                          const std::string& to_date);

    static std::vector<std::string> getSessionsBySize(size_t min_size = 0,
                                                    size_t max_size = 0);

    static std::vector<std::string> getRecentlyModifiedSessions(size_t limit = 10);
};

} // namespace llm
} // namespace clion
