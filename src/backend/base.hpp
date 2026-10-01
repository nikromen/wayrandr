#pragma once

#include <memory>
#include <vector>

#include "../monitor_specs.hpp"

enum class BackendType { WLR_RANDR, KANSHI, AUTO_WLR_RANDR };

class Backend {
public:
    virtual ~Backend() = default;

    virtual void apply(const std::vector<MonitorSpecs> & monitors) = 0;
    virtual void revert() = 0;

    virtual void confirm() {}

    [[nodiscard]] virtual auto has_pending_changes() const -> bool { return false; }

    [[nodiscard]] virtual auto can_confirm() const -> bool { return false; }
};

class NullDisplayBackend : public Backend {
public:
    void apply(const std::vector<MonitorSpecs> & monitors) override;
    void revert() override;
};

auto create_display_backend(BackendType type) -> std::unique_ptr<Backend>;
