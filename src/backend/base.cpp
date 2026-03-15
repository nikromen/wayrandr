#include "backend/base.hpp"

#include <memory>
#include <vector>

#include "backend/kanshi.hpp"
#include "backend/wlr_randr.hpp"
#include "monitor_specs.hpp"

void NullDisplayBackend::apply(const std::vector<MonitorSpecs> & /*monitors*/) {
    pending_confirmation_ = true;
}

void NullDisplayBackend::revert() {
    pending_confirmation_ = false;
}

void NullDisplayBackend::confirm() {
    pending_confirmation_ = false;
}

auto NullDisplayBackend::has_pending_changes() const -> bool {
    return pending_confirmation_;
}

auto create_display_backend(BackendType type) -> std::unique_ptr<Backend> {
    switch (type) {
        case BackendType::WLR_RANDR:
            return std::make_unique<WlrRandrBackend>();
        case BackendType::KANSHI:
            return std::make_unique<KanshiBackend>();
        case BackendType::AUTO_WLR_RANDR:
            return std::make_unique<NullDisplayBackend>();
    }
    return nullptr;
}
