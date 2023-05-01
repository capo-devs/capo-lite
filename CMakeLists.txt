cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

project(capo-lite VERSION "1.0.0")

set(is_root_project OFF)

if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
  set(is_root_project ON)
endif()

option(CAPO_USE_OPENAL "Build and link to OpenAL (for audio)" ON)
option(CAPO_BUILD_EXAMPLES "Build capo examples" ${is_root_project})

add_subdirectory(ext)
add_subdirectory(capo)

if(CAPO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
