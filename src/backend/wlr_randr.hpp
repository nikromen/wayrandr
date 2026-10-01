#pragma once

#include <optional>
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
    void confirm() override;
    [[nodiscard]] auto has_pending_changes() const -> bool override;
    [[nodiscard]] auto can_confirm() const -> bool override;

private:
    std::optional<std::vector<MonitorSpecs>> previous_config_;
    bool apply_succeeded_ = false;

    static void execute(const std::vector<std::string> & args);

    static auto build_wlr_randr_args(const std::vector<MonitorSpecs> & monitors)
        -> std::vector<std::string>;
};
