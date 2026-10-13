cmake_minimum_required(VERSION 3.16)
project(vds VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(VDS_BUILD_TESTS "Build the unit tests (no graphics needed)" ON)

# vds_core: vehicle model and logic. It must never link SDL.
add_library(vds_core STATIC
  src/core/Config.cpp
  src/core/VehicleState.cpp
  src/core/Warnings.cpp
  src/core/Simulation.cpp)
target_include_directories(vds_core PUBLIC src)
target_compile_options(vds_core PRIVATE -Wall -Wextra -Wpedantic)

if(VDS_BUILD_TESTS)
  enable_testing()
  add_executable(vds_tests
    tests/test_main.cpp
    tests/test_state.cpp
    tests/test_interface.cpp
    tests/test_config.cpp
    tests/test_warnings.cpp
    tests/test_policy.cpp)
  target_include_directories(vds_tests PRIVATE tests)
  target_link_libraries(vds_tests PRIVATE vds_core)
  target_compile_options(vds_tests PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_definitions(vds_tests PRIVATE VDS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_test(NAME vds_tests COMMAND vds_tests)
endif()

option(VDS_BUILD_APP "Build the SDL2 dashboard application" ON)

if(VDS_BUILD_APP)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2 SDL2_ttf)

  add_executable(vds
    src/app/main.cpp
    src/ui/Gfx.cpp
    src/ui/Input.cpp
    src/ui/Pages.cpp)
  target_link_libraries(vds PRIVATE vds_core PkgConfig::SDL2)
  target_compile_options(vds PRIVATE -Wall -Wextra -Wpedantic)

  # The executable looks for assets/ and config/ next to itself.
  add_custom_command(TARGET vds POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:vds>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:vds>/config)
endif()
