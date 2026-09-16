#include "backend/base.hpp"

#include <memory>

#include "backend/wlr_randr.hpp"

auto create_display_backend(BackendType type) -> std::unique_ptr<Backend> {
    switch (type) {
        case BackendType::WLR_RANDR:
            return std::make_unique<WlrRandrBackend>();
        case BackendType::KANSHI:
        case BackendType::AUTO_WLR_RANDR:
            return nullptr;
    }
    return nullptr;
}
