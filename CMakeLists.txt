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
find_package(PkgConfig REQUIRED)
pkg_check_modules(SCFG REQUIRED IMPORTED_TARGET scfg>=0.2.0)

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

include(cmake/Instrumentation.cmake)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
list(REMOVE_ITEM PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_library(wayrandr_lib STATIC ${PROJECT_SOURCES})
target_link_libraries(wayrandr_lib PUBLIC
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    nlohmann_json::nlohmann_json spdlog::spdlog tomlplusplus::tomlplusplus)
target_link_libraries(wayrandr_lib PRIVATE PkgConfig::SCFG)
target_include_directories(wayrandr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(ENABLE_DEBUG_LOGS)
    target_compile_definitions(wayrandr_lib PUBLIC ENABLE_DEBUG_LOGS)
endif()
wayrandr_instrument(wayrandr_lib)

# Resources stay on executables so the static linker cannot discard their initializer.
qt_add_resources(QML_RESOURCES qml/resource.qrc)
add_executable(wayrandr src/main.cpp ${QML_RESOURCES})
target_link_libraries(wayrandr PRIVATE wayrandr_lib)
wayrandr_instrument(wayrandr)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test QuickTest)
    add_subdirectory(tests)
endif()
if(WAYRANDR_FUZZING AND NOT BUILD_TESTING)
    message(FATAL_ERROR "Fuzz registration requires BUILD_TESTING=ON")
endif()
