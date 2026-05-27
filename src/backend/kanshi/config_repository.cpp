#include "backend/kanshi/config_repository.hpp"

#include <qhashfunctions.h>
#include <spdlog/spdlog.h>

#include <QStandardPaths>
#include <filesystem>
#include <string>
#include <utility>

#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/types.hpp"
#include "utils/config_file.hpp"

KanshiConfigRepository::KanshiConfigRepository(std::filesystem::path config_path)
    : config_path_(std::move(config_path)) {}

auto KanshiConfigRepository::default_config_path() -> std::filesystem::path {
    const QString config_home = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return std::filesystem::path(config_home.toStdString()) / "kanshi" / "config";
}

auto KanshiConfigRepository::get_config_path() const -> const std::filesystem::path & {
    return config_path_;
}

auto KanshiConfigRepository::load() const -> KanshiConfig {
    KanshiConfig config;
    config.path = config_path_.string();

    const auto file = read_config_file(config_path_);
    config.file_snapshot = file;
    if (!file.content.has_value()) {
        spdlog::info("kanshi config not found at {}, using empty config", config.path);
        return config;
    }

    config = kanshi_config_parser::parse(*file.content);
    config.path = config_path_.string();
    config.file_snapshot = file;

    spdlog::info("Loaded {} kanshi profiles from {}", config.profiles.size(), config.path);
    return config;
}

auto KanshiConfigRepository::save(const KanshiConfig & config) const -> ConfigFileSnapshot {
    // Validate and serialize before touching the destination.
    const auto content = kanshi_config_parser::serialize(config);
    const auto saved = save_config_file(config_path_, content, config.file_snapshot);
    spdlog::info("Saved {} kanshi profiles to {}", config.profiles.size(), config_path_.string());
    return saved;
}
