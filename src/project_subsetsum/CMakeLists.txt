cmake_minimum_required (VERSION 2.8)
project (SUBSET_SUM)
# The version number.
set (SUBSET_SUM_VERSION_MAJOR 0)
set (SUBSET_SUM_VERSION_MINOR 12)

# These will make cmake use clang and clang++ instead of gcc and g++
#SET (CMAKE_C_COMPILER             "/usr/bin/clang")
#SET (CMAKE_C_FLAGS                "-Wall -std=c99")
#SET (CMAKE_C_FLAGS_DEBUG          "-g")
#SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
#SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

#looks like on OSX the c++ clang compiler is also clang, not clang++
#SET (CMAKE_CXX_COMPILER             "/usr/bin/clang")
#SET (CMAKE_CXX_COMPILER             "/opt/local/bin/clang++-mp-3.1")
#SET (CMAKE_CXX_FLAGS                "-Wall")
#SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
#SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
#SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

#SET (CMAKE_AR      "/usr/bin/llvm-ar")
#SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
#SET (CMAKE_NM      "/usr/bin/llvm-nm")
#SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
#SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")

set(PLATFORM 64)

#This may need to be updated or Linux/Windows
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)

#this adds the undvc_common FindX.cmake modules to look for BOINC and OpenCV
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/undvc_common/cmake/Modules/")

#
#   Just in case we need Boost
#
#set(BOOST_LIB_PREFIX "lib")
#set(Boost_LIB_PREFIX "lib")
#set(BOOST_COMPILER "-vc100")
#set(Boost_COMPILER "-vc100")
#set(Boost_ADDITIONAL_VERSIONS "1.47" "1.47.0")
#find_package(Boost REQUIRED system filesystem)
#include_directories (${Boost_INCLUDE_DIR})

find_package(BOINC)

add_subdirectory(undvc_common)
add_subdirectory(common)
add_subdirectory(client)

find_package(MySQL)
find_package(Boost REQUIRED COMPONENTS system)


if (BOINC_FOUND AND MYSQL_FOUND)
    link_directories(${Boost_LIBRARY_DIRS})
    add_subdirectory(server)
else (BOINC_FOUND AND MYSQL_FOUND)
    message(STATUS "Either BOINC libraries (found = ${BOINC_FOUND}) or MYSQL libraries (found = ${MYSQL_FOUND}) missing, not compiling server subdirectory.")
endif (BOINC_FOUND AND MYSQL_FOUND)

