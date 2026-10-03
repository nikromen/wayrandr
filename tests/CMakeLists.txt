# Source registration is explicit; empty levels deliberately create no CTest entries.
add_library(wayrandr_test_support STATIC support/environment.cpp)
target_link_libraries(wayrandr_test_support PUBLIC wayrandr_lib Qt6::Test)
target_include_directories(wayrandr_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(wayrandr_test_support PUBLIC
    FAKE_COMMAND="${CMAKE_CURRENT_SOURCE_DIR}/support/fake_programs/fake_command.py"
    FAKE_GRIM="${CMAKE_CURRENT_SOURCE_DIR}/support/fake_programs/fake_grim.py"
    FAKE_WLR_CLI="${CMAKE_CURRENT_SOURCE_DIR}/support/fake_programs/fake_wlr_randr.py"
    FAKE_DAEMON="${CMAKE_CURRENT_SOURCE_DIR}/support/fake_programs/fake_daemon.py")
wayrandr_instrument(wayrandr_test_support)

function(wayrandr_add_cpp_test level name)
    add_executable(tst_${name} ${ARGN})
    target_link_libraries(tst_${name} PRIVATE wayrandr_test_support)
    wayrandr_instrument(tst_${name})
    add_test(NAME ${level}.${name} COMMAND tst_${name})
    set_tests_properties(${level}.${name} PROPERTIES LABELS ${level} TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QSG_RHI_BACKEND=software;ASAN_OPTIONS=halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    set_property(GLOBAL APPEND PROPERTY WAYRANDR_TEST_LEVELS ${level})
endfunction()

# System drivers receive the real application path and must install isolated fake tools.
function(wayrandr_add_system_test name)
    wayrandr_add_cpp_test(system ${name} ${ARGN})
    target_compile_definitions(tst_${name} PRIVATE WAYRANDR_APP="$<TARGET_FILE:wayrandr>")
    add_dependencies(tst_${name} wayrandr)
endfunction()

wayrandr_add_cpp_test(unit matching unit/matching.cpp)
wayrandr_add_cpp_test(unit layout unit/layout.cpp)

wayrandr_add_cpp_test(integration processes integration/processes.cpp)
wayrandr_add_cpp_test(integration command_security integration/command_security.cpp)
wayrandr_add_cpp_test(integration capture integration/capture.cpp)
wayrandr_add_cpp_test(integration shutdown integration/shutdown.cpp)
wayrandr_add_cpp_test(integration backend_transactions integration/backend_transactions.cpp)
wayrandr_add_cpp_test(integration main_window integration/main_window.cpp)
wayrandr_add_cpp_test(integration profile_lifecycle integration/profile_lifecycle.cpp)
wayrandr_add_cpp_test(integration profile_controller integration/profile_controller.cpp)

# Explicit compatibility check: real parsers, no compositor or daemon connection.
add_executable(tst_config_compatibility EXCLUDE_FROM_ALL integration/config_compatibility.cpp)
target_link_libraries(tst_config_compatibility PRIVATE wayrandr_test_support)
target_compile_definitions(tst_config_compatibility PRIVATE
    KANSHI_CORPUS="${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus/kanshi_config")

wayrandr_add_system_test(startup system/startup.cpp)

qt_add_resources(TEST_QML_RESOURCES ../qml/resource.qrc)
add_executable(wayrandr_qml_tests qml/runner.cpp ${TEST_QML_RESOURCES})
target_link_libraries(wayrandr_qml_tests PRIVATE wayrandr_test_support Qt6::QuickTest)
target_compile_definitions(wayrandr_qml_tests PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
wayrandr_instrument(wayrandr_qml_tests)
function(wayrandr_add_qml_test file)
    get_filename_component(name ${file} NAME_WE)
    add_test(NAME qml.${name} COMMAND wayrandr_qml_tests -input ${file})
    set_tests_properties(qml.${name} PROPERTIES LABELS qml TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QSG_RHI_BACKEND=software;ASAN_OPTIONS=halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endfunction()
file(GLOB_RECURSE qml_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/qml/tst_*.qml")
foreach(file IN LISTS qml_tests)
    wayrandr_add_qml_test(${file})
endforeach()
if(NOT qml_tests)
    message(STATUS "No QML scenarios: runner built, no QML test registered")
endif()
get_property(test_levels GLOBAL PROPERTY WAYRANDR_TEST_LEVELS)
foreach(level unit integration system)
    if(NOT level IN_LIST test_levels)
        message(STATUS "No ${level} scenarios registered")
    endif()
endforeach()

function(wayrandr_add_fuzz_target name)
    if(WAYRANDR_FUZZING)
        add_executable(${name} ${ARGN})
        target_link_libraries(${name} PRIVATE wayrandr_lib)
        wayrandr_instrument(${name})
        target_link_options(${name} PRIVATE -fsanitize=fuzzer)
        set_property(GLOBAL APPEND PROPERTY WAYRANDR_FUZZ_TARGETS ${name})
    endif()
endfunction()
wayrandr_add_fuzz_target(kanshi_config fuzz/kanshi_config.cpp)
if(WAYRANDR_FUZZING)
    get_property(fuzz_targets GLOBAL PROPERTY WAYRANDR_FUZZ_TARGETS)
    file(WRITE "${CMAKE_BINARY_DIR}/fuzz-targets.txt" "")
    foreach(target IN LISTS fuzz_targets)
        file(APPEND "${CMAKE_BINARY_DIR}/fuzz-targets.txt" "${target}\n")
    endforeach()
    if(NOT fuzz_targets)
        message(STATUS "No fuzz targets registered; instrumentation configuration only")
    endif()
endif()
