#pragma once

#include <stdexcept>
#include <string>

#include "backend/kanshi/types.hpp"

namespace kanshi_config_parser {

class ParseError final : public std::runtime_error {
public:
    explicit ParseError(const std::string & message) : std::runtime_error(message) {}
};

[[nodiscard]] auto parse(const std::string & content) -> KanshiConfig;
[[nodiscard]] auto serialize(const KanshiConfig & config) -> std::string;

}  // namespace kanshi_config_parser
