#pragma once

#include <filesystem>

#include "backend/kanshi/types.hpp"
#include "utils/config_file.hpp"

class KanshiConfigRepository {
public:
    explicit KanshiConfigRepository(std::filesystem::path config_path = default_config_path());

    [[nodiscard]] static auto default_config_path() -> std::filesystem::path;
    [[nodiscard]] auto get_config_path() const -> const std::filesystem::path &;

    [[nodiscard]] auto load() const -> KanshiConfig;
    [[nodiscard]] auto save(const KanshiConfig & config) const -> ConfigFileSnapshot;

private:
    std::filesystem::path config_path_;
};
