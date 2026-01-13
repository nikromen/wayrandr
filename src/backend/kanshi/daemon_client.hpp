#pragma once

#include <string>

#include "backend/kanshi/types.hpp"

class KanshiDaemonClient {
public:
    [[nodiscard]] static auto is_running() -> bool;
    [[nodiscard]] static auto status() -> KanshiDaemonStatus;
    static void reload();
    static void switch_profile(const std::string & profile_id);
};
