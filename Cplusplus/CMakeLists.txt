cmake_minimum_required(VERSION 3.28)
# CMAKE VERSION-4.2.0 UUID
# https://github.com/Kitware/CMake/blob/v4.2.0/Help/dev/experimental.rst#id7
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

project(Cplusplus-learn
    VERSION 1.0.0
    DESCRIPTION "Cplusplus-learn"
    HOMEPAGE_URL ""
    LANGUAGES C CXX 
    #CUDA
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

#set(CMAKE_CUDA_STANDARD 20)
#set(CMAKE_CUDA_STANDARD_REQUIRED ON)
#set(CMAKE_CUDA_ARCHITECTURES 61) # GTX1080Ti
#set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
#set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler")
#set(CMAKE_CUDA_ARCHITECTURES native)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(cmake/Global.cmake)
include(cmake/Functions.cmake)
# Comes with third-party libraries and vcpkg management; whichever is added to the search path first will be found first.
include(cmake/Dependencies.cmake) # vcpkg 3rdparty
include(cmake/CompilerConfigs.cmake)
set(CPLUSPLUS_LEARN_BIN_DIR "${CMAKE_SOURCE_DIR}/out/Cplusplus_learn_${CMAKE_BUILD_TYPE}_${PLATFORM_SUFFIX}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "Runtime output directory")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "Library output directory")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "Archive output directory")
set(CMAKE_PCH_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "Precompiled header output directory")
set(CMAKE_ASM_LISTING_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "Assembly listing output directory")
set(CMAKE_CXX_MODULE_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "C++ module output directory")
if (COMPILER_MSVC)
  set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "PDB compile output directory")
  set(CMAKE_PDB_OUTPUT_DIRECTORY ${CPLUSPLUS_LEARN_BIN_DIR} CACHE PATH "PDB output directory")
endif()

add_subdirectory(src)
#add_subdirectory(src/cuda)
#add_subdirectory(src/leetcode)
#add_subdirectory(src/module)
#add_subdirectory(src/music)
#add_subdirectory(src/office)
#add_subdirectory(src/opencv)
#add_subdirectory(src/opengl)
#add_subdirectory(src/parallel)
#add_subdirectory(src/radix_sort)
#add_subdirectory(src/serialization)
#add_subdirectory(src/games101)