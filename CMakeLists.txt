cmake_minimum_required(VERSION 3.16)

project(wayrandr VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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

include(CTest)
if(BUILD_TESTING)
    add_executable(process_test
        tests/process_test.cpp src/utils/helpers.cpp
        src/models/monitor_block.cpp src/models/monitor_block.hpp)
    target_include_directories(process_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(process_test PRIVATE Qt6::Core spdlog::spdlog)
    target_compile_definitions(process_test PRIVATE
        FAKE_COMMAND="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake_command.py"
        FAKE_GRIM="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake_grim.py")
    add_test(NAME processes COMMAND process_test)
    set_tests_properties(processes PROPERTIES TIMEOUT 15)
    set(CONFIRMATION_TEST_SOURCES ${PROJECT_SOURCES})
    list(REMOVE_ITEM CONFIRMATION_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(wlr_confirmation_test tests/wlr_confirmation_test.cpp ${CONFIRMATION_TEST_SOURCES})
    target_include_directories(wlr_confirmation_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(wlr_confirmation_test PRIVATE
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
        nlohmann_json::nlohmann_json spdlog::spdlog tomlplusplus::tomlplusplus)
    target_compile_definitions(wlr_confirmation_test PRIVATE
        FAKE_WLR_CLI="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake_wlr_randr.py"
        FAKE_DAEMON="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake_daemon.py")
    add_test(NAME wlr_confirmation COMMAND wlr_confirmation_test)
    set_tests_properties(wlr_confirmation PROPERTIES TIMEOUT 30)
endif()
