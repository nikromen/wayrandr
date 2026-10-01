#include "backend/kanshi/daemon_client.hpp"

#include <spdlog/spdlog.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>

#include "monitor_specs.hpp"
#include "utils/helpers.hpp"

namespace {

auto socket_path() -> std::filesystem::path {
    const char * runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    const char * wayland_display = std::getenv("WAYLAND_DISPLAY");
    if (runtime_dir == nullptr || wayland_display == nullptr) {
        return {};
    }

    return std::filesystem::path(runtime_dir) / ("fr.emersion.kanshi." + std::string(wayland_display));
}

auto trim(const std::string & value) -> std::string {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

}  // namespace

auto KanshiDaemonClient::is_running() -> bool {
    const auto path = socket_path();
    return !path.empty() && std::filesystem::exists(path);
}

auto KanshiDaemonClient::status() -> KanshiDaemonStatus {
    KanshiDaemonStatus result;
    result.daemon_running = is_running();
    if (!result.daemon_running) {
        return result;
    }

    try {
        const std::string output = run_command("kanshictl", { "status" });
        std::istringstream stream(output);
        std::string line;
        while (std::getline(stream, line)) {
            constexpr std::string_view prefix = "Current profile:";
            if (line.rfind(prefix.data(), 0) == 0) {
                result.active_profile = trim(line.substr(prefix.size()));
            }
        }
    } catch (const std::exception & e) {
        spdlog::warn("Failed to parse kanshictl status: {}", e.what());
    }

    for (const auto & monitor : get_monitor_specs_list()) {
        result.connected_outputs.push_back(monitor.get_name());
    }

    return result;
}

void KanshiDaemonClient::reload() {
    if (!is_running()) {
        throw std::runtime_error("kanshi daemon is not running");
    }

    run_command("kanshictl", { "reload" });
}

void KanshiDaemonClient::switch_profile(const std::string & profile_id) {
    if (!is_running()) {
        throw std::runtime_error("kanshi daemon is not running");
    }

    run_command("kanshictl", { "switch", profile_id });
}
