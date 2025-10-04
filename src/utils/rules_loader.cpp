#include "rules_loader.h"
#include "clion/common.h"

namespace clion {
namespace utils {

std::optional<CLionConfig> RulesLoader::loadConfig(const std::filesystem::path& config_path) {
    // TODO: Implement in Phase 5.2
    return getDefaultConfig();
}

bool RulesLoader::saveConfig(const CLionConfig& config, const std::filesystem::path& config_path) {
    // TODO: Implement in Phase 5.2
    return false;
}

CLionConfig RulesLoader::getDefaultConfig() {
    // TODO: Implement in Phase 5.2
    return CLionConfig{};
}

std::optional<std::filesystem::path> RulesLoader::findConfigFile(const std::filesystem::path& project_root) {
    // TODO: Implement in Phase 5.2
    return project_root / ".clionrules.yaml";
}

} // namespace utils
} // namespace clion
