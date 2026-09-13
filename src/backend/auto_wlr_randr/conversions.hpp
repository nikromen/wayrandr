#pragma once

#include <vector>

#include "backend/auto_wlr_randr/types.hpp"
#include "backend/profile/types.hpp"

namespace auto_wlr_randr_conversions {

[[nodiscard]] auto to_profile_output(const ProfileOutputSetting & setting)
    -> profile::ProfileOutputDefinition;
[[nodiscard]] auto from_profile_output(const profile::ProfileOutputDefinition & output)
    -> ProfileOutputSetting;

[[nodiscard]] auto to_profile_definition(const AutoWlrRandrProfile & profile)
    -> profile::ProfileDefinition;
[[nodiscard]] auto from_profile_definition(const profile::ProfileDefinition & profile)
    -> AutoWlrRandrProfile;

[[nodiscard]] auto to_profile_document(const AutoWlrRandrConfig & config)
    -> profile::ProfileDocument;
[[nodiscard]] auto from_profile_document(const profile::ProfileDocument & document)
    -> AutoWlrRandrConfig;

[[nodiscard]] auto to_connected_output(const ConnectedOutputInfo & output)
    -> profile::ConnectedOutput;
[[nodiscard]] auto from_connected_output(const profile::ConnectedOutput & output)
    -> ConnectedOutputInfo;

[[nodiscard]] auto to_service_status(const DaemonStatus & status) -> profile::ProfileServiceStatus;
[[nodiscard]] auto to_connected_outputs(const std::vector<ConnectedOutputInfo> & outputs)
    -> std::vector<profile::ConnectedOutput>;
[[nodiscard]] auto from_connected_outputs(const std::vector<profile::ConnectedOutput> & outputs)
    -> std::vector<ConnectedOutputInfo>;

}  // namespace auto_wlr_randr_conversions
