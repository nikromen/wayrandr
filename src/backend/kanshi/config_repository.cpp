#include "backend/kanshi/config_repository.hpp"

#include <qhashfunctions.h>
#include <spdlog/spdlog.h>

#include <QStandardPaths>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/types.hpp"

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

    if (!std::filesystem::exists(config_path_)) {
        spdlog::info("kanshi config not found at {}, using empty config", config.path);
        return config;
    }

    std::ifstream input(config_path_);
    if (!input) {
        throw std::runtime_error("Failed to open kanshi config: " + config.path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    config = kanshi_config_parser::parse(buffer.str());
    config.path = config_path_.string();

    spdlog::info("Loaded {} kanshi profiles from {}", config.profiles.size(), config.path);
    return config;
}

void KanshiConfigRepository::save(const KanshiConfig & config) const {
    // Validate before opening/truncating the destination.
    const auto content = kanshi_config_parser::serialize(config);
    if (const auto parent = config_path_.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(config_path_);
    if (!output) {
        throw std::runtime_error("Failed to open kanshi config for writing: " + config.path);
    }

    output << content;
    spdlog::info("Saved {} kanshi profiles to {}", config.profiles.size(), config.path);
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
    add_profile(config, std::move(copy));
}
