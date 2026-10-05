#include "backend/kanshi/config_repository.hpp"

#include <qhashfunctions.h>
#include <spdlog/spdlog.h>

#include <QStandardPaths>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
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

void KanshiConfigRepository::add_profile(KanshiConfig & config, KanshiProfile profile) {
    const auto it = std::find_if(
        config.profiles.begin(), config.profiles.end(), [&](const KanshiProfile & existing) {
            return existing.id == profile.id;
        }
    );
    if (it != config.profiles.end()) {
        *it = std::move(profile);
        return;
    }

    config.profiles.push_back(std::move(profile));
}

void KanshiConfigRepository::delete_profile(KanshiConfig & config, const std::string & profile_id) {
    config.profiles.erase(
        std::remove_if(
            config.profiles.begin(),
            config.profiles.end(),
            [&](const KanshiProfile & profile) { return profile.id == profile_id; }
        ),
        config.profiles.end()
    );
}

void KanshiConfigRepository::duplicate_profile(
    KanshiConfig & config, const std::string & source_id, const std::string & new_id
) {
    const auto it = std::find_if(
        config.profiles.begin(), config.profiles.end(), [&](const KanshiProfile & profile) {
            return profile.id == source_id;
        }
    );
    if (it == config.profiles.end()) {
        throw std::invalid_argument("Source profile not found: " + source_id);
    }

    KanshiProfile copy = *it;
    copy.id = new_id;
    copy.anonymous = false;
    add_profile(config, std::move(copy));
}
