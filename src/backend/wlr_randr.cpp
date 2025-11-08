#include "backend/wlr_randr.hpp"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "monitor_specs.hpp"
#include "nlohmann/json_fwd.hpp"
#include "utils/helpers.hpp"

using json = nlohmann::json;

auto WlrRandrBackend::build_wlr_randr_command(const std::vector<MonitorSpecs> & monitors)
    -> std::string {
    std::string cmd = "wlr-randr";

    for (const auto & monitor : monitors) {
        cmd += fmt::format(" --output {}", monitor.get_name());

        if (monitor.is_enabled()) {
            cmd += " --on";
        } else {
            cmd += " --off";
            continue;
        }

        const auto & settings = const_cast<MonitorSpecs &>(monitor).get_enabled_monitor_settings();
        if (!settings.has_value()) {
            throw std::logic_error(
                fmt::format(
                    "Enabled monitor settings missing for enabled monitor: {} this is probably a "
                    "bug",
                    monitor.get_name()
                )
            );
        }

        const auto & modes = monitor.get_modes();
        size_t const active_mode_index = settings->get_active_mode_index();
        const auto & mode = modes[active_mode_index];
        std::string const mode_str = mode.to_string();
        cmd += fmt::format(" --mode {}", mode_str);

        const auto & pos = settings->get_position();
        cmd += fmt::format(" --pos {},{}", pos.x, pos.y);

        cmd += fmt::format(" --scale {}", settings->get_scale());

        const auto & transform = settings->get_transform();
        cmd += fmt::format(" --transform {}", transform_utils::to_string(transform));

        if (settings->is_adaptive_sync()) {
            cmd += " --adaptive-sync enabled";
        } else {
            cmd += " --adaptive-sync disabled";
        }
    }

    return cmd;
}

void WlrRandrBackend::apply(const std::vector<MonitorSpecs> & monitors) {
    spdlog::info("Applying wlr-randr configuration");

    previous_config_ = get_monitor_specs_list();

    run_command(build_wlr_randr_command(monitors));
    spdlog::info("Configuration applied successfully");
}

void WlrRandrBackend::revert() {
    if (previous_config_.empty()) {
        spdlog::warn("No previous configuration to revert to");
        return;
    }

    spdlog::info("Reverting to previous configuration");
    apply(previous_config_);
}
