#include "backend/base.hpp"

#include <memory>
#include <string>

#include "backend/auto_wlr_randr.hpp"
#include "backend/kanshi.hpp"
#include "backend/wlr_randr.hpp"

auto create_backend(BackendType type) -> std::unique_ptr<Backend> {
    switch (type) {
        case BackendType::WLR_RANDR:
            return std::make_unique<WlrRandrBackend>();
        case BackendType::KANSHI:
            return std::make_unique<KanshiBackend>();
        case BackendType::AUTO_WLR_RANDR:
            return std::make_unique<AutoWlrRandrBackend>();
    }
    return nullptr;
}

// TODO: remove this once implemented properly
auto ProfileBackend::get_config_path() const -> std::string {
    return "";
}
