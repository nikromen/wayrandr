cmake_minimum_required(VERSION 3.16)

project(wayrandr VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Option to enable/disable debug logs
option(ENABLE_DEBUG_LOGS "Enable debug logging" ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

# Fetch dependencies
include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(json spdlog tomlplusplus)

# Source files
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp" "src/*.hpp")

# QML resources
qt_add_resources(QML_RESOURCES qml/resource.qrc)

# Create executable
add_executable(wayrandr ${PROJECT_SOURCES} ${QML_RESOURCES})

# Link libraries
target_link_libraries(wayrandr PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

# Include directories
target_include_directories(wayrandr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Configure debug logging
if(ENABLE_DEBUG_LOGS)
    target_compile_definitions(wayrandr PRIVATE ENABLE_DEBUG_LOGS)
endif()
