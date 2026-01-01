#pragma once

#include <string>

#include "backend/auto_wlr_randr/types.hpp"

namespace auto_wlr_randr_snapshot {
[[nodiscard]] auto create_profile_from_live_state(const std::string & profile_id)
    -> AutoWlrRandrProfile;
}
