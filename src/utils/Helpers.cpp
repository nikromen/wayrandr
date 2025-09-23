#include "Helpers.hpp"
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <spdlog/spdlog.h>

std::string runCommand(const std::string &command) {
    spdlog::debug("Running command: {}", command);
    
    std::ostringstream oss;
    std::array<char, 4096> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        spdlog::critical("Failed to execute command: {}", command);
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        oss << buffer.data();
    }

    std::string output = oss.str();
    spdlog::debug("Command output: {}", output);
    return output;
}
