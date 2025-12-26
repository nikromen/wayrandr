#include "backend/wlr_randr.hpp"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "monitor_specs.hpp"
#include "utils/helpers.hpp"

auto WlrRandrBackend::build_wlr_randr_args(const std::vector<MonitorSpecs> & monitors)
    -> std::vector<std::string> {
    std::vector<std::string> args;

    for (const auto & monitor : monitors) {
        args.push_back("--output");
        args.push_back(monitor.get_name());

        if (!monitor.is_enabled()) {
            args.push_back("--off");
            continue;
        }

        args.push_back("--on");

        const auto & settings = monitor.get_enabled_monitor_settings();
        if (!settings.has_value()) {
            throw std::logic_error(
                "Enabled monitor settings missing for enabled monitor: " + monitor.get_name()
            );
        }

        const auto & modes = monitor.get_modes();
        const size_t active_mode_index = settings->get_active_mode_index();
        if (active_mode_index >= modes.size()) {
            throw std::out_of_range(
                "Active mode index out of range for monitor: " + monitor.get_name()
            );
        }

        const auto & mode = modes[active_mode_index];
        args.push_back("--mode");
        args.push_back(mode.to_wlr_randr_arg());

        const auto & pos = settings->get_position();
        args.push_back("--pos");
        args.push_back(std::to_string(pos.x) + "," + std::to_string(pos.y));

        args.push_back("--scale");
        args.push_back(std::to_string(settings->get_scale()));

        args.push_back("--transform");
        args.push_back(transform_utils::to_string(settings->get_transform()));

        args.push_back("--adaptive-sync");
        args.push_back(settings->is_adaptive_sync() ? "enabled" : "disabled");
    }

    return args;
}

void WlrRandrBackend::apply(const std::vector<MonitorSpecs> & monitors) {
    apply(monitors, true);
}

void WlrRandrBackend::apply(const std::vector<MonitorSpecs> & monitors, bool snapshot_current) {
    spdlog::info("Applying wlr-randr configuration");

    if (snapshot_current) {
        previous_config_ = get_monitor_specs_list();
    }

    const auto args = build_wlr_randr_args(monitors);
    spdlog::debug("wlr-randr argument count: {}", args.size());
#ifdef ENABLE_DEBUG_LOGS
    {
        std::ostringstream cmd_log;
        cmd_log << "wlr-randr";
        for (const auto & arg : args) {
            cmd_log << ' ' << arg;
        }
        spdlog::debug("wlr-randr command: {}", cmd_log.str());
    }
#endif

    run_command("wlr-randr", args);
    spdlog::info("Configuration applied successfully");
}

void WlrRandrBackend::revert() {
    if (previous_config_.empty()) {
        spdlog::warn("No previous configuration to revert to");
        return;
    }

    spdlog::info("Reverting to previous configuration ({} monitors)", previous_config_.size());
    const auto saved = previous_config_;
    apply(saved, false);
}
