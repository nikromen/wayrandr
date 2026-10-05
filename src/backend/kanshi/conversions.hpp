#pragma once


#include "backend/kanshi/types.hpp"
#include "backend/profile/types.hpp"

namespace kanshi_conversions {

[[nodiscard]] auto to_profile_output(const KanshiOutputSetting & setting)
    -> profile::ProfileOutputDefinition;
[[nodiscard]] auto from_profile_output(const profile::ProfileOutputDefinition & output)
    -> KanshiOutputSetting;

[[nodiscard]] auto to_profile_definition(const KanshiProfile & profile)
    -> profile::ProfileDefinition;
[[nodiscard]] auto from_profile_definition(const profile::ProfileDefinition & profile)
    -> KanshiProfile;

[[nodiscard]] auto to_profile_document(const KanshiConfig & config) -> profile::ProfileDocument;
[[nodiscard]] auto from_profile_document(const profile::ProfileDocument & document) -> KanshiConfig;

[[nodiscard]] auto to_service_status(const KanshiDaemonStatus & status)
    -> profile::ProfileServiceStatus;

}  // namespace kanshi_conversions
