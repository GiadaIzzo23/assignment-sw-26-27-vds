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
  src/core/VehicleState.cpp)
target_include_directories(vds_core PUBLIC src)
target_compile_options(vds_core PRIVATE -Wall -Wextra -Wpedantic)

if(VDS_BUILD_TESTS)
  enable_testing()
  add_executable(vds_tests
    tests/test_main.cpp
    tests/test_state.cpp
    tests/test_interface.cpp)
  target_include_directories(vds_tests PRIVATE tests)
  target_link_libraries(vds_tests PRIVATE vds_core)
  target_compile_options(vds_tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vds_tests COMMAND vds_tests)
endif()
