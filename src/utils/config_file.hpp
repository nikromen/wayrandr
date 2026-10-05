#pragma once

#include <filesystem>
#include <optional>
#include <string>

// The exact bytes and target loaded by an editor. An unset target permits only creation.
struct ConfigFileSnapshot {
    std::filesystem::path target;
    std::optional<std::string> content;
    std::string identity;
};

[[nodiscard]] auto read_config_file(const std::filesystem::path & path) -> ConfigFileSnapshot;

// Atomic replacement, without direct-write fallback or a power-loss durability guarantee.
// Locks cooperating writers and checks the loaded version again immediately before commit.
// Editors ignoring the lock can still race the final check/rename; there is no filesystem CAS.
[[nodiscard]] auto save_config_file(
    const std::filesystem::path & path,
    const std::string & content,
    const ConfigFileSnapshot & expected
) -> ConfigFileSnapshot;
