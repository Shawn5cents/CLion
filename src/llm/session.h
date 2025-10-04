#pragma once

#include <string>
#include <vector>
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
};

class SessionManager {
public:
    static bool saveSession(const Session& session);
    static std::optional<Session> loadSession(const std::string& session_id);
    static std::string createSessionId();
};

} // namespace llm
} // namespace clion
