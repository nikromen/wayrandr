#include "Helpers.hpp"
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <sstream>

std::string runCommand(const std::string &command) {
    std::ostringstream oss;
    std::array<char, 4096> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        oss << buffer.data();
    }

    return oss.str();
}
