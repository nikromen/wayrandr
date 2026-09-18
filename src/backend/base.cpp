#include "backend/base.hpp"

#include <memory>
#include <stdexcept>

#include "backend/wlr_randr.hpp"

auto create_display_backend(editor_mode::Mode type) -> std::unique_ptr<Backend> {
    switch (type) {
        case editor_mode::WLR_RANDR:
            return std::make_unique<WlrRandrBackend>();
        case editor_mode::KANSHI:
        case editor_mode::AUTO_WLR_RANDR:
            return nullptr;
    }
    throw std::invalid_argument("Unknown editor mode");
}
