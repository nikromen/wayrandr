#pragma once

#include <filesystem>
#include <string>

#include "backend/kanshi/types.hpp"

class KanshiConfigRepository {
public:
    explicit KanshiConfigRepository(std::filesystem::path config_path = default_config_path());

    [[nodiscard]] static auto default_config_path() -> std::filesystem::path;
    [[nodiscard]] auto get_config_path() const -> const std::filesystem::path &;

    [[nodiscard]] auto load() const -> KanshiConfig;
    void save(const KanshiConfig & config) const;

    static void add_profile(KanshiConfig & config, KanshiProfile profile);
    static void delete_profile(KanshiConfig & config, const std::string & profile_id);
    static void duplicate_profile(
        KanshiConfig & config, const std::string & source_id, const std::string & new_id
    );

private:
    std::filesystem::path config_path_;
};
