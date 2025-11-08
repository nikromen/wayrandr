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

    static auto build_wlr_randr_command(const std::vector<MonitorSpecs> & monitors) -> std::string;
};
