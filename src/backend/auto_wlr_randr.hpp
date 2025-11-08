#pragma once

#include <string>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

struct WlrRandrProfile : public Profile {
    std::vector<std::string> exec;
};

class AutoWlrRandrBackend : public ProfileBackend {
public:
    AutoWlrRandrBackend();
    ~AutoWlrRandrBackend() override = default;

    void apply(const std::vector<MonitorSpecs> & monitors) override;
    void revert() override;

    void save(const Profile & profile) override;
    void delete_profile(const std::string & name) override;

private:
    std::string config_path_;
    std::string previous_config_copy_path_;
    std::vector<Profile> profiles_;
    // toml::table config_data_;

    [[nodiscard]] auto get_config_path() const -> std::string override;
    static auto load_profiles() -> void;
};
