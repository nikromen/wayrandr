#pragma once

#include <filesystem>
#include <string>

#include "backend/auto_wlr_randr/types.hpp"
#include "utils/config_file.hpp"

class AutoWlrRandrConfigRepository {
public:
    explicit AutoWlrRandrConfigRepository(
        std::filesystem::path config_path = default_config_path()
    );

    [[nodiscard]] auto get_config_path() const -> const std::filesystem::path &;
    [[nodiscard]] auto load() const -> AutoWlrRandrConfig;
    [[nodiscard]] auto save(const AutoWlrRandrConfig & config) const -> ConfigFileSnapshot;

    [[nodiscard]] static auto default_config_path() -> std::filesystem::path;

    static void add_profile(AutoWlrRandrConfig & config, AutoWlrRandrProfile profile);
    static void delete_profile(AutoWlrRandrConfig & config, const std::string & profile_id);
    static void duplicate_profile(
        AutoWlrRandrConfig & config, const std::string & source_id, const std::string & new_id
    );

private:
    std::filesystem::path config_path_;
};
