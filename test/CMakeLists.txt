cmake_minimum_required(VERSION 3.16)
project(ad57X1_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AD57X1_COVERAGE "Instrument the library for gcov coverage reports" OFF)

# Use the system GoogleTest if there is one (e.g. libgtest-dev), otherwise download it.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(ad57X1_tests
  test_ad57X1.cpp
  mocks/mocks.cpp
  ../src/ad57X1.cpp
)
# The mocks directory provides Arduino.h and SPI.h
target_include_directories(ad57X1_tests PRIVATE mocks ../src)
target_compile_options(ad57X1_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(ad57X1_tests PRIVATE GTest::gtest GTest::gtest_main)

if(AD57X1_COVERAGE)
  target_compile_options(ad57X1_tests PRIVATE --coverage -O0 -g -fno-inline)
  target_link_options(ad57X1_tests PRIVATE --coverage)
endif()

enable_testing()
include(GoogleTest)
gtest_discover_tests(ad57X1_tests)
