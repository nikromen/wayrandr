#pragma once

#include <qtmetamacros.h>

#include <QMetaObject>
#include <memory>
#include <vector>

#include "../monitor_specs.hpp"

// Editor identity, independent of whether the mode provides a display transaction.
namespace editor_mode {
Q_NAMESPACE

enum Mode : int { WLR_RANDR, KANSHI, AUTO_WLR_RANDR };
Q_ENUM_NS(Mode)
}  // namespace editor_mode

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

// Profile modes have no display backend; unknown mode values throw.
auto create_display_backend(editor_mode::Mode type) -> std::unique_ptr<Backend>;
