#pragma once

#include <string>
#include <vector>

#include "backend/kanshi/types.hpp"
#include "backend/profile/types.hpp"

namespace kanshi_conversions {

[[nodiscard]] auto to_profile_output(const KanshiOutputSetting & setting)
    -> profile::ProfileOutputDefinition;
[[nodiscard]] auto from_profile_output(const profile::ProfileOutputDefinition & output)
    -> KanshiOutputSetting;

[[nodiscard]] auto to_profile_definition(const KanshiProfile & profile) -> profile::ProfileDefinition;
[[nodiscard]] auto from_profile_definition(const profile::ProfileDefinition & profile)
    -> KanshiProfile;

[[nodiscard]] auto to_profile_document(const KanshiConfig & config) -> profile::ProfileDocument;
[[nodiscard]] auto from_profile_document(const profile::ProfileDocument & document) -> KanshiConfig;

[[nodiscard]] auto to_connected_output(const KanshiConnectedOutputInfo & output)
    -> profile::ConnectedOutput;
[[nodiscard]] auto from_connected_output(const profile::ConnectedOutput & output)
    -> KanshiConnectedOutputInfo;

[[nodiscard]] auto to_service_status(const KanshiDaemonStatus & status)
    -> profile::ProfileServiceStatus;

[[nodiscard]] auto to_connected_outputs(const std::vector<KanshiConnectedOutputInfo> & outputs)
    -> std::vector<profile::ConnectedOutput>;
[[nodiscard]] auto from_connected_outputs(const std::vector<profile::ConnectedOutput> & outputs)
    -> std::vector<KanshiConnectedOutputInfo>;

}  // namespace kanshi_conversions
