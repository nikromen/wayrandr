#pragma once

#include <filesystem>
#include <string>

#include "backend/auto_wlr_randr/types.hpp"

class AutoWlrRandrConfigRepository {
public:
    explicit AutoWlrRandrConfigRepository(
        std::filesystem::path config_path = default_config_path()
    );

    [[nodiscard]] auto get_config_path() const -> const std::filesystem::path &;
    [[nodiscard]] auto load() const -> AutoWlrRandrConfig;
    void save(const AutoWlrRandrConfig & config) const;

    [[nodiscard]] static auto default_config_path() -> std::filesystem::path;

    static void add_profile(AutoWlrRandrConfig & config, AutoWlrRandrProfile profile);
    static void delete_profile(AutoWlrRandrConfig & config, const std::string & profile_id);
    static void duplicate_profile(
        AutoWlrRandrConfig & config, const std::string & source_id, const std::string & new_id
    );

private:
    std::filesystem::path config_path_;
};
