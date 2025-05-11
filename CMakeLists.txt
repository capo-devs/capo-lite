cmake_minimum_required(VERSION 3.23)

include(version.cmake)

project(capo-lite VERSION ${capo_version})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

set(capo_is_top_level ${PROJECT_IS_TOP_LEVEL})

option(CAPO_BUILD_EXAMPLES "Build capo examples" ${capo_is_top_level})
option(CAPO_MA_DEBUG_OUTPUT "Enable miniaudio debug output" ${capo_is_top_level})

add_subdirectory(ext)

if(capo_is_top_level)
  configure_file(Doxyfile.in "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile" @ONLY)
endif()

add_subdirectory(lib)

if(CAPO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
