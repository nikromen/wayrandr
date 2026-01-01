#pragma once

#include <string>

#include "backend/auto_wlr_randr/types.hpp"

class AutoWlrRandrDaemonClient {
public:
    [[nodiscard]] static auto is_running() -> bool;
    [[nodiscard]] static auto status() -> DaemonStatus;
    static void reload();
    static void switch_profile(const std::string & profile_id, bool force = false);
};
