#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../monitor_specs.hpp"

enum class BackendType { WLR_RANDR, KANSHI, AUTO_WLR_RANDR };

struct Profile {
    std::string name;
    std::vector<MonitorSpecs> monitors;
};

class Backend {
public:
    virtual ~Backend() = default;

    virtual void apply(const std::vector<MonitorSpecs> & monitors) = 0;
    virtual void revert() = 0;
};

class ProfileBackend : public Backend {
public:
    ~ProfileBackend() override = default;

    virtual void save(const Profile & profile) {}

    virtual void delete_profile(const std::string & name) {}

private:
    std::string config_path_;
    std::string previous_config_copy_path_;
    std::vector<Profile> profiles_;

    [[nodiscard]] virtual auto get_config_path() const -> std::string;
};

auto create_backend(BackendType type) -> std::unique_ptr<Backend>;
