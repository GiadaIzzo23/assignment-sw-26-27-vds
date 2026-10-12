cmake_minimum_required(VERSION 3.16)
project(vds VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(VDS_BUILD_TESTS "Build the unit tests (no graphics needed)" ON)

if(VDS_BUILD_TESTS)
  enable_testing()
  add_executable(vds_tests tests/test_main.cpp)
  target_include_directories(vds_tests PRIVATE tests)
  target_compile_options(vds_tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vds_tests COMMAND vds_tests)
endif()
