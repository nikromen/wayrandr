#pragma once

#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

// TODO: not ready yet
class KanshiBackend : public Backend {
public:
    KanshiBackend();
    ~KanshiBackend() override = default;

    void apply(const std::vector<MonitorSpecs> & monitors) override;
    void revert() override;
    void confirm() override;
    [[nodiscard]] auto has_pending_changes() const -> bool override;

private:
    bool pending_confirmation_ = false;
};
