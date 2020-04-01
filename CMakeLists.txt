cmake_minimum_required(VERSION 3.0.0)

project(generator)
add_executable(generator src/generator.cpp)

project(cmap)

set (CMAKE_VERBOSE_MAKEFILE ON)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -march=native -flto -fopenmp -funroll-loops")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/permutation.hpp
    COMMAND generator
    DEPENDS generator
    COMMENT "Generating cmap::permute and cmap::unravel template specializations"
)

enable_testing()

add_executable(test1 tests/test1.cpp ${CMAKE_BINARY_DIR}/permutation.hpp)
add_executable(test2 tests/test2.cpp)
add_executable(test3 tests/test3.cpp)

target_include_directories(test1 PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR})
target_include_directories(test2 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test3 PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(permutation    test1)
add_test(cmap:iterator  test2)
add_test(cmap:erase     test3)


