#include "backend/profile/types.hpp"

#include <optional>
#include <string>

namespace profile {

auto ConnectedOutput::build_identifier() const -> std::optional<std::string> {
    if (!make.has_value() || !model.has_value()) {
        return std::nullopt;
    }

    if (serial.has_value() && !serial->empty()) {
        return make.value() + " " + model.value() + " " + serial.value();
    }

    return make.value() + " " + model.value();
}

auto ConnectedOutput::display_label() const -> std::string {
    const auto identifier = build_identifier();
    if (identifier.has_value()) {
        return name + " (" + identifier.value() + ")";
    }
    return name;
}

}  // namespace profile
