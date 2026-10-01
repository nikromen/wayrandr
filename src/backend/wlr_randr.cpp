#include "backend/wlr_randr.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "monitor_specs.hpp"
#include "utils/helpers.hpp"

namespace {
auto cli_number(float value) -> std::string {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(6) << value;
    return stream.str();
}
}  // namespace

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
        args.push_back(
            std::to_string(mode.width) + "x" + std::to_string(mode.height) + "@" +
            cli_number(mode.refresh_rate) + "Hz"
        );

        const auto & pos = settings->get_position();
        args.push_back("--pos");
        args.push_back(std::to_string(pos.x) + "," + std::to_string(pos.y));

        args.push_back("--scale");
        args.push_back(cli_number(settings->get_scale()));

        args.push_back("--transform");
        args.push_back(transform_utils::to_string(settings->get_transform()));

        args.push_back("--adaptive-sync");
        args.push_back(settings->is_adaptive_sync() ? "enabled" : "disabled");
    }

    return args;
}

namespace {
void require_visible_output(const std::vector<MonitorSpecs> & monitors) {
    if (std::none_of(monitors.begin(), monitors.end(), [](const auto & monitor) {
            return monitor.is_enabled();
        })) {
        throw std::runtime_error("At least one output must remain enabled");
    }
}
}  // namespace

void WlrRandrBackend::apply(const std::vector<MonitorSpecs> & monitors) {
    if (has_pending_changes()) {
        throw std::logic_error("Resolve the pending configuration before applying again");
    }
    require_visible_output(monitors);
    const auto args = build_wlr_randr_args(monitors);
    auto snapshot = get_monitor_specs_list(true);
    require_visible_output(snapshot);
    // The tolerant reader may choose a preferred mode when no current mode exists.
    // Such a snapshot cannot faithfully restore the actual configuration.
    for (const auto & monitor : snapshot) {
        if (monitor.is_enabled()) {
            const auto & settings = monitor.get_enabled_monitor_settings();
            if (!settings || settings->get_active_mode_index() >= monitor.get_modes().size() ||
                !monitor.get_modes()[settings->get_active_mode_index()].is_current) {
                throw std::runtime_error("Cannot snapshot current mode for " + monitor.get_name());
            }
        }
    }
    (void)build_wlr_randr_args(snapshot);
    previous_config_.emplace(std::move(snapshot));
    apply_succeeded_ = false;
    try {
        execute(args);
        apply_succeeded_ = true;
    } catch (const std::exception & apply_error) {
        // A failing command may already have changed outputs. Attempt one recovery.
        const std::string message = apply_error.what();
        try {
            revert();
        } catch (const std::exception & rollback_error) {
            throw std::runtime_error(message + "; rollback failed: " + rollback_error.what());
        }
        throw std::runtime_error(message + "; previous configuration restored");
    }
}

void WlrRandrBackend::execute(const std::vector<std::string> & args) {
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
    if (!previous_config_) {
        return;
    }
    // Once recovery starts, only a successful rollback may release the snapshot.
    apply_succeeded_ = false;
    const auto connected = get_monitor_specs_list(true);
    std::vector<MonitorSpecs> saved;
    for (const auto & monitor : *previous_config_) {
        if (std::any_of(connected.begin(), connected.end(), [&](const auto & current) {
                return current.get_name() == monitor.get_name();
            })) {
            saved.push_back(monitor);
        }
    }
    // Never send an empty command or disable every surviving output. Keep the
    // original snapshot for a manual retry after the output is reconnected.
    require_visible_output(saved);
    execute(build_wlr_randr_args(saved));
    previous_config_.reset();
}

void WlrRandrBackend::confirm() {
    if (can_confirm()) {
        previous_config_.reset();
        apply_succeeded_ = false;
    }
}

auto WlrRandrBackend::has_pending_changes() const -> bool {
    return previous_config_.has_value();
}

auto WlrRandrBackend::can_confirm() const -> bool {
    return has_pending_changes() && apply_succeeded_;
}
