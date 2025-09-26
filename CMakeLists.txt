cmake_minimum_required(VERSION 3.16)

project(wayrandr VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Option to enable/disable debug logs
option(ENABLE_DEBUG_LOGS "Enable debug logging" ON)

# Find the required Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

# Include nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3  # Use the latest version or specify a specific version
)

# Include spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3  # Use the latest version or specify a specific version
)

FetchContent_MakeAvailable(json spdlog)

# Use the QML resource file
qt_add_resources(QML_RESOURCES qml/resource.qrc)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Automatically find all source files
file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
    "src/*.c"
    "src/*.hpp"
    "src/*.h"
)

# Add the executable
add_executable(wayrandr 
    ${PROJECT_SOURCES}
    ${QML_RESOURCES}
)

# Link Qt libraries
target_link_libraries(wayrandr PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Configure debug logging
if(ENABLE_DEBUG_LOGS)
    target_compile_definitions(wayrandr PRIVATE ENABLE_DEBUG_LOGS)
endif()

