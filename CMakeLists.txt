project(asgard)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
#find_package(GoogleTcmalloc)
find_package(Protoc)

#get git revision as the version number
#include(GetGitRevisionDescription)
#git_describe(GIT_REVISION)

IF(NOT CMAKE_BUILD_TYPE)
    #https://cmake.org/pipermail/cmake/2009-June/030311.html
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -std=c++14 -Wno-reorder -Wno-unused-parameter -rdynamic")
    SET(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAG} --coverage -g")
    SET(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS} --coverage")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")

# Load in the needed CMake modules
include(UseBoost)

#add current compilation dir to include path to handle config.h
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/rapidjson/include")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/date/include")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/")
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}")

link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(asgard)
