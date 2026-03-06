#include "backend/auto_wlr_randr/daemon_client.hpp"

#include <unistd.h>

#include <QStandardPaths>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#include "backend/auto_wlr_randr/types.hpp"
#include "nlohmann/json_fwd.hpp"
#include "utils/helpers.hpp"

namespace {

auto socket_path() -> std::filesystem::path {
    if (const char * runtime_dir = std::getenv("XDG_RUNTIME_DIR")) {
        return std::filesystem::path(runtime_dir) / "auto-wlr-randr" / "auto-wlr-randr.sock";
    }

    return std::filesystem::path("/run/user") / std::to_string(getuid()) / "auto-wlr-randr" /
        "auto-wlr-randr.sock";
}

}  // namespace

auto AutoWlrRandrDaemonClient::is_running() -> bool {
    return std::filesystem::exists(socket_path());
}

auto AutoWlrRandrDaemonClient::status() -> DaemonStatus {
    DaemonStatus result;
    result.daemon_running = is_running();
    if (!result.daemon_running) {
        return result;
    }

    try {
        const std::string output = run_command("auto-wlr-randrctl", { "status" });
        const auto json = nlohmann::json::parse(output);
        result.active_profile = json.value("active_profile", "None");
        if (json.contains("connected_outputs") && json["connected_outputs"].is_array()) {
            for (const auto & connected_output : json["connected_outputs"]) {
                result.connected_outputs.push_back(connected_output.get<std::string>());
            }
        }
    } catch (const std::exception & e) {
        throw std::runtime_error(std::string("Daemon status failed: ") + e.what());
    }

    return result;
}

void AutoWlrRandrDaemonClient::reload() {
    if (!is_running()) {
        throw std::runtime_error("auto-wlr-randr daemon is not running");
    }

    run_command("auto-wlr-randrctl", { "reload" });
}

void AutoWlrRandrDaemonClient::switch_profile(const std::string & profile_id, bool force) {
    if (!is_running()) {
        throw std::runtime_error("auto-wlr-randr daemon is not running");
    }

    if (force) {
        run_command("auto-wlr-randrctl", { "switch", profile_id, "--force" });
        return;
    }

    run_command("auto-wlr-randrctl", { "switch", profile_id });
}
