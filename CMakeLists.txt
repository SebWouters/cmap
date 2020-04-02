cmake_minimum_required(VERSION 3.2.0)
include(CheckCXXCompilerFlag)

project(generator)
add_executable(generator src/generator.cpp)

project(cmap)

set (CMAKE_VERBOSE_MAKEFILE ON)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif()

check_cxx_compiler_flag (-Wall HAS_WALL)
if (HAS_WALL)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

find_package (OpenMP)
if (OpenMP_CXX_FOUND)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

check_cxx_compiler_flag (-xHost HAS_XHOST)
check_cxx_compiler_flag (-march=native HAS_MARCH_NATIVE)
if (HAS_XHOST)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -xHost")
elseif (HAS_MARCH_NATIVE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

check_cxx_compiler_flag (-ipo HAS_IPO)
if (HAS_IPO)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ipo")
endif()

check_cxx_compiler_flag (-flto HAS_FLTO)
if (HAS_FLTO)
    set (CMAKE_CXX_FLAGS "-flto ${CMAKE_CXX_FLAGS}")
endif()

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
add_executable(test4 tests/test4.cpp)
add_executable(test5 tests/test5.cpp)

target_include_directories(test1 PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR})
target_include_directories(test2 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test3 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test4 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test5 PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(permutation      test1)
add_test(cmap:iterator    test2)
add_test(cmap:erase       test3)
add_test(cmap:overwrite   test4)
add_test(cmap:emplace     test5)


