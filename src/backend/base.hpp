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

    // Changes awaiting confirmation or cancellation, tracked by this backend.
    // Profile edits and running operations are tracked separately by controllers.
    [[nodiscard]] virtual auto has_pending_changes() const -> bool = 0;

    [[nodiscard]] virtual auto can_confirm() const -> bool { return has_pending_changes(); }
};

class NullDisplayBackend : public Backend {
public:
    void apply(const std::vector<MonitorSpecs> & monitors) override;
    void revert() override;
    void confirm() override;
    [[nodiscard]] auto has_pending_changes() const -> bool override;

private:
    bool pending_confirmation_ = false;
};

auto create_display_backend(BackendType type) -> std::unique_ptr<Backend>;
