#pragma once

#include <string>

#include "backend/kanshi/types.hpp"

namespace kanshi_snapshot {

[[nodiscard]] auto create_profile_from_live_state(const std::string & profile_id) -> KanshiProfile;

}  // namespace kanshi_snapshot
