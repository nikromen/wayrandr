#include "utils/helpers.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
constexpr size_t K_COMMAND_BUFFER_SIZE = 4096;
}

auto run_command(const std::string & command) -> std::string {
    spdlog::debug("Running command: {}", command);

    std::string output;
    std::array<char, K_COMMAND_BUFFER_SIZE> buffer{};

    using pclose_type = int (*)(FILE *);
    std::unique_ptr<FILE, pclose_type> const pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        spdlog::critical("Failed to execute command: {}", command);
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        output += buffer.data();
    }

    spdlog::debug("Command output: {}", output);
    return output;
}
