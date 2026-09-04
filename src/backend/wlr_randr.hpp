#pragma once

#include <string>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

class WlrRandrBackend : public Backend {
public:
    WlrRandrBackend() = default;
    ~WlrRandrBackend() override = default;

    void apply(const std::vector<MonitorSpecs> & monitors) override;
    void revert() override;

private:
    std::vector<MonitorSpecs> previous_config_;

    void apply(const std::vector<MonitorSpecs> & monitors, bool snapshot_current);

    static auto build_wlr_randr_args(const std::vector<MonitorSpecs> & monitors)
        -> std::vector<std::string>;
};
